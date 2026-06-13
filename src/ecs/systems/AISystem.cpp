#include "ecs/systems/AISystem.hpp"

#include <cmath>

#include "ecs/Components.hpp"

namespace tc {

void AISystem::update(entt::registry& registry, float dt)
{
    (void)dt;

    auto playerView = registry.view<PlayerTag, Position>();
    if (playerView.begin() == playerView.end()) return;

    const entt::entity player = *playerView.begin();
    const auto playerPos = playerView.get<Position>(player);

    auto view = registry.view<EnemyTag, AIBehavior, Position, Velocity>();
    for (auto entity : view) {
        const auto& pos = view.get<Position>(entity);
        auto& velocity = view.get<Velocity>(entity);
        const auto& behavior = view.get<AIBehavior>(entity);

        const float dx = playerPos.x - pos.x;
        const float dy = playerPos.y - pos.y;
        const float distance = std::sqrt(dx * dx + dy * dy);

        if (distance < 0.0001f) {
            velocity.dx = 0.0f;
            velocity.dy = 0.0f;
            continue;
        }

        const float nx = dx / distance;
        const float ny = dy / distance;

        switch (behavior.type) {
        case AIBehavior::RANGED_KEEP_DISTANCE: {
            float keepDistance = 200.0f;
            if (const auto* ranged = registry.try_get<RangedCombat>(entity)) {
                keepDistance = ranged->range * 0.8f;
            }

            if (distance < keepDistance - 10.0f) {
                velocity.dx = -nx;
                velocity.dy = -ny;
            } else if (distance > keepDistance + 10.0f) {
                velocity.dx = nx;
                velocity.dy = ny;
            } else {
                velocity.dx = 0.0f;
                velocity.dy = 0.0f;
            }
            break;
        }
        case AIBehavior::SWARM: {
            float vx = nx - ny * 0.5f;
            float vy = ny + nx * 0.5f;
            const float length = std::sqrt(vx * vx + vy * vy);
            if (length > 0.0001f) {
                vx /= length;
                vy /= length;
            }
            velocity.dx = vx;
            velocity.dy = vy;
            break;
        }
        case AIBehavior::CHASE:
        case AIBehavior::BOSS_DRUID:
        case AIBehavior::BOSS_NAGA:
        case AIBehavior::BOSS_ELEMENTAL:
        case AIBehavior::BOSS_LICH:
        default:
            velocity.dx = nx;
            velocity.dy = ny;
            break;
        }
    }
}

} // namespace tc
