#include "ecs/systems/StatusEffectSystem.hpp"

#include <algorithm>
#include <vector>

#include "ecs/Components.hpp"

namespace tc {

void StatusEffectSystem::update(entt::registry& registry, float dt)
{
    std::vector<entt::entity> expired;

    auto view = registry.view<StatusEffect, Health>();
    for (auto entity : view) {
        auto& effect = view.get<StatusEffect>(entity);
        auto& health = view.get<Health>(entity);

        if (effect.type == StatusEffect::POISON) {
            const int tick = static_cast<int>(effect.dps * dt);
            health.current = std::max(health.current - tick, 0);
        }

        effect.timer -= dt;
        if (effect.timer <= 0.0f) {
            expired.push_back(entity);
        }
    }

    for (auto entity : expired) {
        registry.remove<StatusEffect>(entity);
    }
}

} // namespace tc
