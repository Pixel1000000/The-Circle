#include "sprite/AnimationSystem.hpp"

#include <iostream>

#include "sprite/SpriteTypes.hpp"

namespace tc::sprite {

void AnimationSystem::update(entt::registry& registry, float dt)
{
    auto view = registry.view<AnimationState, BaseAppearance>();
    for (auto entity : view) {
        auto& state = view.get<AnimationState>(entity);
        auto& appearance = view.get<BaseAppearance>(entity);
        if (!appearance.bodySheet) {
            continue;
        }

        // Idle is a single static pose per direction (the "rotations" row),
        // not part of the animations table, so it never advances frames.
        if (state.currentAnimation == "idle") {
            state.currentFrame = 0;
            state.elapsedInFrame = 0.f;
            state.finished = false;
            continue;
        }

        const auto animIt = appearance.bodySheet->animations.find(state.currentAnimation);
        const bool hasDirection = animIt != appearance.bodySheet->animations.end()
            && animIt->second.find(state.facing) != animIt->second.end();

        if (!hasDirection) {
            std::cerr << "[AnimationSystem] No animation \"" << state.currentAnimation
                       << "\" for body, falling back to idle\n";
            state.currentAnimation = "idle";
            state.currentFrame = 0;
            state.elapsedInFrame = 0.f;
            state.finished = false;
            continue;
        }

        const AnimationFrameData& data = animIt->second.at(state.facing);
        if (data.frames.empty()) {
            continue;
        }

        // Direction switches never reset currentFrame/elapsedInFrame - only
        // frame-duration expiry (below) or the fallback above does, so a
        // character mid-stride keeps its stride when it turns.
        if (state.currentFrame >= static_cast<int>(data.frames.size())) {
            state.currentFrame = data.loop ? 0 : static_cast<int>(data.frames.size()) - 1;
        }

        if (data.frameDuration <= 0.f || (state.finished && !data.loop)) {
            continue;
        }

        state.elapsedInFrame += dt;
        while (state.elapsedInFrame >= data.frameDuration) {
            state.elapsedInFrame -= data.frameDuration;
            const int nextFrame = state.currentFrame + 1;
            const int lastFrame = static_cast<int>(data.frames.size()) - 1;
            if (nextFrame <= lastFrame) {
                state.currentFrame = nextFrame;
                // Reaching the last frame of a non-looping animation finishes
                // it immediately - no extra frameDuration is spent "on" it.
                if (nextFrame == lastFrame && !data.loop) {
                    state.finished = true;
                    state.elapsedInFrame = 0.f;
                    break;
                }
            } else if (data.loop) {
                state.currentFrame = 0;
            } else {
                state.currentFrame = lastFrame;
                state.finished = true;
                state.elapsedInFrame = 0.f;
                break;
            }
        }
    }
}

} // namespace tc::sprite
