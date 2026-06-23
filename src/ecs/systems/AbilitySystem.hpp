#pragma once

#include <vector>

#include <entt/entt.hpp>

namespace tc {

// Drives all per-enemy special abilities (DashAbility, BurrowAbility,
// ChargeAbility, RageAbility, TeleportAbility, WitchCloneAbility,
// AbsorbChance, BoneDetach, SkeletonReviveBonus, QuicksandSpawner,
// SwarmScatter, EmergencySummon, SkeletonBones) and the death-triggered
// abilities (TrapSpawner, IceGoblinExplosion, MummyDeathBomb,
// WitchCloneTag) that must run before LootSystem destroys dead enemies.
class AbilitySystem {
public:
    // Called once per frame between AISystem and CombatSystem. Any new
    // enemy entities spawned by an ability (e.g. BoneDetach, the goblin
    // archer's emergency summon) are appended to `spawnedEnemies` so the
    // caller can track them for wave-completion purposes.
    void update(entt::registry& registry, entt::entity player, float dt,
        std::vector<entt::entity>& spawnedEnemies);

    // Called once per frame after CombatSystem/StatusEffectSystem but
    // before LootSystem, so abilities can react to lethal damage dealt
    // this frame before the dead entity is destroyed.
    void handleDeathEffects(entt::registry& registry, entt::entity player, float dt);
};

} // namespace tc
