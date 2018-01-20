// https://forum.minetest.net/viewtopic.php?p=308356#p308356

#include "ssim_downscaling.h"


video::ITexture *add_texture_with_mipmaps(const std::string &name,
	video::IImage *img, video::IVideoDriver *driver)
{
	core::dimension2d<u32> dim = img->getDimension();
	int c1 = ceil(logf(dim.Width) / logf(2));
	int c2 = ceil(logf(dim.Height) / logf(2));
	int mipmapcnt = MYMAX(c1, c2);
	// total size of all mipmap images in bytes
	u32 data_size = 0;
	video::IImage *smallers[mipmapcnt];
	// generate images
	for (int k = 0; k < mipmapcnt; ++k) {
		dim.Width /= 2;
		if (!dim.Width)
			dim.Width = 1;
		dim.Height /= 2;
		if (!dim.Height)
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
