#pragma once

#include <ampersand/audio/MusicId.hpp>

namespace ampersand::audio {

/**
 * @brief Background music for the whole scene, belonging to no entity.
 *
 * Optional: AudioSystem only drives it if the World has one. Set `music` to
 * a track opened with `IAudioEngine::openMusic`, and `playing` to `true` to
 * start it (`false` to stop it), the same way as AudioSource.
 */
struct SceneMusic {
    MusicId music{};
    float volume{100.0F};
    bool loop{true};
    bool playing{false};
};

}  // namespace ampersand::audio
