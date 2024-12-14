// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 HybridDog
// TODO

#pragma once

#include "sound_data.h"
#include "ogg_file.h"


namespace sound {

struct ISoundDataOpenOgg : ISoundDataOpen
{
	OggFileDecodeInfo m_decode_info;

	explicit ISoundDataOpenOgg(const OggFileDecodeInfo &decode_info) :
			m_decode_info(decode_info) {}

	static std::shared_ptr<ISoundDataOpenOgg> fromFile(
		const std::string &sound_name, const std::string &path);

	static std::shared_ptr<ISoundDataOpenOgg> fromBuffer(
		const std::string &sound_name, std::string buffer);

	static std::shared_ptr<ISoundDataOpenOgg> fromOggFile(std::unique_ptr<RAIIOggFile> oggfile,
		const std::string &filename_for_logging);

	virtual bool isStereo() const noexcept override {
		return m_decode_info.is_stereo; }
	virtual f32 getLengthSeconds() const noexcept override {
		return m_decode_info.length_seconds; }
	virtual ALuint getLengthSamples() const noexcept override {
		return m_decode_info.length_samples; }
	virtual const std::string &getNameForLogging() const noexcept override {
		return m_decode_info.name_for_logging; }
};

/**
 * Non-streaming opened sound data.
 * All data is completely loaded in one buffer.
 */
struct SoundDataOpenBufferOgg final : ISoundDataOpenOgg
{
	RAIIALSoundBuffer m_buffer;

	SoundDataOpenBufferOgg(std::unique_ptr<RAIIOggFile> oggfile,
			const OggFileDecodeInfo &decode_info);

	bool isStreaming() const noexcept override { return false; }

	std::tuple<ALuint, ALuint, ALuint> getOrLoadBufferAt(ALuint offset) override
	{
		if (offset >= m_decode_info.length_samples)
			return {0, m_decode_info.length_samples, 0};
		return {m_buffer.get(), m_decode_info.length_samples, offset};
	}
};

/**
 * Streaming opened sound data.
 *
 * Uses a sorted list of contiguous sound data regions (`ContiguousBuffers`s) for
 * efficient seeking.
 */
struct SoundDataOpenStreamOgg final : ISoundDataOpenOgg
{
	/**
	 * An OpenAL buffer that goes until `m_end` (exclusive).
	 */
	struct SoundBufferUntil final
	{
		ALuint m_end;
		RAIIALSoundBuffer m_buffer;
	};

	/**
	 * A sorted non-empty vector of contiguous buffers.
	 * The start (inclusive) of each buffer is the end of its predecessor, or
	 * `m_start` for the first buffer.
	 */
	struct ContiguousBuffers final
	{
		ALuint m_start;
		std::vector<SoundBufferUntil> m_buffers;
	};

	std::unique_ptr<RAIIOggFile> m_oggfile;
	// A sorted vector of non-overlapping, non-contiguous `ContiguousBuffers`s.
	std::vector<ContiguousBuffers> m_bufferss;

	SoundDataOpenStreamOgg(std::unique_ptr<RAIIOggFile> oggfile,
			const OggFileDecodeInfo &decode_info);

	bool isStreaming() const noexcept override { return true; }

	std::tuple<ALuint, ALuint, ALuint> getOrLoadBufferAt(ALuint offset) override;

private:
	// offset must be before after_it's m_start and after (after_it-1)'s last m_end
	// new buffer will be inserted into m_bufferss before after_it
	// returns same as getOrLoadBufferAt
	std::tuple<ALuint, ALuint, ALuint> loadBufferAt(ALuint offset,
			std::vector<ContiguousBuffers>::iterator after_it);
};

} // namespace sound
