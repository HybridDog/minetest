// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 HybridDog

#pragma once

//~ #include "al_helpers.h"

//~ #include <memory>
//~ #include <tuple>
//~ #include <optional>
//~ #include <string>

namespace sound {

/**
 * TODO
 */
class SoundDataReader
{
	ISoundDataOpen &m_sound_data_open;

public:
	SoundDataReader(ISoundDataOpen &sound_data_open, bool looping, f32 start_time) :
		m_sound_data_open{m_sound_data_open},
		m_looping{looping},
		m_sample_pos_prev{static_cast<Aluint>(start_time * 48000)}
	{};

	std::tuple<ALuint, ALuint, ALuint> getNextBuffer()
	{
		auto [buf, buf_end, offset_in_buf] = m_sound_data_open.getOrLoadBufferAt(m_sample_pos_prev);
		m_sample_pos_prev = buf_end;

	}



	/**
	 * The audio data can either be mono or stereo.
	 * @return Whether it's stereo data. TODO: is it interleaved stereo?
	 */
	virtual bool isStereo() const noexcept = 0;

	/**
	 * @return TODO
	 */
	virtual f32 getLengthSeconds() const noexcept = 0;

	/**
	 * @return TODO
	 */
	virtual ALuint getLengthSamples() const noexcept = 0;

	/**
	 * @return TODO
	 */
	virtual const std::string &getNameForLogging() const noexcept = 0;

	/**
	 * Load a buffer containing data starting at the given offset. Or just get it
	 * if it was already loaded.
	 *
	 * This function returns multiple values:
	 * * `buffer`: The OpenAL buffer.
	 * * `buffer_end`: The offset (in the file) where `buffer` ends (exclusive).
	 * * `offset_in_buffer`: Offset relative to `buffer`'s start where the requested
	 *       `offset` is.
	 *       `offset_in_buffer == 0` is guaranteed if some loaded buffer ends at
	 *       `offset`.
	 *
	 * @param offset The start of the buffer.
	 * @return `{buffer, buffer_end, offset_in_buffer}` or `{0, sound_data_end, 0}`
	 *         if `offset` is invalid.
	 */
	virtual std::tuple<ALuint, ALuint, ALuint> getOrLoadBufferAt(ALuint offset) = 0;
};


} // namespace sound
