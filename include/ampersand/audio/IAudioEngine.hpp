#pragma once

#include <ampersand/audio/Music.hpp>
#include <ampersand/audio/Sound.hpp>
#include <ampersand/audio/Voice.hpp>
#include <ampersand/core/math/Vec3.hpp>
#include <filesystem>

namespace ampersand::audio {

/**
 * @brief Abstract interface for all audio engine backends.
 *
 * Short sound effects are loaded once as a SoundId and can be played any
 * number of times at once; each `play` call starts an independent VoiceId
 * that can be controlled and queried on its own. Longer tracks are opened
 * as a MusicId and streamed from disk instead of being fully loaded.
 */
class IAudioEngine {
public:
    /**
     * @brief Destroys the audio engine instance.
     */
    virtual ~IAudioEngine() = default;

    IAudioEngine(const IAudioEngine&) = delete;
    IAudioEngine& operator=(const IAudioEngine&) = delete;

    /**
     * @brief Loads a sound effect from disk under a generated identifier.
     *
     * @param path File system path to the sound file.
     * @return Identifier assigned to the loaded sound, or 0 on failure.
     */
    [[nodiscard]] virtual SoundId loadSound(
        const std::filesystem::path& path) = 0;

    /**
     * @brief Starts a new instance of a loaded sound.
     *
     * @param sound Sound to play, as returned by loadSound.
     * @param volume Volume of the voice, from 0 (mute) to 100 (full).
     * @param pitch Pitch multiplier applied to the voice (1 is unchanged).
     * @param loop Whether the voice restarts from the beginning when it ends.
     * @return Identifier of the new voice, or 0 if the sound is unknown.
     */
    [[nodiscard]] virtual VoiceId play(SoundId sound, float volume, float pitch,
                                       bool loop) = 0;

    /** @brief Stops a voice and releases it. No-op if it does not exist. */
    virtual void stop(VoiceId voice) = 0;

    /** @brief Pauses a voice. No-op if it does not exist. */
    virtual void pause(VoiceId voice) = 0;

    /** @brief Resumes a paused voice. No-op if it does not exist. */
    virtual void resume(VoiceId voice) = 0;

    /**
     * @brief Checks whether a voice is currently playing.
     *
     * @param voice Voice to check.
     * @return true if it is playing, false if paused, stopped or unknown.
     */
    [[nodiscard]] virtual bool isPlaying(VoiceId voice) const = 0;

    /** @brief Changes the volume of a voice. No-op if it does not exist. */
    virtual void setVolume(VoiceId voice, float volume) = 0;

    /** @brief Changes the pitch of a voice. No-op if it does not exist. */
    virtual void setPitch(VoiceId voice, float pitch) = 0;

    /**
     * @brief Positions a voice in 3D space and enables its spatialization.
     *
     * No-op if the voice does not exist.
     */
    virtual void setPosition(VoiceId voice,
                             const core::math::Vec3f& position) = 0;

    /**
     * @brief Moves the listener that spatial voices are heard relative to.
     */
    virtual void setListenerPosition(const core::math::Vec3f& position) = 0;

    /**
     * @brief Changes the global volume multiplier applied to every voice
     * and music, on top of their own volume.
     *
     * @param volume Global volume, from 0 (mute) to 100 (unchanged).
     */
    virtual void setGlobalVolume(float volume) = 0;

    /**
     * @brief Opens a streamed music track under a generated identifier.
     *
     * @param path File system path to the music file.
     * @return Identifier assigned to the track, or 0 on failure.
     */
    [[nodiscard]] virtual MusicId openMusic(
        const std::filesystem::path& path) = 0;

    /**
     * @brief Starts or resumes a music track.
     *
     * @param music Track to play, as returned by openMusic.
     * @param volume Volume of the track, from 0 (mute) to 100 (full).
     * @param loop Whether the track restarts from the beginning when it ends.
     */
    virtual void playMusic(MusicId music, float volume, bool loop) = 0;

    /**
     * @brief Changes the volume of a music track without restarting it.
     *
     * No-op if the track does not exist.
     */
    virtual void setMusicVolume(MusicId music, float volume) = 0;

    /** @brief Pauses a music track. No-op if it does not exist. */
    virtual void pauseMusic(MusicId music) = 0;

    /** @brief Stops a music track and resets its position. */
    virtual void stopMusic(MusicId music) = 0;

    /**
     * @brief Checks whether a music track is currently playing.
     *
     * @param music Track to check.
     * @return true if it is playing, false if paused, stopped or unknown.
     */
    [[nodiscard]] virtual bool isMusicPlaying(MusicId music) const = 0;

protected:
    /**
     * @brief Creates a default audio engine instance.
     */
    IAudioEngine() = default;
};

}  // namespace ampersand::audio
