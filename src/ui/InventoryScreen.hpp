#pragma once

#include <SFML/Graphics.hpp>
#include <entt/entt.hpp>

#include "core/FontManager.hpp"
#include "core/Localization.hpp"

namespace tc {

// Tab-key inventory overlay: lists equipped items with all stats and elemental
// bonuses. A '?' button in the top-right corner toggles an elemental reference
// panel showing what each element does per slot type.
class InventoryScreen {
public:
    enum class Button { NONE, HELP };

    InventoryScreen();

    void render(sf::RenderWindow& window, const Localization& localization,
        const FontManager& fontManager, entt::registry& registry, entt::entity player);

    Button getButtonAt(sf::Vector2f point) const;

    void toggleHelp();
    void resetHelp();

private:
    sf::RectangleShape panel;
    sf::RectangleShape helpBtn;
    bool helpVisible = false;
};

} // namespace tc
