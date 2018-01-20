// https://forum.minetest.net/viewtopic.php?p=308356#p308356

#include "ssim_downscaling.h"

#include <assert.h>

#define SQR_NP 2 // squareroot of the patch size, recommended: 2


#define CLAMP(V, A, B) (V) < (A) ? (A) : (V) > (B) ? (B) : (V)

struct matrix {
	u32 w;
	u32 h;
	float *data;
};

/*! \brief get y, cb and cr values each in [0;1] from u8 r, g and b values
 *
 * there's gamma correction,
 * see http://www.ericbrasseur.org/gamma.html?i=1#Assume_a_gamma_of_2.2
 * 0.5 is added to cb and cr to have them in [0;1]
 */
static void rgb2ycbcr(u8 r8, u8 g8, u8 b8, float *y, float *cb, float *cr)
{
	float divider = 1.0f / 255.0f;
	float r = powf(r8 * divider, 2.2f);
	float g = powf(g8 * divider, 2.2f);
	float b = powf(b8 * divider, 2.2f);
	*y = (0.299f * r + 0.587f * g + 0.114f * b);
	*cb = (-0.168736f * r - 0.331264f * g + 0.5f * b) + 0.5f;
	*cr = (0.5f * r - 0.418688f * g - 0.081312f * b) + 0.5f;
}

/*! \brief the inverse of the function above
 *
 * numbers from http://www.equasys.de/colorconversion.html
 * if values are too big or small, they're clamped
 */
static void ycbcr2rgb(float y, float cb, float cr, u8 *r, u8 *g, u8 *b)
{
	float vr = (y + 1.402f * (cr - 0.5f));
	float vg = (y - 0.344136f * (cb - 0.5f) - 0.714136f * (cr - 0.5f));
	float vb = (y + 1.772f * (cb - 0.5f));
	float exponent = 1.0f / 2.2f;
	vr = powf(vr, exponent);
	vg = powf(vg, exponent);
	vb = powf(vb, exponent);
	*r = CLAMP(vr * 255.0f, 0, 255);
	*g = CLAMP(vg * 255.0f, 0, 255);
	*b = CLAMP(vb * 255.0f, 0, 255);
}

/*! \brief Convert an rgba image to 4 ycbcr matrices with values in [0, 1]
 */
static struct matrix *image_to_matrices(u32 *raw, u32 w, u32 h)
{
	struct matrix *matrices =
		(struct matrix *)malloc(4 * sizeof(struct matrix));
	for (int i = 0; i < 4; ++i) {
		matrices[i].w = w;
		matrices[i].h = h;
		matrices[i].data = (float *)malloc(w * h * sizeof(float));
	}
	for (u32 i = 0; i < w * h; ++i) {
		u8 *rgba = (u8 *)&raw[i];
		// put y, cb, cr and transpatency into the matrices
		rgb2ycbcr(*rgba, *(rgba+1), *(rgba+2),
			&matrices[0].data[i], &matrices[1].data[i], &matrices[2].data[i]);
		float divider = 1.0f / 255.0f;
		matrices[3].data[i] = *(rgba+3) * divider;
	}
	return matrices;
}


video::ITexture *add_texture_with_mipmaps(const std::string &name,
	video::IImage *img, video::IVideoDriver *driver)
{
	core::dimension2d<u32> dim = img->getDimension();
	u32 w = dim.Width;
	u32 h = dim.Height;

	// mipmaps are generated until width and height are 1,
	// see https://git.io/vNgmX
	int mipmapcnt = MYMAX(ceil(logf(w) / logf(2)), ceil(logf(h) / logf(2)));

	// put the original texture into a matrix
	assert(img->getImageDataSizeInBytes() == w * h * 4); // ensure rgba size
	//~ u32 *raw = (u32 *)img->lock(video::ETLM_READ_ONLY, 0);
	u32 *raw = (u32 *)img->lock();
	struct matrix *matrices = image_to_matrices(raw, w, h);
	img->unlock();
	for (int i = 0; i < 4; ++i)
		free(matrices[i].data);
	free(matrices);





	// total size of all mipmap images in bytes
	u32 data_size = 0;
	video::IImage *smallers[mipmapcnt];
	// generate images
	int k;
	for (k = 0; k < mipmapcnt; ++k) {
		if (dim.Width == 1 || dim.Height == 1)
			// stripes are downscaled differently (they usually don't appear)
			break;
		dim.Width /= 2;
		dim.Height /= 2;
		video::IImage *smaller = driver->createImage(img->getColorFormat(),
			dim);
		//~ sanity_check(smaller != NULL);
		// custom scaling algorithm
		video::SColor col = video::SColor(255, 255, 255, 200);
		smaller->fill(col);
		// get image size
		data_size += smaller->getImageDataSizeInBytes();
		smallers[k] = smaller;
	}
	for (; k < mipmapcnt; ++k) {
		// stripe downscaling, this only happens for non-square textures
		dim.Width /= 2;
		dim.Height /= 2;
		if (!dim.Width)
			dim.Width = 1;
		else if (!dim.Height)
			dim.Height = 1;
		video::IImage *smaller = driver->createImage(img->getColorFormat(),
			dim);
		//~ sanity_check(smaller != NULL);
		// custom scaling algorithm
		video::SColor col = video::SColor(255, 255, 255, 200);
		smaller->fill(col);
		// get image size
		data_size += smaller->getImageDataSizeInBytes();
		smallers[k] = smaller;
	}
	// allocate memory for all image data
	u8* data = new u8[data_size];
	int current_pos = 0;
	// copy image data to the array
	for (int k = 0; k < mipmapcnt; ++k) {
		u8* raw = (u8*) smallers[k]->lock();
		u32 len = smallers[k]->getImageDataSizeInBytes();
		memcpy(data + current_pos, raw, len);
		smallers[k]->unlock();
		current_pos += len;
	}

	// create the texture
	video::ITexture *tex = driver->addTexture(name.c_str(), img, data);

	// clean up memory
	delete[] data;
	for (int k = 0; k < mipmapcnt; ++k)
		smallers[k]->drop();

	return tex;
}
