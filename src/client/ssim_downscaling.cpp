// https://forum.minetest.net/viewtopic.php?p=308356#p308356

#include "ssim_downscaling.h"

#include "log.h"

#define SQR_NP 2 // squareroot of the patch size, recommended: 2
#define LINEAR_RATIO 0.5f // used for mixing in linear downscaled values


#define CLAMP(V, A, B) (V) < (A) ? (A) : (V) > (B) ? (B) : (V)
#define MIN(V, R) ((V) < (R) ? (V) : (R))
#define MAX(V, R) ((V) > (R) ? (V) : (R))
#define INDEX(X, Y, STRIDE) ((Y) * (STRIDE) + (X))

struct Matrix {
	u32 w;
	u32 h;
	std::unique_ptr<f32[]> data;
	Matrix(u32 width, u32 height):
		w{width},
		h{height},
		data{std::make_unique<f32[]>(width * height)}
	{}
};

/*! \brief linear to sRGB conversion
 *
 * taken from https://github.com/tobspr/GLSL-Color-Spaces/
 */
f32 linear_to_srgb(f32 v)
{
	if (v > 0.0031308f)
		return 1.055f * powf(v, 1.0f / 2.4f) - 0.055f;
	return 12.92f * v;
}
f32 srgb_to_linear(f32 v)
{
	if (v > 0.04045f)
		return powf((v + 0.055f) / 1.055f, 2.4f);
	return v / 12.92f;
}

/*! \brief get y, cb and cr values each in [0;1] from u8 r, g and b values
 *
 * there's gamma correction,
 * see http://www.ericbrasseur.org/gamma.html?i=1#Assume_a_gamma_of_2.2
 * 0.5 is added to cb and cr to have them in [0;1]
 */
static void rgb2ycbcr(u8 r_8, u8 g_8, u8 b_8, f32 &y, f32 &cb, f32 &cr)
{
	f32 r = srgb_to_linear(r_8 / 255.0f);
	f32 g = srgb_to_linear(g_8 / 255.0f);
	f32 b = srgb_to_linear(b_8 / 255.0f);
	y = (0.299f * r + 0.587f * g + 0.114f * b);
	cb = (-0.168736f * r - 0.331264f * g + 0.5f * b) + 0.5f;
	cr = (0.5f * r - 0.418688f * g - 0.081312f * b) + 0.5f;
}

/*! \brief the inverse of the function above
 *
 * numbers from http://www.equasys.de/colorconversion.html
 * if values are too big or small, they're clamped
 */
static void ycbcr2rgb(f32 y, f32 cb, f32 cr, u8 &r_8, u8 &g_8, u8 &b_8)
{
	f32 r = (y + 1.402f * (cr - 0.5f));
	f32 g = (y - 0.344136f * (cb - 0.5f) - 0.714136f * (cr - 0.5f));
	f32 b = (y + 1.772f * (cb - 0.5f));
	r = linear_to_srgb(r);
	g = linear_to_srgb(g);
	b = linear_to_srgb(b);
	r_8 = CLAMP(r * 255.0f, 0, 255);
	g_8 = CLAMP(g * 255.0f, 0, 255);
	b_8 = CLAMP(b * 255.0f, 0, 255);
}

/*! \brief Convert an bgra image to 4 ycbcr matrices with values in [0, 1]
 */
void image_to_matrices(u32 *raw, std::array<Matrix, 4> &matrices)
{
	u32 w = matrices[0].w;
	u32 h = matrices[0].h;
	for (u32 i = 0; i < w * h; ++i) {
		u8 *bgra = (u8 *)&raw[i];
		// put y, cb, cr and transpatency into the matrices
		rgb2ycbcr(*(bgra+2), *(bgra+1), *bgra,
			matrices[0].data[i], matrices[1].data[i], matrices[2].data[i]);
		matrices[3].data[i] = *(bgra+3) / 255.0f;
	}
}

/*! \brief Convert 4 matrices to an bgra image, which is passed
 */
