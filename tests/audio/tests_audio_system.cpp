#include <gtest/gtest.h>

#include <ampersand/audio/IAudioEngine.hpp>
#include <ampersand/audio/components/AudioSource.hpp>
#include <ampersand/audio/resources/AudioListener.hpp>
#include <ampersand/audio/resources/SceneMusic.hpp>
#include <ampersand/audio/systems/AudioSystem.hpp>
#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <filesystem>
#include <set>

namespace {

using ampersand::audio::AudioListener;
using ampersand::audio::AudioSource;
using ampersand::audio::AudioSystem;
using ampersand::audio::IAudioEngine;
using ampersand::audio::MusicId;
using ampersand::audio::MusicRef;
using ampersand::audio::SceneMusic;
using ampersand::audio::SoundId;
using ampersand::audio::SoundRef;
using ampersand::audio::VoiceId;
using ampersand::core::ecs::Entity;
using ampersand::core::ecs::ResourceNotFound;
using ampersand::core::ecs::World;

/**
 * @brief Audio engine that plays nothing and records what it is asked, so the
 * AudioSystem can be tested without sound files or an audio device.
 */
class FakeAudioEngine final : public IAudioEngine {
public:
    FakeAudioEngine() = default;

    SoundId loadSound(const std::filesystem::path& /*path*/) override {
        return 1;
    }

    VoiceId play(SoundId sound, float volume, float /*pitch*/,
                 bool /*loop*/) override {
        if (sound == 0) {
            return 0;
        }
        playedVoices++;
        lastVolume = volume;
        playingVoices.insert(nextVoice);
        return nextVoice++;
    }

    void stop(VoiceId voice) override {
        playingVoices.erase(voice);
        stoppedVoices.insert(voice);
    }

    void pause(VoiceId /*voice*/) override {}
    void resume(VoiceId /*voice*/) override {}

    [[nodiscard]] bool isPlaying(VoiceId voice) const override {
        return playingVoices.contains(voice);
    }

    void setVolume(VoiceId /*voice*/, float volume) override {
        lastVolume = volume;
    }

    void setPitch(VoiceId /*voice*/, float /*pitch*/) override {}
    void setPosition(
        VoiceId /*voice*/,
        const ampersand::core::math::Vec3f& /*position*/) override {}
    void setListenerPosition(
        const ampersand::core::math::Vec3f& /*position*/) override {}

    void setGlobalVolume(float volume) override { globalVolume = volume; }

    MusicId openMusic(const std::filesystem::path& /*path*/) override {
        return 1;
    }

    void playMusic(MusicId music, float /*volume*/, bool /*loop*/) override {
        playingMusic.insert(music);
    }

    void setMusicVolume(MusicId /*music*/, float volume) override {
        lastVolume = volume;
    }

    void pauseMusic(MusicId /*music*/) override {}

    void stopMusic(MusicId music) override { playingMusic.erase(music); }

    [[nodiscard]] bool isMusicPlaying(MusicId music) const override {
        return playingMusic.contains(music);
    }

    /** @brief Ends a voice on its own, like a non-looping sound reaching its
     * end. */
    void finish(VoiceId voice) { playingVoices.erase(voice); }

    std::set<VoiceId> playingVoices;
    std::set<VoiceId> stoppedVoices;
    std::set<MusicId> playingMusic;
    VoiceId nextVoice{1};
    int playedVoices{0};
    float lastVolume{-1.0F};
    float globalVolume{-1.0F};
};

Entity createSource(World& world, AudioSource source) {
    const Entity entity = world.create();
    world.add(entity, source);
    return entity;
}

}  // namespace

TEST(AudioSystem, ThrowsWithoutAudioListener) {
    World world;

    EXPECT_THROW(AudioSystem{}.update(world), ResourceNotFound);
}

TEST(AudioSystem, AppliesTheGlobalVolumeAsIs) {
    FakeAudioEngine engine;
    World world;
    world.addResource<AudioListener>(engine).volume = 40.0F;

    AudioSystem{}.update(world);

    EXPECT_FLOAT_EQ(engine.globalVolume, 40.0F);
}

TEST(AudioSystem, StartsASoundOnce) {
    FakeAudioEngine engine;
    World world;
    world.addResource<AudioListener>(engine);
    createSource(world, {.source = SoundRef{1}, .playing = true});
    AudioSystem audio;

    audio.update(world);
    audio.update(world);

    EXPECT_EQ(engine.playedVoices, 1);
    EXPECT_TRUE(engine.isPlaying(1));
}

TEST(AudioSystem, SyncsTheVolumeWhilePlaying) {
    FakeAudioEngine engine;
    World world;
    world.addResource<AudioListener>(engine);
    const Entity ship =
        createSource(world, {.source = SoundRef{1}, .playing = true});
    AudioSystem audio;
    audio.update(world);

    world.get<AudioSource>(ship).volume = 30.0F;
    audio.update(world);

    EXPECT_FLOAT_EQ(engine.lastVolume, 30.0F);
}

