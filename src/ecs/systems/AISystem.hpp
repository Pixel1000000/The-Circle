#pragma once

#include <entt/entt.hpp>

namespace tc {

// Drives enemy Velocity based on their AIBehavior relative to the player.
// Boss behaviors currently fall back to CHASE - their unique attack patterns
// are added on top once boss encounters are implemented.
class AISystem {
public:
    void update(entt::registry& registry, float dt);
};

} // namespace tc
