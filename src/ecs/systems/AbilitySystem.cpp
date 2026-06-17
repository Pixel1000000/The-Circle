#include "ecs/systems/AbilitySystem.hpp"

#include "ecs/Components.hpp"

namespace tc {

void AbilitySystem::update(entt::registry& registry, entt::entity player, float dt)
{
    (void)registry;
    (void)player;
    (void)dt;
    // Per-ability logic is added in later stages.
}

void AbilitySystem::handleDeathEffects(entt::registry& registry, float dt)
{
    (void)registry;
    (void)dt;
    // Death-triggered ability logic (TrapSpawner, FreezeOnDeath,
    // MummyDeathBomb) is added in a later stage.
}

} // namespace tc
