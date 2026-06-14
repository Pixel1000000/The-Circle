#pragma once

#include <SFML/Graphics.hpp>
#include <entt/entt.hpp>

#include "core/FontManager.hpp"
#include "core/Localization.hpp"

namespace tc {

// Draws every entity with Position + Renderable as a placeholder rectangle.
// Enemies (Health + Name) additionally get an HP bar and a name label above
// their sprite.
class RenderSystem {
public:
    void update(entt::registry& registry, sf::RenderWindow& window, const Localization& localization,
        const FontManager& fontManager, float dt);

private:
    sf::Text nameText;
    bool textInitialized = false;
};

} // namespace tc
