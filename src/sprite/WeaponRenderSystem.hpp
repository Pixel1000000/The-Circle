#pragma once

#include <entt/entt.hpp>

#include <SFML/Graphics.hpp>

namespace tc::sprite {

// Draws the weapon socketed to each wielder's hand, following its own
// attack curve rather than the body's frames. Since a weapon keyframe can
// ask to be drawn either in front of or behind the body on a given frame,
// callers run this twice per frame around the body/equipment render pass:
// once with drawFrontPass=false (before the body), once with true (after).
class WeaponRenderSystem {
public:
    void render(sf::RenderTarget& target, entt::registry& registry, bool drawFrontPass);
};

} // namespace tc::sprite
