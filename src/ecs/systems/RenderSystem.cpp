#include "ecs/systems/RenderSystem.hpp"

#include <algorithm>
#include <vector>

#include "ecs/Components.hpp"

namespace tc {

void RenderSystem::update(entt::registry& registry, sf::RenderWindow& window, float dt)
{
    std::vector<entt::entity> expiredFlashes;
    for (auto entity : registry.view<HitFlash>()) {
        auto& flash = registry.get<HitFlash>(entity);
        flash.timer -= dt;
        if (flash.timer <= 0.0f) {
            expiredFlashes.push_back(entity);
        }
    }
    for (auto entity : expiredFlashes) {
        registry.remove<HitFlash>(entity);
    }

    auto view = registry.view<Position, Renderable>();
    for (auto entity : view) {
        const auto& pos = view.get<Position>(entity);
        const auto& renderable = view.get<Renderable>(entity);

        sf::Color color = renderable.color;
        if (const auto* flash = registry.try_get<HitFlash>(entity)) {
            const float t = std::clamp(flash->timer / HitFlash::DURATION, 0.0f, 1.0f);
            color.r = static_cast<sf::Uint8>(color.r + (255 - color.r) * t);
            color.g = static_cast<sf::Uint8>(color.g * (1.0f - t));
            color.b = static_cast<sf::Uint8>(color.b * (1.0f - t));
        }

        sf::RectangleShape shape(renderable.size);
        shape.setOrigin(renderable.size.x * 0.5f, renderable.size.y * 0.5f);
        shape.setPosition(pos.x, pos.y);
        shape.setFillColor(color);

        window.draw(shape);
    }
}

} // namespace tc
