#pragma once

#include <SFML/Graphics.hpp>

#include "core/AudioManager.hpp"
#include "core/FontManager.hpp"
#include "core/Localization.hpp"

namespace tc {

// Settings sub-screen shown from PauseScreen: volume sliders + language toggle.
class SettingsScreen {
public:
    SettingsScreen();

    enum class Button { NONE, BACK, LANG_RU, LANG_EN,
                        MUSIC_MINUS, MUSIC_PLUS, SFX_MINUS, SFX_PLUS };

    void render(sf::RenderWindow& window, const Localization& loc,
                const FontManager& fontManager, const AudioManager& audio);

    Button getButtonAt(sf::Vector2f point) const;

private:
    sf::RectangleShape overlay;
    sf::RectangleShape panel;

    sf::RectangleShape backBtn;
    sf::RectangleShape langRuBtn;
    sf::RectangleShape langEnBtn;
    sf::RectangleShape musicMinusBtn;
    sf::RectangleShape musicPlusBtn;
    sf::RectangleShape sfxMinusBtn;
    sf::RectangleShape sfxPlusBtn;
};

} // namespace tc
