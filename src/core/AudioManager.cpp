#include "core/AudioManager.hpp"

#include <iostream>

namespace tc {

void AudioManager::playMusic(const std::string& track, bool loop)
{
    if (currentTrack == track && music.getStatus() == sf::Music::Playing) {
        return;
    }

    const std::string path = "assets/sounds/music/" + track + ".ogg";
    if (!music.openFromFile(path)) {
        std::cerr << "[AudioManager] Could not open " << path << "\n";
        return;
    }

    currentTrack = track;
    music.setLoop(loop);
    music.play();
}

void AudioManager::stopMusic()
{
    music.stop();
    currentTrack.clear();
}

void AudioManager::playSfx(const std::string& name)
{
    auto it = sfxBuffers.find(name);
    if (it == sfxBuffers.end()) {
        sf::SoundBuffer buffer;
        const std::string path = "assets/sounds/sfx/" + name + ".ogg";
        if (!buffer.loadFromFile(path)) {
            std::cerr << "[AudioManager] Could not open " << path << "\n";
            return;
        }
        it = sfxBuffers.emplace(name, std::move(buffer)).first;
    }

    activeSounds.remove_if([](const sf::Sound& sound) { return sound.getStatus() != sf::Sound::Playing; });

    activeSounds.emplace_back(it->second);
    activeSounds.back().play();
}

} // namespace tc
