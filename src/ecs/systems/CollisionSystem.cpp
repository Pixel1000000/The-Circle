#include "ecs/systems/CollisionSystem.hpp"

#include <cmath>
#include <vector>

#include "ecs/Components.hpp"

namespace tc {

void CollisionSystem::update(entt::registry& registry)
{
    std::vector<entt::entity> entities;
    for (auto entity : registry.view<Position, Renderable, Health>()) {
        if (registry.all_of<PhaseThrough>(entity)) continue;
        entities.push_back(entity);
    }

    for (std::size_t i = 0; i < entities.size(); ++i) {
        for (std::size_t j = i + 1; j < entities.size(); ++j) {
            const entt::entity a = entities[i];
            const entt::entity b = entities[j];

            auto& posA = registry.get<Position>(a);
            auto& posB = registry.get<Position>(b);
            const auto& sizeA = registry.get<Renderable>(a).size;
            const auto& sizeB = registry.get<Renderable>(b).size;

            const float dx = posB.x - posA.x;
            const float dy = posB.y - posA.y;

            const float overlapX = (sizeA.x + sizeB.x) * 0.5f - std::abs(dx);
            const float overlapY = (sizeA.y + sizeB.y) * 0.5f - std::abs(dy);

            if (overlapX <= 0.0f || overlapY <= 0.0f) continue;

            if (overlapX < overlapY) {
                const float push = overlapX * 0.5f;
                const float sign = dx >= 0.0f ? 1.0f : -1.0f;
                posA.x -= push * sign;
                posB.x += push * sign;
            } else {
                const float push = overlapY * 0.5f;
                const float sign = dy >= 0.0f ? 1.0f : -1.0f;
                posA.y -= push * sign;
                posB.y += push * sign;
            }
        }
    }
}

} // namespace tc
