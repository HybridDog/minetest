// https://forum.minetest.net/viewtopic.php?p=308356#p308356

#include "ssim_downscaling.h"

#include "log.h"

#define SQR_NP 2 // squareroot of the patch size, recommended: 2
#define LINEAR_RATIO 0.5f // used for mixing in linear downscaled values


#define CLAMP(V, A, B) (V) < (A) ? (A) : (V) > (B) ? (B) : (V)

struct matrix {
	u32 w;
	u32 h;
	float *data;
};

/*! \brief get y, cb and cr values each in [0;1] from u8 b, g and r values
 *
 * there's gamma correction because r, g and b are obviously in srgb format,
 * see http://www.ericbrasseur.org/gamma.html?i=1#Assume_a_gamma_of_2.2
 * 0.5 is added to cb and cr to have them in [0;1]
 */
static void rgb2ycbcr(u8 b8, u8 g8, u8 r8, float *y, float *cb, float *cr)
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
static void ycbcr2rgb(float y, float cb, float cr, u8 *b, u8 *g, u8 *r)
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

/*! \brief Convert an bgra image to 4 ycbcr matrices with values in [0, 1]
 */
static struct matrix *image_to_matrices(u32 *raw, u32 w, u32 h)
{
	struct matrix *matrices = new struct matrix[4];
	for (int i = 0; i < 4; ++i) {
		matrices[i].w = w;
		matrices[i].h = h;
		matrices[i].data = new float[w * h];
	}
	for (u32 i = 0; i < w * h; ++i) {
		u8 *bgra = (u8 *)&raw[i];
		// put y, cb, cr and transpatency into the matrices
		rgb2ycbcr(*bgra, *(bgra+1), *(bgra+2),
			&matrices[0].data[i], &matrices[1].data[i], &matrices[2].data[i]);
		float divider = 1.0f / 255.0f;
		matrices[3].data[i] = *(bgra+3) * divider;
	}
	return matrices;
}

/*! \brief Convert 4 matrices to an bgra image, which is passed
 */
static void matrices_to_image(struct matrix *matrices, u32 *raw)
{
	int w = matrices[0].w;
	int h = matrices[0].h;
	for (int i = 0; i < w * h; ++i) {
		u8 *bgra = (u8 *)&raw[i];
		ycbcr2rgb(matrices[0].data[i], matrices[1].data[i], matrices[2].data[i],
			bgra, bgra+1, bgra+2);
		float a = matrices[3].data[i] * 255;
		*(bgra+3) = CLAMP(a, 0, 255);
	}
}

/*! \brief Frees 4 matrices
 *
 * \param mat The 4 matrices, e.g. obtained form image_to_matrices.
 */
static void free_matrices(struct matrix *matrices)
{
	for (int i = 0; i < 4; ++i)
		delete[] matrices[i].data;
	delete[] matrices;
}

/*! \brief The actual downscaling algorithm
 *
 * \param mat One of the 4 matrices obtained form image_to_matrices.
 * \param s The factor by which the image should become downscaled.
 */
