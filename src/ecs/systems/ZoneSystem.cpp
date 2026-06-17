#include "ecs/systems/ZoneSystem.hpp"

#include "ecs/Components.hpp"

namespace tc {

void ZoneSystem::update(entt::registry& registry, entt::entity player, float dt)
{
    (void)registry;
    (void)player;
    (void)dt;
    // Zone effect/expiry logic is added in a later stage.
}

} // namespace tc
