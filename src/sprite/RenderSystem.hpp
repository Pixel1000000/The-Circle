#pragma once

#include <entt/entt.hpp>

#include <SFML/Graphics.hpp>

namespace tc::sprite {

// Draws the body + paperdoll equipment layers for every entity with
// AnimationState + BaseAppearance. Named tc::sprite::RenderSystem to avoid
// colliding with the placeholder-rectangle tc::RenderSystem.
class RenderSystem {
public:
    void render(sf::RenderTarget& target, entt::registry& registry);
};

} // namespace tc::sprite