static void downscale_perc(struct matrix *mat, int s, struct matrix *target)
{
	// preparation
	int w = mat->w; // input width
	int h = mat->h;
	float *input = mat->data;
	int w2 = target->w; // output width
	int h2 = target->h;
	int output_elems_cnt = w2 * h2;
	float *l = new float[output_elems_cnt];
	float *l2 = new float[output_elems_cnt];
	float *d = target->data;

	// set d's entries to 0 (because it's used for a sum)
	for (int i = 0; i < w2 * h2; ++i)
		d[i] = 0;

	// get l and l2, the input image and it's size are used only here
	for (int ysm = 0; ysm < h2; ++ysm) {
		for (int xsm = 0; xsm < w2; ++xsm) {
			// xsm and ysm are coords for the subsampled image
			int x = xsm * s;
			int y = ysm * s;
			float acc = 0;
			float acc2 = 0;
			for (int yc = y; yc < y + s; ++yc) {
				for (int xc = x; xc < x + s; ++xc) {
					float v = input[((yc + h) % h) * w + (xc + w) % w];
					acc += v;
					acc2 += v * v;
				}
			}
			int ism = ysm*w2+xsm;
			float divider = 1.0f / (s * s);
			l[ism] = acc * divider;
			l2[ism] = acc2 * divider;
		}
	}

	float patch_sz_div = 1.0f / (SQR_NP * SQR_NP);
	// calculate the average of the results of all possible patch sets
	for (int y_offset = 0; y_offset > -SQR_NP; --y_offset) {
		for (int x_offset = 0; x_offset > -SQR_NP; --x_offset) {
			float *m = new float[output_elems_cnt];
			float *r = new float[output_elems_cnt];

			// get m
			for (int y = 0; y < h2; ++y) {
				for (int x = 0; x < w2; ++x) {
					float acc = 0;
					// ys (y start) can be -1, then h2-1 is used for the index
					int ys = y - (y + SQR_NP + y_offset) % SQR_NP;
					int xs = x - (x + SQR_NP + x_offset) % SQR_NP;
					for (int yc = ys; yc < ys + SQR_NP; ++yc) {
						for (int xc = xs; xc < xs + SQR_NP; ++xc) {
							acc += l[((yc + h2) % h2) * w2 + (xc + w2) % w2];
						}
					}
					m[y*w2+x] = acc * patch_sz_div;
				}
			}

			// get r
			for (int y = 0; y < h2; ++y) {
				for (int x = 0; x < w2; ++x) {
					float acc = 0;
					float acc2 = 0;
					int ys = y - (y + SQR_NP + y_offset) % SQR_NP;
					int xs = x - (x + SQR_NP + x_offset) % SQR_NP;
					for (int yc = ys; yc < ys + SQR_NP; ++yc) {
						for (int xc = xs; xc < xs + SQR_NP; ++xc) {
							int i = ((yc + h2) % h2) * w2 + (xc + w2) % w2;
							acc += l[i] * l[i];
							acc2 += l2[i];
						}
					}
					int i = y*w2+x;
					float mv = m[i];
					float slv = acc * patch_sz_div - mv * mv;
					float shv = acc2 * patch_sz_div - mv * mv;
					if (slv >= 0.000001f) // epsilon is 10⁻⁶
						r[i] = sqrtf(shv / slv);
					else
						r[i] = 0;
				}
			}

			// get d, which is the output
			for (int y = 0; y < h2; ++y) {
				for (int x = 0; x < w2; ++x) {
					float acc_m = 0;
					float acc_r = 0;
					float acc_t = 0;
					int ys = y - (y + SQR_NP + y_offset) % SQR_NP;
					int xs = x - (x + SQR_NP + x_offset) % SQR_NP;
					for (int yc = ys; yc < ys + SQR_NP; ++yc) {
						for (int xc = xs; xc < xs + SQR_NP; ++xc) {
							int i = ((yc + h2) % h2) * w2 + (xc + w2) % w2;
							acc_m += m[i];
							acc_r += r[i];
							acc_t += r[i] * m[i];
						}
					}
					int i = y*w2+x;
					d[i] += (
							acc_m * patch_sz_div
							+ acc_r * patch_sz_div * l[i]
							- acc_t * patch_sz_div
						);
				}
			}
			delete[] m;
			delete[] r;
		}
	}

	for (int i = 0; i < w2 * h2; ++i) {
		// divide values in d for the (arithmetic) average
		d[i] *= patch_sz_div;
		// select a mix between linear and this downscaling
		d[i] = l[i] * LINEAR_RATIO + d[i] * (1.0f - LINEAR_RATIO);
	}

/*
	for (int i = 0; i < w2 * h2; ++i) {
		// divide values in d for the (arithmetic) average
		d[i] *= patch_sz_div;
	}

	// select a mix between linear and this downscaling for low resolutions
	float linear_ratio = 0.0f;
	if (w2 <= 3)
		// likely generating a 2x2 image
		linear_ratio = 0.75f;
	else if (w2 <= 7)
		// likely generating a 4x4 image
		linear_ratio = 0.5f;
	else if (w2 <= 15)
		// likely generating a 8x8 image
		linear_ratio = 0.3f;
	if (linear_ratio > 0.0f) {
		float other_ratio = 1.0f - linear_ratio;
		for (int i = 0; i < w2 * h2; ++i)
			d[i] = l[i] * linear_ratio + d[i] * other_ratio;
	}
*/

	// tidy up
	delete[] l;
	delete[] l2;
}

