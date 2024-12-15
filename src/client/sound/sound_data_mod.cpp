// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 HybridDog

#include "sound_data.h"
#include "sound_data_mod.h"
#include "sound_constants.h"

#include <fstream>
#include <alext.h>


namespace {

std::vector<u8> read_file(const std::string &path)
{
	std::ifstream infile{path, std::ios::binary | std::ifstream::ate};
	if (!infile)
		throw std::runtime_error("Could not load file: " + path);
	auto size = infile.tellg();
	std::vector<u8> data;
	data.resize(size);
	infile.seekg(0);
	if (!infile.read(reinterpret_cast<char*>(data.data()), size))
		throw std::runtime_error("Could not load file contents from: " + path);
	return data;
}

}

namespace sound {

ISoundDataOpenMod::ISoundDataOpenMod(const std::string &path) :
	m_mod{read_file(path)}
{
	// TODO: loading data this way is just temporary and should happen in
	// getOrLoadBufferAt
	std::unique_ptr<f32[]> snd_buffer{new f32[48000 * 3 * 2]};
	m_mod.read_interleaved_stereo(48000, 48000 * 3, snd_buffer.get());

	m_snd_buffer_id = RAIIALSoundBuffer::generate();
	alBufferData(m_snd_buffer_id.get(), AL_FORMAT_STEREO_FLOAT32, static_cast<void *>(snd_buffer.get()), 48000 * 3 * 2 * 4, 48000);
	ALenum error = alGetError();
	if (error != AL_NO_ERROR) {
		warningstream << "Audio: OpenAL error: " << getAlErrorString(error)
				<< "preparing sound buffer for sound" << std::endl;
	}
}

//~ ISoundDataOpenMod::ISoundDataOpenMod(const std::uint8_t &buf_begin, const std::uint8_t &buf_end) :
ISoundDataOpenMod::ISoundDataOpenMod(const std::vector<uint8_t> &buf) :
	m_mod{buf}
{
}

// TODO: what if the same sound is played multiple times simultaneously with different offsets?
std::tuple<ALuint, ALuint, ALuint> ISoundDataOpenMod::getOrLoadBufferAt(ALuint offset)
{
	warningstream << "Audio: offset: " << offset
			<< ", m_mod_bytepos: " << m_mod_bytepos << std::endl;

	constexpr ALuint num_required_bytes = static_cast<ALuint>(48000 * MIN_STREAM_BUFFER_LENGTH) * 4 * 2;
	// TODO: can we omit this buffer to reduce memory usage?
	std::unique_ptr<u8[]> snd_buffer{new u8[num_required_bytes]};
	if (offset == 0 && m_mod_bytepos > 0) {
		// seek to the beginning
		double secs_now = m_mod.set_position_seconds(0);
		m_mod_bytepos = 0;
	}
	if (offset == m_mod_bytepos) {
		// offset ist the position where m_mod is currently at.
		f32 *myptr = reinterpret_cast<f32 *>(snd_buffer.get());
		m_mod.read_interleaved_stereo(48000, num_required_bytes / (4 * 2), myptr);
		m_mod_bytepos += offset + num_required_bytes;

		//~ m_snd_buffer_id = RAIIALSoundBuffer::generate();
		alBufferData(m_snd_buffer_id.get(), AL_FORMAT_STEREO_FLOAT32, static_cast<void *>(snd_buffer.get()), num_required_bytes, 48000);
		ALenum error = alGetError();
		if (error != AL_NO_ERROR) {
			warningstream << "Audio: OpenAL error: " << getAlErrorString(error)
					<< "preparing sound buffer for sound" << std::endl;
		}
		return {m_snd_buffer_id.get(), offset + num_required_bytes, 0};
	}

	return {0, getLengthSamples(), 0};
}

} // namespace sound
