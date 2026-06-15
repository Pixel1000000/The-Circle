#include "ecs/systems/StatusEffectSystem.hpp"

#include <algorithm>
#include <vector>

#include "config/ConfigLoader.hpp"
#include "ecs/Components.hpp"

namespace tc {

namespace {

// Accumulates `dps * dt` into `accumulator` and returns the whole number of
// damage points to apply this tick, leaving the fractional remainder behind.
int accumulateTick(float& accumulator, float dps, float dt)
{
    accumulator += dps * dt;
    const int tick = static_cast<int>(accumulator);
    if (tick > 0) {
        accumulator -= static_cast<float>(tick);
    }
    return tick;
}

} // namespace

void StatusEffectSystem::update(entt::registry& registry, float dt)
{
    std::vector<entt::entity> expired;

    auto view = registry.view<StatusEffect, Health>();
    for (auto entity : view) {
        auto& effect = view.get<StatusEffect>(entity);
        auto& health = view.get<Health>(entity);

        if (effect.type == StatusEffect::POISON && !registry.all_of<Invulnerable>(entity)) {
            const int tick = accumulateTick(effect.damageAccumulator, effect.dps, dt);
            if (tick > 0) {
                health.current = std::max(health.current - tick, 0);
            }
        }

        effect.timer -= dt;
        if (effect.timer <= 0.0f) {
            expired.push_back(entity);
        }
    }

    for (auto entity : expired) {
        registry.remove<StatusEffect>(entity);
        registry.remove<IceChill>(entity);
    }

    const auto& elementalConfig = ConfigLoader::get().getElementalConfig();

    // NatureStack: stacking DoT from a NATURE-element weapon.
    std::vector<entt::entity> expiredNature;
    for (auto entity : registry.view<NatureStack, Health>()) {
        auto& stack = registry.get<NatureStack>(entity);
        auto& health = registry.get<Health>(entity);

        if (!registry.all_of<Invulnerable>(entity)) {
            float resist = 0.0f;
            if (const auto* elementalResist = registry.try_get<ElementalResist>(entity)) {
                resist = elementalResist->natureResist;
            }
            const float dps = elementalConfig.natureBaseDps * stack.dpsMultiplier * (1.0f - resist);
            const int tick = accumulateTick(stack.damageAccumulator, dps, dt);
            if (tick > 0) {
                health.current = std::max(health.current - tick, 0);
            }
        }

        stack.timer -= dt;
        if (stack.timer <= 0.0f) {
            expiredNature.push_back(entity);
        }
    }
    for (auto entity : expiredNature) {
        registry.remove<NatureStack>(entity);
    }

    // FireBurn: refreshable DoT from a FIRE-element weapon.
    std::vector<entt::entity> expiredFire;
    for (auto entity : registry.view<FireBurn, Health>()) {
        auto& burn = registry.get<FireBurn>(entity);
        auto& health = registry.get<Health>(entity);

        if (!registry.all_of<Invulnerable>(entity)) {
            float resist = 0.0f;
            if (const auto* elementalResist = registry.try_get<ElementalResist>(entity)) {
                resist = elementalResist->fireResist;
            }
            const float dps = burn.dps * (1.0f - resist);
            const int tick = accumulateTick(burn.damageAccumulator, dps, dt);
            if (tick > 0) {
                health.current = std::max(health.current - tick, 0);
            }
        }

        burn.timer -= dt;
        if (burn.timer <= 0.0f) {
            expiredFire.push_back(entity);
        }
    }
    for (auto entity : expiredFire) {
        registry.remove<FireBurn>(entity);
    }

    // DecayEffect: temporary Health.max reduction, restored on expiry.
    std::vector<entt::entity> expiredDecay;
    for (auto entity : registry.view<DecayEffect>()) {
        auto& decay = registry.get<DecayEffect>(entity);
        decay.timer -= dt;
        if (decay.timer <= 0.0f) {
            expiredDecay.push_back(entity);
        }
    }
    for (auto entity : expiredDecay) {
        const auto& decay = registry.get<DecayEffect>(entity);
        auto& health = registry.get<Health>(entity);
        health.max += decay.maxHpReduction;
        health.current = std::min(health.current, health.max);
        registry.remove<DecayEffect>(entity);
    }
}

} // namespace tc
