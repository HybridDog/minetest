#pragma once

#include <array>

#include <ITexture.h>
#include <IImage.h>
#include <IVideoDriver.h>

#include "irrlichttypes.h"
#include "util/numeric.h"


class TextureStochastic {
public:
	TextureStochastic(video::IVideoDriver &driver, const video::IImage &img,
		const std::string &name);

	video::ITexture *getGaussianizedTexture() const {
		return m_texture_gaussianized; }
	video::ITexture *getLUTTexture() const { return m_texture_lut; }
	std::array<float, 9> &getCorrelatingMatrix() {
		return m_correlating_matrix; }
	std::array<float, 3> &getColTranslation() { return m_col_translation; }

private:
	std::array<float, 9> m_correlating_matrix;
	std::array<float, 3> m_col_translation;
	video::ITexture *m_texture_gaussianized{nullptr};
	video::ITexture *m_texture_lut{nullptr};
};

// Save a pointer in the SMaterial in a hacky way
void store_in_material(video::SMaterial &material,
	const TextureStochastic *texture_stochastic);

// Get a pointer from the SMaterial in a hacky way
TextureStochastic *get_from_material(const video::SMaterial &material);
