#include <ampersand/audio/SfmlAudioEngine.hpp>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace ampersand::audio {

SoundId SfmlAudioEngine::loadSound(const std::filesystem::path& path) {
    auto buffer = std::make_unique<sf::SoundBuffer>();
    if (!buffer->loadFromFile(path)) return 0;

    const SoundId soundId = this->nextSoundId_++;
    this->buffers_.emplace(soundId, std::move(buffer));
    return soundId;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
VoiceId SfmlAudioEngine::play(SoundId sound, float volume, float pitch,
                              bool loop) {
    if (!std::isfinite(volume) || !std::isfinite(pitch)) {
        throw std::invalid_argument("Voice volume and pitch must be finite");
    }

    this->pruneFinishedVoices();

    const auto buffer = this->buffers_.find(sound);
    if (buffer == this->buffers_.end()) return 0;

    auto voice = std::make_unique<sf::Sound>(*buffer->second);
    voice->setVolume(volume);
    voice->setPitch(pitch);
    voice->setLooping(loop);
    voice->play();

    const VoiceId voiceId = this->nextVoiceId_++;
    this->voices_.emplace(voiceId, std::move(voice));
    return voiceId;
}

void SfmlAudioEngine::stop(VoiceId voice) {
    if (const auto sound = this->findVoice(voice)) {
        sound->get().stop();
        this->voices_.erase(voice);
    }
}

void SfmlAudioEngine::pause(VoiceId voice) {
    if (const auto sound = this->findVoice(voice)) sound->get().pause();
}

void SfmlAudioEngine::resume(VoiceId voice) {
    if (const auto sound = this->findVoice(voice)) sound->get().play();
}

bool SfmlAudioEngine::isPlaying(VoiceId voice) const {
    const auto voiceIt = this->voices_.find(voice);
    return voiceIt != this->voices_.end() &&
           voiceIt->second->getStatus() == sf::Sound::Status::Playing;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void SfmlAudioEngine::setVolume(VoiceId voice, float volume) {
    if (!std::isfinite(volume)) {
        throw std::invalid_argument("Voice volume must be finite");
    }
    if (const auto sound = this->findVoice(voice)) {
        sound->get().setVolume(volume);
    }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void SfmlAudioEngine::setPitch(VoiceId voice, float pitch) {
    if (!std::isfinite(pitch)) {
        throw std::invalid_argument("Voice pitch must be finite");
    }
    if (const auto sound = this->findVoice(voice)) {
        sound->get().setPitch(pitch);
    }
}

void SfmlAudioEngine::setPosition(VoiceId voice,
                                  const core::math::Vec3f& position) {
    if (const auto sound = this->findVoice(voice)) {
        sound->get().setSpatializationEnabled(true);
        sound->get().setPosition({position.x, position.y, position.z});
    }
}

void SfmlAudioEngine::setListenerPosition(const core::math::Vec3f& position) {
    sf::Listener::setPosition({position.x, position.y, position.z});
}

void SfmlAudioEngine::setGlobalVolume(float volume) {
    if (!std::isfinite(volume)) {
        throw std::invalid_argument("Global volume must be finite");
    }
    sf::Listener::setGlobalVolume(volume);
}

MusicId SfmlAudioEngine::openMusic(const std::filesystem::path& path) {
    auto music = std::make_unique<sf::Music>();
    if (!music->openFromFile(path)) return 0;

    const MusicId musicId = this->nextMusicId_++;
    this->music_.emplace(musicId, std::move(music));
    return musicId;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void SfmlAudioEngine::playMusic(MusicId music, float volume, bool loop) {
    if (!std::isfinite(volume)) {
        throw std::invalid_argument("Music volume must be finite");
    }
    if (const auto track = this->findMusic(music)) {
        track->get().setVolume(volume);
        track->get().setLooping(loop);
        track->get().play();
    }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void SfmlAudioEngine::setMusicVolume(MusicId music, float volume) {
    if (!std::isfinite(volume)) {
        throw std::invalid_argument("Music volume must be finite");
    }
    if (const auto track = this->findMusic(music))
        track->get().setVolume(volume);
}

void SfmlAudioEngine::pauseMusic(MusicId music) {
    if (const auto track = this->findMusic(music)) track->get().pause();
}

void SfmlAudioEngine::stopMusic(MusicId music) {
    if (const auto track = this->findMusic(music)) track->get().stop();
}

bool SfmlAudioEngine::isMusicPlaying(MusicId music) const {
    const auto musicIt = this->music_.find(music);
    return musicIt != this->music_.end() &&
           musicIt->second->getStatus() == sf::Music::Status::Playing;
}

void SfmlAudioEngine::pruneFinishedVoices() {
    for (auto voiceIt = this->voices_.begin();
         voiceIt != this->voices_.end();) {
        if (voiceIt->second->getStatus() == sf::Sound::Status::Stopped) {
            voiceIt = this->voices_.erase(voiceIt);
        } else {
            ++voiceIt;
        }
    }
}

std::optional<std::reference_wrapper<sf::Sound>> SfmlAudioEngine::findVoice(
    VoiceId voice) {
    const auto voiceIt = this->voices_.find(voice);
    if (voiceIt == this->voices_.end()) return std::nullopt;
    return *voiceIt->second;
}

std::optional<std::reference_wrapper<sf::Music>> SfmlAudioEngine::findMusic(
    MusicId music) {
    const auto musicIt = this->music_.find(music);
    if (musicIt == this->music_.end()) return std::nullopt;
    return *musicIt->second;
}

}  // namespace ampersand::audio
