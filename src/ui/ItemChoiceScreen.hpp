#pragma once

#include <SFML/Graphics.hpp>

#include "core/FontManager.hpp"
#include "core/Localization.hpp"
#include "ecs/Components.hpp"

namespace tc {

// Modal overlay shown when an equipment drop's slot is already occupied and
// the slot's autoUpgrade* flag is not set (ItemUpgrader::getAutoUpgrade).
// Lets the player choose between equipping the dropped item (takeNew) or
// merging its stats into the current item (upgradeCurrent), optionally
// remembering the choice for the slot via the "don't ask again" checkbox.
class ItemChoiceScreen {
public:
    enum class Button { NONE, TAKE_NEW, UPGRADE_CURRENT, CHECKBOX };

    ItemChoiceScreen();

    // Resets the "don't ask again" checkbox to unchecked.
    void open();

    void render(sf::RenderWindow& window, const Localization& localization, const FontManager& fontManager,
        int slot, int newTier, Element newElement, float newPercent,
        int currentTier, Element currentElement, float currentPercent);

    Button getButtonAt(sf::Vector2f point) const;

    void toggleDontAskAgain();
    bool isDontAskAgain() const;

private:
    sf::RectangleShape overlay;
    sf::RectangleShape takeNewButton;
    sf::RectangleShape upgradeButton;
    sf::RectangleShape checkbox;

    bool dontAskAgain = false;
};

} // namespace tc
