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

#pragma once
#include "core.h"
#include "client/shader.h"

class RenderingCorePlain : public RenderingCore
{
protected:
	int scale = 0;
	video::ITexture *lowres = nullptr;
	video::ITexture *texture_rendered = nullptr;
	video::SMaterial mat1;
	video::ITexture *texture_l = nullptr;
	video::ITexture *texture_l2 = nullptr;
	// The rendertargets are required when rendering to more than just a single
	// output texture
	irr::core::array<irr::video::IRenderTarget> renderTargets1;
	video::SMaterial mat2;
	video::ITexture *texture_m = nullptr;
	video::ITexture *texture_r = nullptr;
	irr::core::array<irr::video::IRenderTarget> renderTargets2;
	video::SMaterial mat3;

	void initTextures() override;
	void clearTextures() override;
	void beforeDraw() override;
	void upscale();
	void prepareMaterial(IWritableShaderSource *s, int shader, int n,
		video::SMaterial &mat);
	void drawImage(video::SMaterial &mat);

public:
	RenderingCorePlain(IrrlichtDevice *_device, Client *_client, Hud *_hud);
	void drawAll() override;
	v2u32 getScreensize() const { return screensize; }
};
