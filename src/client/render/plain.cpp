/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "plain.h"
#include "settings.h"
#include "client/client.h"
#include "client/tile.h"


#include <stdio.h>


// With the IShaderConstantSetter, uniforms of the shader can be set (probably)
class OversampleShaderConstantSetter : public IShaderConstantSetter
{
public:
	OversampleShaderConstantSetter(RenderingCorePlain *core):
		m_core(core),
		m_pixlen("u_base_pixlen")
	{}

	~OversampleShaderConstantSetter() override = default;

	void onSetConstants(video::IMaterialRendererServices *services,
			bool is_highlevel) override
	{
		if (!is_highlevel)
			return;

		v2u32 render_size = m_core->getScreensize() * 2;
		float as_array[2] = {
			1.0f / (float)render_size.X,
			1.0f / (float)render_size.Y,
		};
		m_pixlen.set(as_array, services);
	}

	//~ void onSetMaterial(const video::SMaterial& material) override
	//~ {
		//~ m_render_size = render_size;
	//~ }

private:
	RenderingCorePlain *m_core;
	CachedPixelShaderSetting<float, 2> m_pixlen;
};

// Each shader requires a constant setter and a factory for it
class OversampleShaderConstantSetterFactory : public IShaderConstantSetterFactory
{
	RenderingCorePlain *m_core;
public:
	OversampleShaderConstantSetterFactory(RenderingCorePlain *core):
		m_core(core)
	{}

	virtual IShaderConstantSetter* create()
	{
		return new OversampleShaderConstantSetter(m_core);
	}
};



inline u32 scaledown(u32 coef, u32 size)
{
	return (size + coef - 1) / coef;
}

RenderingCorePlain::RenderingCorePlain(
	IrrlichtDevice *_device, Client *_client, Hud *_hud)
	: RenderingCore(_device, _client, _hud)
{
	scale = g_settings->getU16("undersampling");


	IWritableShaderSource *s = client->getShaderSource();
	// The factory must be added before the getShader call.
	s->addShaderConstantSetterFactory(new
		OversampleShaderConstantSetterFactory(this));

	u32 shader = s->getShader("oversampling1", TILE_MATERIAL_BASIC, 0);
	prepareMaterial(s, shader, 2, mat1);
	prepareMaterial(s, shader, 2, mat2);
	// TODO: hat 3 zwei eingangstexturen?
	prepareMaterial(s, shader, 2, mat3);
}

void RenderingCorePlain::prepareMaterial(IWritableShaderSource *s, int shader,
	int n, video::SMaterial &mat)
{
	// The material has the texture inputs for the oversampling shader
	mat.UseMipMaps = false;
	mat.MaterialType = s->getShaderInfo(shader).material;
	for (int k = 0; k < n; ++k) {
		mat.TextureLayer[k].AnisotropicFilter = false;
		mat.TextureLayer[k].BilinearFilter = false;
		mat.TextureLayer[k].TrilinearFilter = false;
		mat.TextureLayer[k].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
		mat.TextureLayer[k].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
	}
}

void RenderingCorePlain::initTextures()
{
	v2u32 render_size = screensize * 2;
	texture_rendered = driver->addRenderTargetTexture(render_size, "3d_render",
		video::ECF_A16B16G16R16F);
		//~ video::ECF_A8R8G8B8);
	mat1.TextureLayer[0].Texture = texture_rendered;

	texture_l = driver->addRenderTargetTexture(screensize, "linear_downscaled",
		video::ECF_A16B16G16R16F);
	texture_l2 = driver->addRenderTargetTexture(screensize,
		"squared_linear_down", video::ECF_A16B16G16R16F);
	renderTargets1.push_back(texture_l);
	renderTargets1.push_back(texture_l2);
	mat2.TextureLayer[0].Texture = texture_l;
	mat2.TextureLayer[1].Texture = texture_l2;

	texture_m = driver->addRenderTargetTexture(screensize,
		"data_m", video::ECF_A16B16G16R16F);
	texture_r = driver->addRenderTargetTexture(screensize,
		"data_r", video::ECF_A16B16G16R16F);
	mat3.TextureLayer[0].Texture = texture_m;
	mat3.TextureLayer[1].Texture = texture_r;


	if (scale <= 1)
		return;
	v2u32 size{scaledown(scale, screensize.X), scaledown(scale, screensize.Y)};
	lowres = driver->addRenderTargetTexture(
			size, "render_lowres", video::ECF_A8R8G8B8);
}

void RenderingCorePlain::clearTextures()
{
	driver->removeTexture(texture_rendered);
	driver->removeTexture(texture_l);
	driver->removeTexture(texture_l2);
	driver->removeTexture(texture_m);
	driver->removeTexture(texture_r);

	if (scale <= 1)
		return;
	driver->removeTexture(lowres);
}

void RenderingCorePlain::beforeDraw()
{
	if (scale <= 1)
		return;
	driver->setRenderTarget(lowres, true, true, skycolor);
}

void RenderingCorePlain::upscale()
{
	if (scale <= 1)
		return;
	driver->setRenderTarget(0, true, true);
	v2u32 size{scaledown(scale, screensize.X), scaledown(scale, screensize.Y)};
	v2u32 dest_size{scale * size.X, scale * size.Y};
	driver->draw2DImage(lowres, core::rect<s32>(0, 0, dest_size.X, dest_size.Y),
			core::rect<s32>(0, 0, size.X, size.Y));
}

void RenderingCorePlain::drawImage(video::SMaterial &mat)
{
	static const video::S3DVertex vertices[4] = {
			video::S3DVertex(1.0, -1.0, 0.0, 0.0, 0.0, -1.0,
					video::SColor(255, 0, 255, 255), 1.0, 0.0),
			video::S3DVertex(-1.0, -1.0, 0.0, 0.0, 0.0, -1.0,
					video::SColor(255, 255, 0, 255), 0.0, 0.0),
			video::S3DVertex(-1.0, 1.0, 0.0, 0.0, 0.0, -1.0,
					video::SColor(255, 255, 255, 0), 0.0, 1.0),
			video::S3DVertex(1.0, 1.0, 0.0, 0.0, 0.0, -1.0,
					video::SColor(255, 255, 255, 255), 1.0, 1.0),
	};
	static const u16 indices[6] = {0, 1, 2, 2, 3, 0};
	driver->setMaterial(mat);
	driver->drawVertexPrimitiveList(&vertices, 4, &indices, 2);
}

void RenderingCorePlain::drawAll()
{
	// Draw the actual frame without HUD
	driver->setRenderTarget(texture_rendered, true, true, skycolor);
	draw3D();

	// Do the downscaling
	driver->setRenderTarget(nullptr, false, false, skycolor);
	drawImage(mat1);



	// Draw HUD etc.
	drawPostFx();
	//~ upscale();
	drawHUD();
}




