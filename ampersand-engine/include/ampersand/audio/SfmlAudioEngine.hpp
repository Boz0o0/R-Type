#pragma once

#include <SFML/Audio.hpp>
#include <ampersand/audio/IAudioEngine.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

namespace ampersand::audio {

/**
 * @brief SFML-backed implementation of the audio engine interface.
 *
 * Sound effects are decoded once into a `sf::SoundBuffer` per SoundId; each
 * `play` call creates its own `sf::Sound` voice bound to that buffer, so
 * the same sound can be heard several times at once. Music tracks are
 * streamed from disk through `sf::Music` instead.
 */
class SfmlAudioEngine final : public IAudioEngine {
public:
    [[nodiscard]] SoundId loadSound(const std::filesystem::path& path) override;

    [[nodiscard]] VoiceId play(SoundId sound, float volume, float pitch,
                               bool loop) override;
    void stop(VoiceId voice) override;
    void pause(VoiceId voice) override;
    void resume(VoiceId voice) override;
    [[nodiscard]] bool isPlaying(VoiceId voice) const override;
    void setVolume(VoiceId voice, float volume) override;
    void setPitch(VoiceId voice, float pitch) override;
    void setPosition(VoiceId voice, const core::math::Vec3f& position) override;

    void setListenerPosition(const core::math::Vec3f& position) override;
    void setGlobalVolume(float volume) override;

    [[nodiscard]] MusicId openMusic(const std::filesystem::path& path) override;
    void playMusic(MusicId music, float volume, bool loop) override;
    void setMusicVolume(MusicId music, float volume) override;
    void pauseMusic(MusicId music) override;
    void stopMusic(MusicId music) override;
    [[nodiscard]] bool isMusicPlaying(MusicId music) const override;

private:
    /**
     * @brief Drops voices that reached their end on their own (non-looping
     * sounds), so finished one-shots do not accumulate.
     */
    void pruneFinishedVoices();

    /** @brief Finds an active voice, or nullopt if it does not exist. */
    [[nodiscard]] std::optional<std::reference_wrapper<sf::Sound>> findVoice(
        VoiceId voice);

    /** @brief Finds an opened music track, or nullopt if it does not exist. */
    [[nodiscard]] std::optional<std::reference_wrapper<sf::Music>> findMusic(
        MusicId music);

    std::unordered_map<SoundId, std::unique_ptr<sf::SoundBuffer>> buffers_;
    std::unordered_map<VoiceId, std::unique_ptr<sf::Sound>> voices_;
    std::unordered_map<MusicId, std::unique_ptr<sf::Music>> music_;
    SoundId nextSoundId_{1};
    VoiceId nextVoiceId_{1};
    MusicId nextMusicId_{1};
};

}  // namespace ampersand::audio
