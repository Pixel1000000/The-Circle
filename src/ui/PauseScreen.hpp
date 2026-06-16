#pragma once

#include <SFML/Graphics.hpp>

#include "core/FontManager.hpp"
#include "core/Localization.hpp"

namespace tc {

// Semi-transparent overlay drawn on top of the frozen game world while
// PlayState is paused (Esc). Offers Resume or returning to the main menu.
class PauseScreen {
public:
    enum class Button { NONE, RESUME, SETTINGS, MAIN_MENU };

    PauseScreen();

    void render(sf::RenderWindow& window, const Localization& localization, const FontManager& fontManager);

    Button getButtonAt(sf::Vector2f point) const;

private:
    sf::RectangleShape overlay;
    sf::RectangleShape resumeButton;
    sf::RectangleShape settingsButton;
    sf::RectangleShape mainMenuButton;
};

} // namespace tc
