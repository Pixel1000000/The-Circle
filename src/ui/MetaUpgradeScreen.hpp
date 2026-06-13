#pragma once

#include <SFML/Graphics.hpp>

#include "core/FontManager.hpp"
#include "core/Localization.hpp"
#include "ecs/Components.hpp"

namespace tc {

// Renders the post-run meta-progression screen (Strength / Endurance / Health
// upgrades) and reports which button a click landed on.
class MetaUpgradeScreen {
public:
    enum class Button { NONE, STRENGTH, ENDURANCE, HEALTH, CONTINUE };

    MetaUpgradeScreen();

    void render(sf::RenderWindow& window, const Localization& localization,
        const FontManager& fontManager, const MetaStats& stats);

    Button getButtonAt(sf::Vector2f point) const;

private:
    sf::RectangleShape strengthButton;
    sf::RectangleShape enduranceButton;
    sf::RectangleShape healthButton;
    sf::RectangleShape continueButton;
};

} // namespace tc
