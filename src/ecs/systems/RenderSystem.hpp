#pragma once

#include <SFML/Graphics.hpp>
#include <entt/entt.hpp>

namespace tc {

// Draws every entity with Position + Renderable as a placeholder rectangle.
class RenderSystem {
public:
    void update(entt::registry& registry, sf::RenderWindow& window, float dt);
};

} // namespace tc
