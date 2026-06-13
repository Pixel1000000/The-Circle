#pragma once

#include <entt/entt.hpp>

namespace tc {

// Applies poison DoT damage each tick and removes StatusEffect components
// once their duration has elapsed. Slow is read directly by MovementSystem.
class StatusEffectSystem {
public:
    void update(entt::registry& registry, float dt);
};

} // namespace tc
