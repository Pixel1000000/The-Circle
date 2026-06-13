#pragma once

#include <entt/entt.hpp>
#include <SFML/Graphics.hpp>

#include "core/FontManager.hpp"
#include "core/Localization.hpp"

namespace tc {

// In-game overlay: player health bar, key fragment progress and potion charges.
class HUD {
public:
    void render(sf::RenderWindow& window, entt::registry& registry, entt::entity player,
        const Localization& localization, const FontManager& fontManager, int keyFragmentsRequired);
};

} // namespace tc
