#pragma once

#include <entt/entt.hpp>

namespace tc::sprite {

// Advances AnimationState for every entity with AnimationState + BaseAppearance.
class AnimationSystem {
public:
    void update(entt::registry& registry, float dt);
};

} // namespace tc::sprite
