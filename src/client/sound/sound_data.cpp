// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 DS
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2011 Sebastian 'Bahamada' Rühl
// Copyright (C) 2011 Cyriaque 'Cisoun' Skrapits <cysoun@gmail.com>
// Copyright (C) 2011 Giuseppe Bilotta <giuseppe.bilotta@gmail.com>

#include "sound_data.h"
#include "sound_data_mod.h"
#include "sound_data_ogg.h"

#include "sound_constants.h"
#include <algorithm>

namespace sound {

/*
 * SoundDataUnopenBuffer struct
 */

std::shared_ptr<ISoundDataOpen> SoundDataUnopenBuffer::open(const std::string &sound_name) &&
{
warningstream << sound_name << " is played\n";
    if (sound_name.length() >= 4 && !sound_name.compare(sound_name.length() - 4,
			4, ".ogg")) {
		// It ends with .ogg, so we assume that it is Ogg audio.
		return ISoundDataOpenOgg::fromBuffer(sound_name, std::move(m_buffer));
	}
	// Other ending -> try openmpt
	try {
		// TODO: why does this fail?
		// /lua minetest.sound_play("secretly")
		// secretly.it in a sounds folder of a loaded mod
		return std::make_shared<ISoundDataOpenMod>(*m_buffer.cbegin(), *m_buffer.cend());
	} catch (const openmpt::exception &e) {
		std::cerr << "Cannot load \"" << sound_name << "\": " << e.what() << "\n";
	}
	return nullptr;
}

/*
 * SoundDataUnopenFile struct
 */

std::shared_ptr<ISoundDataOpen> SoundDataUnopenFile::open(const std::string &sound_name) &&
{
	// load from file at m_path

    if (sound_name.length() >= 4 && !sound_name.compare(sound_name.length() - 4,
			4, ".ogg")) {
		// It ends with .ogg, so we assume that it is Ogg audio.
		return ISoundDataOpenOgg::fromFile(sound_name, m_path);
	}

	// Other ending -> try openmpt
	try {
		return std::make_shared<ISoundDataOpenMod>(m_path);
	} catch (const openmpt::exception &e) {
		std::cerr << "Cannot load \"" << sound_name << "\": " << e.what() << "\n";
	}
	return nullptr;

}

} // namespace sound
