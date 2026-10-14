#pragma once

#include <ampersand/audio/IAudioEngine.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/ISystem.hpp>
#include <optional>
#include <unordered_map>
#include <utility>

namespace ampersand::audio {

struct AudioSource;

/**
 * @brief Starts, stops and live-syncs every AudioSource and SceneMusic.
 *
 * Owns no audio data itself: it only drives the IAudioEngine of the
 * AudioListener resource, following the `playing` flag of each AudioSource
 * and of the SceneMusic resource (if the World has one). Remembers which entity
 * started which voice or music track between frames, so it can tell a
 * fresh `playing = true` from one that is already running, and stops the
 * sound of any entity that no longer has an AudioSource (destroyed, or the
 * component removed).
 */
class AudioSystem final : public core::ecs::ISystem {
public:
    /**
     * @brief Applies the global volume, then starts, stops or live-syncs
     * every AudioSource and the SceneMusic resource.
     *
     * @param world World holding the entities and the AudioListener
     * resource, with its engine (and the SceneMusic resource, if any).
     * @throws ResourceNotFound if the World has no AudioListener.
     * @throws std::invalid_argument if a volume is not finite.
     */
    void update(core::ecs::World& world) override;

private:
    /**
     * @brief Stops the voice or music of every entity that no longer has an
     * AudioSource, alive or not.
     */
    void stopOrphanedSounds(core::ecs::World& world, IAudioEngine& engine);

    /** @brief Starts, stops or live-syncs the voice of one entity's SFX. */
    void updateSoundSource(IAudioEngine& engine, core::ecs::Entity entity,
                           AudioSource& source);

    /** @brief Starts, stops or live-syncs the music of one entity's source. */
    void updateMusicSource(IAudioEngine& engine, core::ecs::Entity entity,
                           AudioSource& source);

    /**
     * @brief Starts, stops or live-syncs the SceneMusic, and stops the track
     * it started if the resource changed its music or was removed.
     */
    void updateSceneMusic(core::ecs::World& world, IAudioEngine& engine);

    std::unordered_map<core::ecs::Entity, std::pair<SoundId, VoiceId>>
        activeVoices_;
    std::unordered_map<core::ecs::Entity, MusicId> activeMusic_;
    std::optional<MusicId> sceneMusic_;
};

}  // namespace ampersand::audio
