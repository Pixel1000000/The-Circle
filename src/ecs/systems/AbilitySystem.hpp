#pragma once

#include <entt/entt.hpp>

namespace tc {

// Drives all per-enemy special abilities (DashAbility, BurrowAbility,
// ChargeAbility, RageAbility, TeleportAbility, AbsorbChance, BoneDetach,
// SkeletonReviveBonus, QuicksandSpawner, SwarmScatter, EmergencySummon) and
// the death-triggered abilities (TrapSpawner, FreezeOnDeath,
// MummyDeathBomb) that must run before LootSystem destroys dead enemies.
class AbilitySystem {
public:
    // Called once per frame between AISystem and CombatSystem.
    void update(entt::registry& registry, entt::entity player, float dt);

    // Called once per frame after CombatSystem/StatusEffectSystem but
    // before LootSystem, so abilities can react to lethal damage dealt
    // this frame before the dead entity is destroyed.
    void handleDeathEffects(entt::registry& registry, float dt);
};

} // namespace tc
