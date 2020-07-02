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
#include "client/shader.h"
#include "client/tile.h"


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
	u32 shader = s->getShader("oversampling1", TILE_MATERIAL_BASIC, 0);
	mat1.UseMipMaps = false;
	mat1.MaterialType = s->getShaderInfo(shader).material;
	mat1.TextureLayer[0].AnisotropicFilter = false;
	mat1.TextureLayer[0].BilinearFilter = false;
	mat1.TextureLayer[0].TrilinearFilter = false;
	mat1.TextureLayer[0].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
	mat1.TextureLayer[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
}

void RenderingCorePlain::initTextures()
{
	v2u32 render_size = screensize * 2;
	rendered = driver->addRenderTargetTexture(render_size, "3d_render",
		//~ video::ECF_A8R8G8B8);
		video::ECF_A16B16G16R16F);
	renderTargets.push_back(rendered);
	mat1.TextureLayer[0].Texture = rendered;

	if (scale <= 1)
		return;
	v2u32 size{scaledown(scale, screensize.X), scaledown(scale, screensize.Y)};
	lowres = driver->addRenderTargetTexture(
			size, "render_lowres", video::ECF_A8R8G8B8);
}

void RenderingCorePlain::clearTextures()
{
	driver->removeTexture(rendered);

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

void RenderingCorePlain::drawAll()
{
	//~ driver->setRenderTarget(renderTargets, true, true, skycolor);
	driver->setRenderTarget(rendered, true, true, skycolor);
	draw3D();

	driver->setRenderTarget(nullptr, false, false, skycolor);
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
	driver->setMaterial(mat1);
	driver->drawVertexPrimitiveList(&vertices, 4, &indices, 2);


	drawPostFx();
	//~ upscale();
	drawHUD();
}