static void matrices_to_image(std::array<Matrix, 4> &matrices, u32 *raw)
{
	int w = matrices[0].w;
	int h = matrices[0].h;
	for (int i = 0; i < w * h; ++i) {
		u8 *bgra = (u8 *)&raw[i];
		ycbcr2rgb(matrices[0].data[i], matrices[1].data[i], matrices[2].data[i],
			*(bgra+2), *(bgra+1), *bgra);
		float a = matrices[3].data[i] * 255;
		*(bgra+3) = CLAMP(a, 0, 255);
	}
}

/*! \brief Calculate the downscaled L and L2 for lower resolutions
 *
 * \param mat The input data
 * \param targets The resolutions and memory for the lower-resolution output
 */
static void downscale(Matrix &mat, std::vector<std::array<Matrix, 2>> &targets)
{
	u32 w{mat.w};
	u32 h{mat.h};
	u32 input_size{w * h};
	f32 *l{mat.data.get()};
	auto l2_init{std::make_unique<f32[]>(input_size)};
	f32 *l2{l2_init.get()};
	for (u32 i{0}; i < input_size; ++i) {
		l2[i] = l[i] * l[i];
	}
	for (auto &mats_smaller : targets) {
		Matrix &mat_smaller_l{mats_smaller[0]};
		Matrix &mat_smaller_l2{mats_smaller[1]};
		u32 w2{mat_smaller_l.w};
		u32 h2{mat_smaller_l.h};
		u32 scaling_w{w / w2};
		u32 scaling_h{h / h2};
		f32 divider_s = 1.0f / (scaling_w * scaling_h);
		for (u32 y_small{0}; y_small < h2; ++y_small) {
			for (u32 x_small{0}; x_small < w2; ++x_small) {
				f32 acc_l{0};
				f32 acc_l2{0};
				u32 x{x_small * scaling_w};
				u32 y{y_small * scaling_h};
				for (u32 yc = y; yc < y + scaling_h; ++yc) {
					for (u32 xc = x; xc < x + scaling_w; ++xc) {
						u32 vi{INDEX(xc % w, yc % h, w)};
						acc_l += l[vi];
						acc_l2 += l2[vi];
					}
				}
				u32 vi{INDEX(x_small, y_small, w2)};
				mat_smaller_l.data[vi] = acc_l * divider_s;
				mat_smaller_l2.data[vi] = acc_l2 * divider_s;
			}
		}
		w = w2;
		h = h2;
		l = mat_smaller_l.data.get();
		l2 = mat_smaller_l2.data.get();
	}
}

/*! \brief Use the L and L2 to calculate the perceptually-downscaled output
 *
 * \param mats The input data L and L2
 * \param target Output
 */
static void sharpen(std::array<Matrix, 2> &mats, Matrix &target)
{
	u32 w{mats[0].w};
	u32 h{mats[0].h};
	auto &l{mats[0].data};
	auto &l2{mats[1].data};
	auto m_all{std::make_unique<f32[]>(w * h)};
	auto r_all{std::make_unique<f32[]>(w * h)};
	auto &d{target.data};

	f32 patch_sz_div = 1.0f / (SQR_NP * SQR_NP);

	// Calculate m and r for all patch offsets
	for (u32 y_start{0}; y_start < h; ++y_start) {
		for (u32 x_start{0}; x_start < w; ++x_start) {
			f32 acc_m{0};
			f32 acc_r_1{0};
			f32 acc_r_2{0};
			for (u32 y{y_start}; y < y_start + SQR_NP; ++y) {
				for (u32 x{x_start}; x < x_start + SQR_NP; ++x) {
					u32 i{INDEX(x % w, y % h, w)};
					acc_m += l[i];
					acc_r_1 += l[i] * l[i];
					acc_r_2 += l2[i];
				}
			}
			f32 mv{acc_m * patch_sz_div};
			f32 slv{acc_r_1 * patch_sz_div - mv * mv};
			f32 shv{acc_r_2 * patch_sz_div - mv * mv};
			u32 i{INDEX(x_start, y_start, w)};
			m_all[i] = mv;
			if (slv >= 0.000001f) // epsilon is 10⁻⁶
				r_all[i] = sqrtf(shv / slv);
			else
				r_all[i] = 1.0f;
		}
	}

	// Calculate the average of the results of all possible patch sets
	// d is the output
	for (u32 y{0}; y < h; ++y) {
		for (u32 x{0}; x < w; ++x) {
			u32 i{INDEX(x, y, w)};
			f32 liner_scaled{l[i]};
			f32 acc_d{0};
			for (int y_offset{0}; y_offset > -SQR_NP; --y_offset) {
				for (int x_offset{0}; x_offset > -SQR_NP; --x_offset) {
					int x_patch_off{static_cast<int>(x) + x_offset};
					int y_patch_off{static_cast<int>(y) + y_offset};
					x_patch_off = (x_patch_off + w) % w;
					y_patch_off = (y_patch_off + h) % h;
					u32 i_patch_off{INDEX(x_patch_off, y_patch_off, w)};
					f32 mv{m_all[i_patch_off]};
					f32 rv{r_all[i_patch_off]};
					acc_d += mv + rv * liner_scaled - rv * mv;
				}
			}
			d[i] = liner_scaled * LINEAR_RATIO
				+ acc_d * patch_sz_div * (1.0f - LINEAR_RATIO);
		}
	}
}


