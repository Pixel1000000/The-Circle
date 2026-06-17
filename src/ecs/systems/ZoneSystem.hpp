#pragma once

#include <entt/entt.hpp>

namespace tc {

// Checks the player and enemies against every effect zone (TrapTag,
// QuicksandTag, IceZoneTag), applies the matching effect on overlap, and
// destroys zones once their duration expires.
class ZoneSystem {
public:
    void update(entt::registry& registry, entt::entity player, float dt);
};

} // namespace tc