/*! \brief Function which calls functions for downscaling
 *
 * \param matrices The content from the original image.
 * \param downscale_factor Must be a natural number.
 * \param raw The place where the downscaled srgb image is saved to.
 */
static void downscale_an_image(struct matrix *matrices, int downscale_factor,
	u32 *raw)
{
	int h = matrices[0].h;
	int w = matrices[0].w;
	int h2 = h / downscale_factor;
	int w2 = w / downscale_factor;
	struct matrix *smaller_matrices = new struct matrix[4];
	for (int i = 0; i < 4; ++i) {
		smaller_matrices[i].h = h2;
		smaller_matrices[i].w = w2;
		smaller_matrices[i].data = new float[w2 * h2];
		downscale_perc(&matrices[i], downscale_factor, &smaller_matrices[i]);
	}
	matrices_to_image(smaller_matrices, raw);
	free_matrices(smaller_matrices);
}

/*! \brief Function for linearly downscaling a stripe
 *
 * This also uses gamma correction.
 * If the longer_stripe had an odd length, one pixel is simply ignored.
 */
static void downscale_stripe(u32 *longer_stripe, u32 smaller_length,
	u32 *smaller_stripe)
{
	// bgra order again
	for (u32 x = 0; x < smaller_length; ++x) {
		u8 *bgra_l = (u8 *)&longer_stripe[2 * x];
		u8 *bgra_r = (u8 *)&longer_stripe[2 * x + 1];
		u8 *bgra_target = (u8 *)&smaller_stripe[x];
		for (int i = 0; i < 3; ++i) {
			bgra_target[i] = powf(
				0.5f * (powf(bgra_l[i], 2.2f) + powf(bgra_r[i], 2.2f)),
				1.0f / 2.2f);
		}
		// alpha doesn't need gamma correction (afaIk)
		bgra_target[3] = 0.5f * (bgra_l[3] + bgra_r[3]);
	}
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

	// ensure rgba size
	if (img->getImageDataSizeInBytes() != w * h * 4)
		errorstream << "size is " << img->getImageDataSizeInBytes() <<
			" but expected " << w*h*4 << std::endl;
	if (img->getColorFormat() != video::ECF_A8R8G8B8)
		errorstream << "unexpected colour format" << std::endl;
	// the bytes are in bgra order (in big endian order)

	// put the original texture into a matrix
	//~ u32 *raw = (u32 *)img->lock(video::ETLM_READ_ONLY, 0);
	u32 *raw = (u32 *)img->lock();
	struct matrix *matrices = image_to_matrices(raw, w, h);
	img->unlock();

	int k;
	// get the total size of all mipmap images in bytes
	int total_pixel_cnt = 0;
	for (k = 0; k < mipmapcnt; ++k) {
		w /= 2;
		h /= 2;
		if (h == 0)
			h = 1;
		else if (w == 0)
			w = 1;
		total_pixel_cnt += w * h;
	}
	u32 *data = new u32[total_pixel_cnt];

	w = dim.Width;
	h = dim.Height;

	//~ video::IImage *current_image = (video::IImage *)data

	// generate images
	u32 *current_image = data;
	for (k = 0; k < mipmapcnt; ++k) {
		if (w == 1 || h == 1)
			// stripes are downscaled differently (they usually don't appear)
			break;
		w /= 2;
		h /= 2;
		// each step the size is halved and floored
		int downscaling_factor = 1 << (k+1);
		downscale_an_image(matrices, downscaling_factor, current_image);
		// make current_image point to the next smaller image
		current_image += w * h;
	}

	u32 *previous_stripe = current_image - w * h;
	bool horizontal_stripe = h == 1;
	for (; k < mipmapcnt; ++k) {
		// stripe downscaling, this only happens for non-square textures
		w /= 2;
		h /= 2;
		if (horizontal_stripe)
			h = 1;
		else
			w = 1;
		downscale_stripe(previous_stripe, w * h, current_image);
		previous_stripe = current_image;
		current_image += w * h;
	}

	// create the texture
	video::ITexture *tex = driver->addTexture(name.c_str(), img, data);

	// clean up memory
	free_matrices(matrices);
	delete[] data;

	return tex;
}