TEST(AudioSystem, StopsWhenPlayingIsSetToFalse) {
    FakeAudioEngine engine;
    World world;
    world.addResource<AudioListener>(engine);
    const Entity ship =
        createSource(world, {.source = SoundRef{1}, .playing = true});
    AudioSystem audio;
    audio.update(world);

    world.get<AudioSource>(ship).playing = false;
    audio.update(world);

    EXPECT_FALSE(engine.isPlaying(1));
    EXPECT_TRUE(engine.stoppedVoices.contains(1));
}

TEST(AudioSystem, FinishedSoundResetsPlayingAndReleasesTheVoice) {
    FakeAudioEngine engine;
    World world;
    world.addResource<AudioListener>(engine);
    const Entity ship =
        createSource(world, {.source = SoundRef{1}, .playing = true});
    AudioSystem audio;
    audio.update(world);

    engine.finish(1);
    audio.update(world);

    EXPECT_FALSE(world.get<AudioSource>(ship).playing);
    EXPECT_TRUE(engine.stoppedVoices.contains(1));
}

TEST(AudioSystem, UnknownSoundResetsPlaying) {
    FakeAudioEngine engine;
    World world;
    world.addResource<AudioListener>(engine);
    const Entity ship =
        createSource(world, {.source = SoundRef{0}, .playing = true});
    AudioSystem audio;

    audio.update(world);
    audio.update(world);

    EXPECT_FALSE(world.get<AudioSource>(ship).playing);
}

TEST(AudioSystem, ChangingTheSoundStopsTheOldVoice) {
    FakeAudioEngine engine;
    World world;
    world.addResource<AudioListener>(engine);
    const Entity ship =
        createSource(world, {.source = SoundRef{1}, .playing = true});
    AudioSystem audio;
    audio.update(world);

    world.get<AudioSource>(ship).source = SoundRef{2};
    audio.update(world);

    EXPECT_FALSE(engine.isPlaying(1));
    EXPECT_TRUE(engine.isPlaying(2));
}

TEST(AudioSystem, DestroyedEntityStopsItsSound) {
    FakeAudioEngine engine;
    World world;
    world.addResource<AudioListener>(engine);
    const Entity ship =
        createSource(world, {.source = SoundRef{1}, .playing = true});
    AudioSystem audio;
    audio.update(world);

    world.destroy(ship);
    audio.update(world);

    EXPECT_FALSE(engine.isPlaying(1));
}

TEST(AudioSystem, RemovedSourceStopsItsMusic) {
    FakeAudioEngine engine;
    World world;
    world.addResource<AudioListener>(engine);
    const Entity boss = createSource(
        world, {.source = MusicRef{3}, .loop = true, .playing = true});
    AudioSystem audio;
    audio.update(world);
    ASSERT_TRUE(engine.isMusicPlaying(3));

    world.remove<AudioSource>(boss);
    audio.update(world);

    EXPECT_FALSE(engine.isMusicPlaying(3));
}

TEST(AudioSystem, SceneMusicStartsAndStops) {
    FakeAudioEngine engine;
    World world;
    world.addResource<AudioListener>(engine);
    world.addResource<SceneMusic>() = {.music = 1, .playing = true};
    AudioSystem audio;

    audio.update(world);
    EXPECT_TRUE(engine.isMusicPlaying(1));

    world.getResource<SceneMusic>().playing = false;
    audio.update(world);
    EXPECT_FALSE(engine.isMusicPlaying(1));
}

TEST(AudioSystem, ChangingTheSceneMusicStopsTheOldTrack) {
    FakeAudioEngine engine;
    World world;
    world.addResource<AudioListener>(engine);
    world.addResource<SceneMusic>() = {.music = 1, .playing = true};
    AudioSystem audio;
    audio.update(world);

    world.getResource<SceneMusic>().music = 2;
    audio.update(world);

    EXPECT_FALSE(engine.isMusicPlaying(1));
    EXPECT_TRUE(engine.isMusicPlaying(2));
    EXPECT_TRUE(world.getResource<SceneMusic>().playing);
}

TEST(AudioSystem, RemovingTheSceneMusicStopsIt) {
    FakeAudioEngine engine;
    World world;
    world.addResource<AudioListener>(engine);
    world.addResource<SceneMusic>() = {.music = 1, .playing = true};
    AudioSystem audio;
    audio.update(world);

    world.removeResource<SceneMusic>();
    audio.update(world);

    EXPECT_FALSE(engine.isMusicPlaying(1));
}

TEST(AudioSystem, FinishedSceneMusicResetsPlaying) {
    FakeAudioEngine engine;
    World world;
    world.addResource<AudioListener>(engine);
    world.addResource<SceneMusic>() = {
        .music = 1,
        .loop = false,
        .playing = true,
    };
    AudioSystem audio;
    audio.update(world);

    engine.stopMusic(1);
    audio.update(world);

    EXPECT_FALSE(world.getResource<SceneMusic>().playing);
}
