#pragma once

#include <entt/entt.hpp>

namespace tc {

// Moves drifting blizzard zones and applies SLOW to the player while
// they overlap any BlizzardZone entity.
class BlizzardSystem {
public:
    void update(entt::registry& registry, entt::entity player, float dt);
};

} // namespace tc
