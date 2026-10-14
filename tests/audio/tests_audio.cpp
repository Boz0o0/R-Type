#include <gtest/gtest.h>

#include <ampersand/audio/SfmlAudioEngine.hpp>
#include <ampersand/audio/components/AudioSource.hpp>
#include <ampersand/audio/resources/AudioListener.hpp>
#include <variant>

TEST(AudioModule, MissingAssetsReturnZeroIds) {
    ampersand::audio::SfmlAudioEngine engine;

    EXPECT_EQ(engine.loadSound("/definitely/not/a/real/sound.wav"), 0);
    EXPECT_EQ(engine.openMusic("/definitely/not/a/real/music.ogg"), 0);
}

TEST(AudioModule, PlayingAnUnknownSoundReturnsZero) {
    ampersand::audio::SfmlAudioEngine engine;

    EXPECT_EQ(engine.play(1, 100.0F, 1.0F, false), 0);
}

TEST(AudioModule, UnknownVoicesAreNoOpAndNeverPlaying) {
    ampersand::audio::SfmlAudioEngine engine;

    EXPECT_FALSE(engine.isPlaying(1));
    EXPECT_NO_THROW(engine.stop(1));
    EXPECT_NO_THROW(engine.pause(1));
    EXPECT_NO_THROW(engine.resume(1));
    EXPECT_NO_THROW(engine.setVolume(1, 50.0F));
    EXPECT_NO_THROW(engine.setPitch(1, 2.0F));
    EXPECT_NO_THROW(engine.setPosition(1, {1.0F, 2.0F, 3.0F}));
}

TEST(AudioModule, UnknownMusicIsNoOpAndNeverPlaying) {
    ampersand::audio::SfmlAudioEngine engine;

    EXPECT_FALSE(engine.isMusicPlaying(1));
    EXPECT_NO_THROW(engine.playMusic(1, 100.0F, true));
    EXPECT_NO_THROW(engine.pauseMusic(1));
    EXPECT_NO_THROW(engine.stopMusic(1));
}

TEST(AudioModule, ListenerPositionCanBeSetWithoutAnyVoice) {
    ampersand::audio::SfmlAudioEngine engine;

    EXPECT_NO_THROW(engine.setListenerPosition({0.0F, 0.0F, 0.0F}));
}

TEST(AudioSourceComponent, DefaultsToAudibleNonLoopingSilence) {
    const ampersand::audio::AudioSource source;

    ASSERT_TRUE(
        std::holds_alternative<ampersand::audio::SoundRef>(source.source));
    EXPECT_EQ(std::get<ampersand::audio::SoundRef>(source.source).id, 0U);
    EXPECT_FLOAT_EQ(source.volume, 100.0F);
    EXPECT_FALSE(source.loop);
    EXPECT_FALSE(source.playing);
}

TEST(AudioListenerResource, DefaultsToFullVolume) {
    ampersand::audio::SfmlAudioEngine engine;
    const ampersand::audio::AudioListener listener{
        .engine = engine,
    };

    EXPECT_FLOAT_EQ(listener.volume, 100.0F);
}
