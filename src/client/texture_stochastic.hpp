#pragma once

#include <array>

#include <ITexture.h>
#include <IImage.h>
#include <IVideoDriver.h>

#include "irrlichttypes.h"
#include "util/numeric.h"


/// Container for stochastic texture sampling data for a single image
class TextureStochastic {
public:
	/// The constructor performs the preprocessing to get the member variables
	TextureStochastic(video::IVideoDriver &driver, const video::IImage &img,
		const std::string &name);

	/*! Get a strongly normalized texture for the image passed to the
	 * constructor
	 *
	 * The texture has decorrelated colours and a gaussianized histogram, which
	 * the shader can undo with the content from the other member variables.
	 */
	video::ITexture *getGaussianizedTexture() const {
		return m_texture_gaussianized; }

	/*! Get the look up table for the inverse histogram transformation
	 *
	 * This texture turns the gaussianized histogram to the histogram of the
	 * input image with decorrelated colors.
	 */
	video::ITexture *getLUTTexture() const { return m_texture_lut; }

	/*! Get a matrix for the transformation back to the input image's colors
	 *
	 * The matrix bundles three linear operations:
	 * * Scaling colours from the Look Up Table texture, which is needed because
	 *   the texture has values in [0, 1]
	 * * Mapping colours back to the OKLab color space (the decorrelation)
	 * * Application of the first step of the OKLab-to-RGB conversion
	 */
	std::array<float, 9> &getCorrelatingMatrix() {
		return m_correlating_matrix; }

	/*! Get a color offset for the look up table
	 *
	 * This offset is added in the shaders since the look up table texture
	 * cannot contain negative values and the offset cannot be integrated in the
	 * decorrelation matrix as with matrix multiplication we can only scale and
	 * not offset vectors.
	 */
	std::array<float, 3> &getColTranslation() { return m_col_translation; }

private:
	std::array<float, 9> m_correlating_matrix;
	std::array<float, 3> m_col_translation;
	video::ITexture *m_texture_gaussianized{nullptr};
	video::ITexture *m_texture_lut{nullptr};
};