/*! \brief The actual downscaling algorithm
 *
 * \param mat The 4 matrices obtained form image_to_matrices.
 * \param s The factor by which the image should become downscaled.
 */
static void downscale_perc(Matrix &mat, int downscale_factor, Matrix &target)
{
	u32 h2{mat.h / downscale_factor};
	u32 w2{mat.w / downscale_factor};
	std::vector<std::array<Matrix, 2>> downscaleds;
	downscaleds.emplace_back(std::array<Matrix, 2>{Matrix(w2, h2),
		Matrix(w2, h2)});
	downscale(mat, downscaleds);

	sharpen(downscaleds[0], target);
}

/*! \brief Function which calls functions for downscaling
 *
 * \param matrices The content from the original image.
 * \param downscale_factor Must be a natural number.
 * \param raw The place where the downscaled srgb image is saved to.
 */
static void downscale_an_image(std::array<Matrix, 4> &matrices,
	int downscale_factor, u32 *raw)
{
	u32 h = matrices[0].h;
	u32 w = matrices[0].w;
	u32 h2 = h / downscale_factor;
	u32 w2 = w / downscale_factor;
	std::array<Matrix, 4> smaller_matrices{Matrix(w2, h2), Matrix(w2, h2),
		Matrix(w2, h2), Matrix(w2, h2)};
	for (int i = 0; i < 4; ++i) {
		downscale_perc(matrices[i], downscale_factor, smaller_matrices[i]);
	}
	matrices_to_image(smaller_matrices, raw);
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

	// ensure rgba size
	if (img->getImageDataSizeInBytes() != w * h * 4)
		errorstream << "size is " << img->getImageDataSizeInBytes() <<
			" but expected " << w*h*4 << std::endl;
	if (img->getColorFormat() != video::ECF_A8R8G8B8)
		errorstream << "unexpected colour format" << std::endl;
	// the bytes are in bgra order (in big endian order)

	// put the original texture into a matrix
	std::array<Matrix, 4> matrices{Matrix(w, h), Matrix(w, h), Matrix(w, h),
		Matrix(w, h)};
	image_to_matrices((u32 *)img->getData(), matrices);

	// Get the number of mip map images and their total size in bytes.
	// Mip maps are generated until the width and height are 1,
	// see https://git.io/vNgmX
	int total_pixel_cnt{0};
	int mipmapcnt{0};
	while (w > 1 || h > 1) {
		w /= 2;
		h /= 2;
		if (h == 0)
			h = 1;
		else if (w == 0)
			w = 1;
		total_pixel_cnt += w * h;
		++mipmapcnt;
	}
	auto data{std::make_unique<u32[]>(total_pixel_cnt)};

	w = dim.Width;
	h = dim.Height;

	//~ video::IImage *current_image = (video::IImage *)data

	// generate images
	int k;
	u32 *current_image{data.get()};
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
	video::ITexture *tex = driver->addTexture(name.c_str(), img);
	tex->regenerateMipMapLevels(data.get());

	return tex;
}
