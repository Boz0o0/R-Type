#pragma once

#include <ampersand/audio/MusicId.hpp>
#include <ampersand/audio/SoundId.hpp>
#include <variant>

namespace ampersand::audio {

/**
 * @brief Tags a SoundId inside AudioSource::source.
 *
 * SoundId and MusicId are both plain `std::uint32_t`, so a bare
 * `std::variant<SoundId, MusicId>` would have two indistinguishable
 * alternatives. This wrapper (and MusicRef) gives the variant two distinct
 * types instead.
 */
struct SoundRef {
    SoundId id{};
};

/**
 * @brief Tags a MusicId inside AudioSource::source (see SoundRef).
 *
 * A MusicId is a single stream: only one source (AudioSource or SceneMusic)
 * can play it at a time, or they stop and restart each other.
 */
struct MusicRef {
    MusicId id{};
};

/**
 * @brief Sound or music an entity plays, driven by the AudioSystem.
 *
 * A plain data component: attach it with `world.add`. `source` is either a
 * `SoundRef` (a short effect, played through its own voice) or a
 * `MusicRef` (a streamed track); the AudioSystem starts, stops and
 * live-syncs the volume of whichever it holds.
 *
 * Set `playing` to `true` to start it and `false` to stop it. The
 * AudioSystem resets `playing` to `false` on its own once a non-looping
 * sound or track reaches its end, or right away if the id is unknown (a
 * failed load returns 0). An entity plays one voice at a time: setting
 * `playing` again while it plays does not start a second one.
 */
struct AudioSource {
    std::variant<SoundRef, MusicRef> source{SoundRef{}};
    float volume{100.0F};
    bool loop{false};
    bool playing{false};
};

}  // namespace ampersand::audio
