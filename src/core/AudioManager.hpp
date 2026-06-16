#pragma once

#include <list>
#include <string>
#include <unordered_map>

#include <SFML/Audio.hpp>

namespace tc {

// Background music (streamed via sf::Music) plus one-shot sound effects
// (loaded into sf::SoundBuffer and cached). Files are expected under
// assets/sounds/music/<track>.ogg and assets/sounds/sfx/<name>.ogg
class AudioManager {
public:
    void playMusic(const std::string& track, bool loop = true);
    void stopMusic();

    void playSfx(const std::string& name);

    void setMusicVolume(float volume); // 0–100
    void setSfxVolume(float volume);   // 0–100
    float getMusicVolume() const { return musicVolume; }
    float getSfxVolume() const { return sfxVolume; }

private:
    sf::Music music;
    std::string currentTrack;
    float musicVolume = 100.0f;
    float sfxVolume = 100.0f;

    std::unordered_map<std::string, sf::SoundBuffer> sfxBuffers;
    std::list<sf::Sound> activeSounds;
};

} // namespace tc
