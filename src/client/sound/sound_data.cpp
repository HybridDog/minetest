// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 DS
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2011 Sebastian 'Bahamada' Rühl
// Copyright (C) 2011 Cyriaque 'Cisoun' Skrapits <cysoun@gmail.com>
// Copyright (C) 2011 Giuseppe Bilotta <giuseppe.bilotta@gmail.com>

#include "sound_data.h"
#include "sound_data_ogg.h"

#include "sound_constants.h"
#include <algorithm>

namespace sound {

/*
 * SoundDataUnopenBuffer struct
 */

std::shared_ptr<ISoundDataOpen> SoundDataUnopenBuffer::open(const std::string &sound_name) &&
{
	return ISoundDataOpenOgg::fromBuffer(sound_name, std::move(m_buffer));
}

/*
 * SoundDataUnopenFile struct
 */

std::shared_ptr<ISoundDataOpen> SoundDataUnopenFile::open(const std::string &sound_name) &&
{
	return ISoundDataOpenOgg::fromFile(sound_name, m_path);
}

} // namespace sound
