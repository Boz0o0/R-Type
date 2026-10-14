#include <ampersand/audio/components/AudioSource.hpp>
#include <ampersand/audio/resources/AudioListener.hpp>
#include <ampersand/audio/resources/SceneMusic.hpp>
#include <ampersand/audio/systems/AudioSystem.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <variant>

namespace ampersand::audio {

void AudioSystem::update(core::ecs::World& world) {
    const auto& listener = world.getResource<AudioListener>();
    IAudioEngine& engine = listener.engine;
    engine.setGlobalVolume(listener.volume);

    this->stopOrphanedSounds(world, engine);

    for (const core::ecs::Entity entity : world.owners<AudioSource>()) {
        auto& source = world.get<AudioSource>(entity);
        if (std::holds_alternative<SoundRef>(source.source)) {
            this->updateSoundSource(engine, entity, source);
        } else {
            this->updateMusicSource(engine, entity, source);
        }
    }

    this->updateSceneMusic(world, engine);
}

void AudioSystem::stopOrphanedSounds(core::ecs::World& world,
                                     IAudioEngine& engine) {
    for (auto voiceIt = this->activeVoices_.begin();
         voiceIt != this->activeVoices_.end();) {
        if (world.alive(voiceIt->first) &&
            world.has<AudioSource>(voiceIt->first)) {
            ++voiceIt;
        } else {
            engine.stop(voiceIt->second.second);
            voiceIt = this->activeVoices_.erase(voiceIt);
        }
    }

    for (auto musicIt = this->activeMusic_.begin();
         musicIt != this->activeMusic_.end();) {
        if (world.alive(musicIt->first) &&
            world.has<AudioSource>(musicIt->first)) {
            ++musicIt;
        } else {
            engine.stopMusic(musicIt->second);
            musicIt = this->activeMusic_.erase(musicIt);
        }
    }
}

void AudioSystem::updateSoundSource(IAudioEngine& engine,
                                    core::ecs::Entity entity,
                                    AudioSource& source) {
    const auto sound = std::get<SoundRef>(source.source).id;
    const auto voiceIt = this->activeVoices_.find(entity);
    const bool tracksThisSound =
        voiceIt != this->activeVoices_.end() && voiceIt->second.first == sound;

    if (source.playing) {
        if (!tracksThisSound) {
            if (voiceIt != this->activeVoices_.end()) {
                engine.stop(voiceIt->second.second);  // retargeted mid-play
            }
            const VoiceId voice =
                engine.play(sound, source.volume, 1.0F, source.loop);
            this->activeVoices_[entity] = {sound, voice};
        } else if (engine.isPlaying(voiceIt->second.second)) {
            engine.setVolume(voiceIt->second.second, source.volume);
        } else {
            engine.stop(voiceIt->second.second);  // releases the finished voice
            this->activeVoices_.erase(voiceIt);
            source.playing = false;
        }
    } else if (voiceIt != this->activeVoices_.end()) {
        engine.stop(voiceIt->second.second);
        this->activeVoices_.erase(voiceIt);
    }
}

void AudioSystem::updateMusicSource(IAudioEngine& engine,
                                    core::ecs::Entity entity,
                                    AudioSource& source) {
    const auto music = std::get<MusicRef>(source.source).id;
    const auto musicIt = this->activeMusic_.find(entity);
    const bool tracksThisMusic =
        musicIt != this->activeMusic_.end() && musicIt->second == music;

    if (source.playing) {
        if (!tracksThisMusic) {
            if (musicIt != this->activeMusic_.end()) {
                engine.stopMusic(musicIt->second);  // retargeted mid-play
            }
            engine.playMusic(music, source.volume, source.loop);
            this->activeMusic_[entity] = music;
        } else if (engine.isMusicPlaying(music)) {
            engine.setMusicVolume(music, source.volume);
        } else {
            this->activeMusic_.erase(musicIt);
            source.playing = false;
        }
    } else if (musicIt != this->activeMusic_.end()) {
        engine.stopMusic(musicIt->second);
        this->activeMusic_.erase(musicIt);
    }
}

void AudioSystem::updateSceneMusic(core::ecs::World& world,
                                   IAudioEngine& engine) {
    const bool wanted = world.hasResource<SceneMusic>() &&
                        world.getResource<SceneMusic>().playing;
    if (!wanted) {
        if (this->sceneMusic_) {
            engine.stopMusic(*this->sceneMusic_);
            this->sceneMusic_.reset();
        }
        return;
    }

    auto& scene = world.getResource<SceneMusic>();
    if (this->sceneMusic_ != scene.music) {
        if (this->sceneMusic_) {
            engine.stopMusic(*this->sceneMusic_);
        }
        engine.playMusic(scene.music, scene.volume, scene.loop);
        this->sceneMusic_ = scene.music;
    } else if (engine.isMusicPlaying(scene.music)) {
        engine.setMusicVolume(scene.music, scene.volume);
    } else {
        this->sceneMusic_.reset();
        scene.playing = false;
    }
}

}  // namespace ampersand::audio
