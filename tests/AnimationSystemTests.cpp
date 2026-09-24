#include <doctest/doctest.h>

#include <entt/entt.hpp>

#include "sprite/AnimationSystem.hpp"
#include "sprite/SpriteTypes.hpp"

using namespace tc::sprite;

namespace {

std::shared_ptr<SpriteSheetAsset> makeTestAsset()
{
    auto asset = std::make_shared<SpriteSheetAsset>();

    AnimationFrameData walkSouth;
    walkSouth.frames = {{0, 0, 64, 64}, {64, 0, 64, 64}, {128, 0, 64, 64}, {192, 0, 64, 64}};
    walkSouth.frameDuration = 0.1f;
    walkSouth.loop = true;
    asset->animations["walk"][Facing::South] = walkSouth;

    AnimationFrameData walkEast = walkSouth;
    asset->animations["walk"][Facing::East] = walkEast;

    AnimationFrameData deathSouth;
    deathSouth.frames = {{0, 64, 64, 64}, {64, 64, 64, 64}, {128, 64, 64, 64}};
    deathSouth.frameDuration = 0.2f;
    deathSouth.loop = false;
    asset->animations["death"][Facing::South] = deathSouth;

    return asset;
}

} // namespace

TEST_CASE("AnimationSystem advances frames on a looping animation and wraps")
{
    entt::registry registry;
    auto entity = registry.create();
    registry.emplace<BaseAppearance>(entity, BaseAppearance{makeTestAsset()});
    auto& state = registry.emplace<AnimationState>(entity);
    state.currentAnimation = "walk";
    state.facing = Facing::South;

    AnimationSystem system;

    CHECK(state.currentFrame == 0);
    system.update(registry, 0.1f);
    CHECK(state.currentFrame == 1);
    system.update(registry, 0.1f);
    CHECK(state.currentFrame == 2);
    system.update(registry, 0.1f);
    CHECK(state.currentFrame == 3);
    system.update(registry, 0.1f);
    CHECK(state.currentFrame == 0); // wrapped
}

TEST_CASE("Switching facing mid-animation does not reset currentFrame")
{
    entt::registry registry;
    auto entity = registry.create();
    registry.emplace<BaseAppearance>(entity, BaseAppearance{makeTestAsset()});
    auto& state = registry.emplace<AnimationState>(entity);
    state.currentAnimation = "walk";
    state.facing = Facing::South;

    AnimationSystem system;
    system.update(registry, 0.1f);
    system.update(registry, 0.1f);
    REQUIRE(state.currentFrame == 2);

    state.facing = Facing::East; // both directions have the same 4-frame walk
    system.update(registry, 0.0f); // no time elapsed, just re-evaluate direction

    CHECK(state.currentFrame == 2);
}

TEST_CASE("AnimationSystem falls back to idle when the animation is missing for the body")
{
    entt::registry registry;
    auto entity = registry.create();
    registry.emplace<BaseAppearance>(entity, BaseAppearance{makeTestAsset()});
    auto& state = registry.emplace<AnimationState>(entity);
    state.currentAnimation = "run"; // not present in the test asset
    state.facing = Facing::South;
    state.currentFrame = 2;

    AnimationSystem system;
    system.update(registry, 0.0f);

    CHECK(state.currentAnimation == "idle");
    CHECK(state.currentFrame == 0);
    CHECK_FALSE(state.finished);
}

TEST_CASE("AnimationSystem stops and marks finished on a non-looping animation's last frame")
{
    entt::registry registry;
    auto entity = registry.create();
    registry.emplace<BaseAppearance>(entity, BaseAppearance{makeTestAsset()});
    auto& state = registry.emplace<AnimationState>(entity);
    state.currentAnimation = "death";
    state.facing = Facing::South;

    AnimationSystem system;
    system.update(registry, 0.2f);
    CHECK(state.currentFrame == 1);
    system.update(registry, 0.2f);
    CHECK(state.currentFrame == 2);
    CHECK(state.finished);

    // Further updates must not advance past the last frame nor loop back.
    system.update(registry, 1.0f);
    CHECK(state.currentFrame == 2);
    CHECK(state.finished);
}
