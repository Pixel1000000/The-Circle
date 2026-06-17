#include "ecs/systems/AbilitySystem.hpp"

#include <cmath>

#include "ecs/Components.hpp"

namespace tc {

namespace {

constexpr float DASH_SPEED = 480.0f;
constexpr float CHARGE_TRIGGER_RANGE = 260.0f;
constexpr float CHARGE_DURATION = 0.5f;

sf::Vector2f directionTo(const Position& from, const Position& to)
{
    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length < 0.0001f) return {0.0f, 0.0f};
    return {dx / length, dy / length};
}

float healthFraction(const Health& health)
{
    return health.max > 0 ? static_cast<float>(health.current) / static_cast<float>(health.max) : 0.0f;
}

} // namespace

void AbilitySystem::update(entt::registry& registry, entt::entity player, float dt)
{
    if (!registry.valid(player)) return;
    const auto& playerPos = registry.get<Position>(player);

    // Wolf: periodic burst dash toward the player, invulnerable mid-dash.
    for (auto entity : registry.view<DashAbility, Position, Velocity, Health>()) {
        auto& dash = registry.get<DashAbility>(entity);
        const auto& pos = registry.get<Position>(entity);
        auto& health = registry.get<Health>(entity);
        if (health.current <= 0) continue;

        if (dash.dashing) {
            dash.durationTimer -= dt;
            const sf::Vector2f dir = directionTo(pos, playerPos);
            registry.get<Position>(entity).x += dir.x * DASH_SPEED * dt;
            registry.get<Position>(entity).y += dir.y * DASH_SPEED * dt;
            if (dash.durationTimer <= 0.0f) {
                dash.dashing = false;
                dash.timer = dash.cooldown;
                registry.remove<Invulnerable>(entity);
            }
        } else {
            dash.timer -= dt;
            if (dash.timer <= 0.0f) {
                dash.dashing = true;
                dash.durationTimer = dash.duration;
                registry.emplace_or_replace<Invulnerable>(entity);
            }
        }
    }

    // Scorpion: burrows (becomes invulnerable and stops moving) once its HP
    // drops below the threshold, for a fixed duration, once.
    for (auto entity : registry.view<BurrowAbility, Velocity, Health>()) {
        auto& burrow = registry.get<BurrowAbility>(entity);
        auto& health = registry.get<Health>(entity);
        if (health.current <= 0) continue;

        if (burrow.burrowed) {
            burrow.timer -= dt;
            registry.get<Velocity>(entity) = {0.0f, 0.0f};
            if (burrow.timer <= 0.0f) {
                burrow.burrowed = false;
                registry.remove<Invulnerable>(entity);
            }
        } else if (healthFraction(health) < burrow.hpThreshold) {
            burrow.burrowed = true;
            burrow.timer = burrow.duration;
            registry.emplace_or_replace<Invulnerable>(entity);
        }
    }

    // Raider: periodically charges in a straight line at the player.
    for (auto entity : registry.view<ChargeAbility, Position, Velocity, Health>()) {
        auto& charge = registry.get<ChargeAbility>(entity);
        const auto& pos = registry.get<Position>(entity);
        auto& health = registry.get<Health>(entity);
        if (health.current <= 0) continue;

        if (charge.charging) {
            registry.get<Position>(entity).x += charge.chargeDir.x * charge.speed * dt;
            registry.get<Position>(entity).y += charge.chargeDir.y * charge.speed * dt;
            charge.timer -= dt;
            if (charge.timer <= 0.0f) {
                charge.charging = false;
                charge.timer = charge.cooldown;
            }
        } else {
            charge.timer -= dt;
            if (charge.timer <= 0.0f) {
                const float dx = playerPos.x - pos.x;
                const float dy = playerPos.y - pos.y;
                if (dx * dx + dy * dy <= CHARGE_TRIGGER_RANGE * CHARGE_TRIGGER_RANGE) {
                    charge.charging = true;
                    charge.chargeDir = directionTo(pos, playerPos);
                    charge.timer = CHARGE_DURATION;
                } else {
                    charge.timer = 0.5f; // retry shortly until in range
                }
            }
        }
    }

    // Yeti: enrages once, permanently boosting speed and damage.
    for (auto entity : registry.view<RageAbility, Speed, Damage, Health>()) {
        auto& rage = registry.get<RageAbility>(entity);
        if (rage.enraged) continue;
        auto& health = registry.get<Health>(entity);
        if (health.current <= 0) continue;

        if (healthFraction(health) < rage.hpThreshold) {
            rage.enraged = true;
            registry.get<Speed>(entity).value *= rage.speedMult;
            registry.get<Damage>(entity).value = static_cast<int>(
                std::round(registry.get<Damage>(entity).value * rage.damageMult));
        }
    }

    // Skeleton warrior: after reviving once, gains a brief invulnerability
    // window plus a permanent damage bonus.
    for (auto entity : registry.view<SkeletonReviveBonus, ReviveOnce, Damage, Health>()) {
        auto& bonus = registry.get<SkeletonReviveBonus>(entity);
        auto& revive = registry.get<ReviveOnce>(entity);
        auto& health = registry.get<Health>(entity);
        if (health.current <= 0) continue;

        if (revive.used && !bonus.bonusActive) {
            bonus.bonusActive = true;
            bonus.invulTimer = bonus.invulDuration;
            registry.emplace_or_replace<Invulnerable>(entity);
            registry.get<Damage>(entity).value = static_cast<int>(
                std::round(registry.get<Damage>(entity).value * bonus.damageMult));
        } else if (bonus.bonusActive && bonus.invulTimer > 0.0f) {
            bonus.invulTimer -= dt;
            if (bonus.invulTimer <= 0.0f) {
                registry.remove<Invulnerable>(entity);
            }
        }
    }
}

void AbilitySystem::handleDeathEffects(entt::registry& registry, float dt)
{
    (void)registry;
    (void)dt;
    // Death-triggered ability logic (TrapSpawner, FreezeOnDeath,
    // MummyDeathBomb) is added in a later stage.
}

} // namespace tc
