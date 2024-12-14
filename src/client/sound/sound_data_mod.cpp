// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 HybridDog

#include "sound_data.h"
#include "sound_data_mod.h"

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

ISoundDataOpenMod::ISoundDataOpenMod(const std::uint8_t &buf_begin, const std::uint8_t &buf_end) :
	m_mod{&buf_begin, &buf_end}
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

std::tuple<ALuint, ALuint, ALuint> ISoundDataOpenMod::getOrLoadBufferAt(ALuint offset)
{
	// TODO
	ALuint buf_size = 48000 * 3 * 2 * 4;
	if (buf_size <= offset)
		return {0, getLengthSamples(), 0};
	return {m_snd_buffer_id.get(), 48000 * 3 * 2 * 4, offset};
}

} // namespace sound
