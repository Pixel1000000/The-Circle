#pragma once

#include <entt/entt.hpp>

namespace tc {

// Integrates Position from Velocity (normalized direction) and Speed.
class MovementSystem {
public:
    void update(entt::registry& registry, float dt);
};

} // namespace tc
