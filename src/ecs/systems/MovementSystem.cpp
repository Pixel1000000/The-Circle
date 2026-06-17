#include "ecs/systems/MovementSystem.hpp"

#include <algorithm>

#include "ecs/Components.hpp"

namespace tc {

void MovementSystem::update(entt::registry& registry, float dt)
{
    auto view = registry.view<Position, Velocity, Speed>();
    for (auto entity : view) {
        if (registry.all_of<Stunned>(entity)) continue;

        auto& position = view.get<Position>(entity);
        const auto& velocity = view.get<Velocity>(entity);
        float speed = view.get<Speed>(entity).value;

        if (const auto* effect = registry.try_get<StatusEffect>(entity)) {
            if (effect->type == StatusEffect::SLOW) {
                float slowResist = 0.0f;
                if (const auto* resist = registry.try_get<ElementalResist>(entity)) {
                    slowResist = resist->slowResist;
                }
                if (slowResist < 1.0f) {
                    speed *= 0.5f;
                }
            }
        }

        position.x += velocity.dx * speed * dt;
        position.y += velocity.dy * speed * dt;

        if (const auto* equipment = registry.try_get<Equipment>(entity)) {
            if (equipment->leggingsElement == Element::NATURE && (velocity.dx != 0.0f || velocity.dy != 0.0f)) {
                if (auto* health = registry.try_get<Health>(entity)) {
                    if (auto* lifesteal = registry.try_get<Lifesteal>(entity)) {
                        if (health->current < health->max) {
                            lifesteal->accumulator += speed * equipment->leggingsElementPercent * 0.03f * dt;
                            const int healAmount = static_cast<int>(lifesteal->accumulator);
                            if (healAmount > 0) {
                                lifesteal->accumulator -= static_cast<float>(healAmount);
                                health->current = std::min(health->current + healAmount, health->max);
                            }
                        }
                    }
                }
            }
        }
    }
}

} // namespace tc
