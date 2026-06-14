#pragma once

#include <entt/entt.hpp>

namespace tc {

// Separates overlapping entities with Position + Renderable + Health (i.e.
// the player, enemies and bosses - not projectiles). For each overlapping
// pair, pushes both entities apart by half the AABB penetration depth along
// the axis of least overlap. Entities with PhaseThrough are skipped.
class CollisionSystem {
public:
    void update(entt::registry& registry);
};

} // namespace tc
