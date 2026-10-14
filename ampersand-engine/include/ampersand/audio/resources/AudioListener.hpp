#pragma once

#include <ampersand/audio/IAudioEngine.hpp>

namespace ampersand::audio {

/**
 * @brief The audio engine used by the World and the global volume applied to
 * every sound and music.
 *
 * Required by AudioSystem: `world.addResource<AudioListener>(engine)` once
 * (with an SfmlAudioEngine, for example), then set `volume` whenever the game
 * changes it (an options menu, for example).
 */
struct AudioListener {
    /** @brief Engine to load sounds and music with. Must outlive the World. */
    IAudioEngine& engine;
    /** @brief Global volume, from 0 (mute) to 100 (unchanged). */
    float volume{100.0F};
};

}  // namespace ampersand::audio
