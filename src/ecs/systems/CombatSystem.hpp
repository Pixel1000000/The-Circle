#pragma once

#include <entt/entt.hpp>

namespace tc {

// Handles melee/ranged attacks, projectile collisions and damage mitigation
// (Armor + BlockAbility), plus the Potion charge-on-kill mechanic.
class CombatSystem {
public:
    void update(entt::registry& registry, float dt);

    // Consumes one potion charge to heal the player. Returns false if no
    // charge is available or the player is already at full health.
    bool usePotion(entt::registry& registry, entt::entity player);

private:
    void updateMelee(entt::registry& registry, float dt);
    void updateRanged(entt::registry& registry, float dt);
    void updateProjectiles(entt::registry& registry);

    void applyDamage(entt::registry& registry, entt::entity attacker, entt::entity target, int rawDamage);
};

} // namespace tc
