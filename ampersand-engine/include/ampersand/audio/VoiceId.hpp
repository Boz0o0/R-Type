#pragma once

#include <cstdint>

namespace ampersand::audio {

/**
 * @brief Identifier assigned to a single playing instance of a sound.
 *
 * A SoundId names audio data; a VoiceId names one active playback of it,
 * since the same sound can be played several times at once.
 */
using VoiceId = std::uint32_t;

}  // namespace ampersand::audio
