// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 HybridDog

#pragma once

#include "sound_data.h"

// TODO: is it possible to use a forward declaration for openmpt::module?
#include <libopenmpt/libopenmpt.hpp>


namespace sound {

struct ISoundDataOpenMod final : ISoundDataOpen
{
	explicit ISoundDataOpenMod(const std::string &path);
	explicit ISoundDataOpenMod(const u8 &buf_begin, const u8 &buf_end);

	virtual bool isStereo() const noexcept override {
		return true; }
	virtual f32 getLengthSeconds() const noexcept override {
		return 3.0f; }
	virtual ALuint getLengthSamples() const noexcept override {
		return 48000 * 3; }
	virtual const std::string &getNameForLogging() const noexcept override {
		static std::string name = "tmp";
		return name; }
	bool isStreaming() const noexcept override { return true; }
	std::tuple<ALuint, ALuint, ALuint> getOrLoadBufferAt(ALuint offset) override;

	DISABLE_CLASS_COPY(ISoundDataOpenMod);

private:
	openmpt::module m_mod;
	RAIIALSoundBuffer m_snd_buffer_id;
};

} // namespace sound
