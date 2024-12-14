// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 DS
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2011 Sebastian 'Bahamada' Rühl
// Copyright (C) 2011 Cyriaque 'Cisoun' Skrapits <cysoun@gmail.com>
// Copyright (C) 2011 Giuseppe Bilotta <giuseppe.bilotta@gmail.com>

#pragma once

#include "al_helpers.h"

#include <memory>
#include <tuple>
#include <optional>
#include <string>

namespace sound {

/**
 * Stores sound pcm data buffers.
 */
struct ISoundDataOpen
{
	virtual ~ISoundDataOpen() = default;

	/**
	 * Iff the data is streaming, there is more than one buffer.
	 * @return Whether it's streaming data.
	 */
	virtual bool isStreaming() const noexcept = 0;

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


/**
 * Will be opened lazily when first used.
 */
struct ISoundDataUnopen
{
	virtual ~ISoundDataUnopen() = default;

	// Note: The ISoundDataUnopen is moved (see &&). It is not meant to be kept
	// after opening.
	virtual std::shared_ptr<ISoundDataOpen> open(const std::string &sound_name) && = 0;
};

/**
 * Sound file is in a memory buffer.
 */
struct SoundDataUnopenBuffer final : ISoundDataUnopen
{
	std::string m_buffer;

	explicit SoundDataUnopenBuffer(std::string &&buffer) : m_buffer(std::move(buffer)) {}

	std::shared_ptr<ISoundDataOpen> open(const std::string &sound_name) && override;
};

/**
 * Sound file is in file system.
 */
struct SoundDataUnopenFile final : ISoundDataUnopen
{
	std::string m_path;

	explicit SoundDataUnopenFile(const std::string &path) : m_path(path) {}

	std::shared_ptr<ISoundDataOpen> open(const std::string &sound_name) && override;
};

} // namespace sound
