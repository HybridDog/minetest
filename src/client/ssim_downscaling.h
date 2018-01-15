#include <ITexture.h>
#include <IVideoDriver.h>
#include "util/string.h"
#include "util/numeric.h"

video::ITexture *add_texture_with_mipmaps(const std::string &name,
	video::IImage *img, video::IVideoDriver *driver);
