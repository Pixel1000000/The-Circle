#include "ecs/systems/MovementSystem.hpp"

#include "ecs/Components.hpp"

namespace tc {

void MovementSystem::update(entt::registry& registry, float dt)
{
    auto view = registry.view<Position, Velocity, Speed>();
    for (auto entity : view) {
        auto& position = view.get<Position>(entity);
        const auto& velocity = view.get<Velocity>(entity);
        float speed = view.get<Speed>(entity).value;

        if (const auto* effect = registry.try_get<StatusEffect>(entity)) {
            if (effect->type == StatusEffect::SLOW) {
                speed *= 0.5f;
            }
        }

        position.x += velocity.dx * speed * dt;
        position.y += velocity.dy * speed * dt;
    }
}

} // namespace tc
