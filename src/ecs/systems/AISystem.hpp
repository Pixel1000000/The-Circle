#pragma once

#include <entt/entt.hpp>

namespace tc {

// Drives enemy Velocity based on their AIBehavior relative to the player.
// Boss behaviors (BOSS_DRUID/BOSS_NAGA/BOSS_ELEMENTAL/BOSS_LICH) additionally
// cycle attack patterns over time via the entity's BossAI state.
class AISystem {
public:
    void update(entt::registry& registry, float dt);
};

} // namespace tc
