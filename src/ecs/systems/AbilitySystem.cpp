#include "ecs/systems/AbilitySystem.hpp"

#include <algorithm>
#include <cmath>
#include <random>

#include "config/ConfigLoader.hpp"
#include "ecs/Components.hpp"
#include "ecs/EntityFactory.hpp"

namespace tc {

namespace {

constexpr float DASH_SPEED = 480.0f;
constexpr float DASH_COOLDOWN_JITTER = 2.0f; // wolves don't dash in lockstep
constexpr float CHARGE_TRIGGER_RANGE = 260.0f;
constexpr float CHARGE_DURATION = 0.5f;
constexpr int MAX_CONCURRENT_QUICKSAND = 3;
constexpr float ABSORB_RELEASE_RADIUS = 150.0f;
constexpr float TELEPORT_MIN_OFFSET = 150.0f;
constexpr float TELEPORT_MAX_OFFSET = 260.0f;

std::mt19937& rng()
{
    static std::mt19937 instance{std::random_device{}()};
    return instance;
}

sf::Vector2f directionTo(const Position& from, const Position& to)
{
    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length < 0.0001f) return {0.0f, 0.0f};
    return {dx / length, dy / length};
}

float distanceBetween(const Position& a, const Position& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

float healthFraction(const Health& health)
{
    return health.max > 0 ? static_cast<float>(health.current) / static_cast<float>(health.max) : 0.0f;
}

const EnemyTemplate* findTemplateById(const std::string& id)
{
    for (const auto& tmpl : ConfigLoader::get().getEnemyConfig().enemies) {
        if (tmpl.id == id) return &tmpl;
    }
    return nullptr;
}

void spawnTrapRing(entt::registry& registry, sf::Vector2f center, const TrapSpawner& spawner)
{
    for (int i = 0; i < spawner.count; ++i) {
        const float angle = (2.0f * 3.14159265f * static_cast<float>(i)) / static_cast<float>(spawner.count);
        const sf::Vector2f offset{std::cos(angle) * spawner.radius, std::sin(angle) * spawner.radius};

        const entt::entity trap = registry.create();
        registry.emplace<Position>(trap, center.x + offset.x, center.y + offset.y);
        registry.emplace<Renderable>(trap, sf::Color(140, 100, 40, 160), sf::Vector2f{24.0f, 24.0f});
        registry.emplace<TrapTag>(trap);
        registry.emplace<TrapZone>(trap, spawner.stunDuration);
        registry.emplace<ZoneDuration>(trap, 20.0f);
    }
}

} // namespace

void AbilitySystem::update(entt::registry& registry, entt::entity player, float dt,
    std::vector<entt::entity>& spawnedEnemies)
{
    if (!registry.valid(player)) return;
    const auto& playerPos = registry.get<Position>(player);

    // Wolf: periodic burst dash in a straight line, invulnerable mid-dash.
    // The direction is locked to the player's position at activation time
    // (no homing while dashing), and the cooldown gets a random jitter so a
    // pack of wolves doesn't all dash on the same beat.
    for (auto entity : registry.view<DashAbility, Position, Velocity, Health>()) {
        auto& dash = registry.get<DashAbility>(entity);
        const auto& pos = registry.get<Position>(entity);
        auto& health = registry.get<Health>(entity);
        if (health.current <= 0) continue;

        if (dash.dashing) {
            dash.durationTimer -= dt;
            registry.get<Position>(entity).x += dash.dirX * DASH_SPEED * dt;
            registry.get<Position>(entity).y += dash.dirY * DASH_SPEED * dt;
            if (dash.durationTimer <= 0.0f) {
                dash.dashing = false;
                std::uniform_real_distribution<float> jitterDist(0.0f, DASH_COOLDOWN_JITTER);
                dash.timer = dash.cooldown + jitterDist(rng());
                registry.remove<Invulnerable>(entity);
            }
        } else {
            dash.timer -= dt;
            if (dash.timer <= 0.0f) {
                dash.dashing = true;
                dash.durationTimer = dash.duration;
                const sf::Vector2f dir = directionTo(pos, playerPos);
                dash.dirX = dir.x;
                dash.dirY = dir.y;
                registry.emplace_or_replace<Invulnerable>(entity);
            }
        }
    }

    // Scorpion: at <30% HP gets one (50%-chance) opportunity per life to
    // burrow. On success it teleports a medium distance in a random
    // direction, leaves a quicksand zone at its old spot, and regenerates
    // HP while burrowed. It surfaces and resumes chasing after maxDuration
    // seconds, or surfaces immediately and dashes once in a straight line
    // to the player's last known position if the player gets within
    // dashRange while it's still burrowed.
    for (auto entity : registry.view<BurrowAbility, Position, Velocity, Health>()) {
        auto& burrow = registry.get<BurrowAbility>(entity);
        auto& pos = registry.get<Position>(entity);
        auto& health = registry.get<Health>(entity);
        if (health.current <= 0) continue;

        if (burrow.dashing) {
            pos.x += burrow.dashDirX * burrow.dashSpeed * dt;
            pos.y += burrow.dashDirY * burrow.dashSpeed * dt;
            burrow.timer -= dt;
            if (burrow.timer <= 0.0f) {
                burrow.dashing = false;
                registry.remove<Invulnerable>(entity);
            }
            continue;
        }

        if (burrow.burrowed) {
            burrow.timer -= dt;
            registry.get<Velocity>(entity) = {0.0f, 0.0f};
            health.current = std::min(health.max, health.current
                + static_cast<int>(std::round(health.max * burrow.regenPercentPerSecond * dt)));

            if (distanceBetween(pos, playerPos) <= burrow.dashRange) {
                burrow.burrowed = false;
                burrow.dashing = true;
                burrow.timer = burrow.dashDuration;
                const sf::Vector2f dir = directionTo(pos, playerPos);
                burrow.dashDirX = dir.x;
                burrow.dashDirY = dir.y;
                registry.emplace_or_replace<Invulnerable>(entity);
            } else if (burrow.timer <= 0.0f) {
                burrow.burrowed = false;
                registry.remove<Invulnerable>(entity);
            }
        } else if (!burrow.usedOnce && healthFraction(health) < burrow.hpThreshold) {
            burrow.usedOnce = true;
            std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
            if (chanceDist(rng()) <= burrow.burrowChance) {
                const entt::entity zone = registry.create();
                registry.emplace<Position>(zone, pos.x, pos.y);
                registry.emplace<Renderable>(zone, sf::Color(210, 180, 100, 130), sf::Vector2f{40.0f, 40.0f});
                registry.emplace<QuicksandTag>(zone);
                registry.emplace<ZoneDuration>(zone, 4.0f);

                std::uniform_real_distribution<float> offsetDist(burrow.teleportMinOffset, burrow.teleportMaxOffset);
                std::uniform_real_distribution<float> angleDist(0.0f, 6.2831853f);
                const float angle = angleDist(rng());
                const float offset = offsetDist(rng());
                pos.x += std::cos(angle) * offset;
                pos.y += std::sin(angle) * offset;

                burrow.burrowed = true;
                burrow.timer = burrow.maxDuration;
                registry.emplace_or_replace<Invulnerable>(entity);
            }
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

    // Wasp swarm: scatters away from the player briefly after taking damage.
    for (auto entity : registry.view<SwarmScatter, Position, Velocity, Health>()) {
        auto& scatter = registry.get<SwarmScatter>(entity);
        const auto& pos = registry.get<Position>(entity);
        auto& health = registry.get<Health>(entity);
        if (health.current <= 0) continue;

        if (scatter.scattered) {
            scatter.timer -= dt;
            const sf::Vector2f away = directionTo(playerPos, pos);
            registry.get<Velocity>(entity) = {away.x, away.y};
            if (scatter.timer <= 0.0f) {
                scatter.scattered = false;
            }
        } else if (registry.all_of<HitFlash>(entity)) {
            scatter.scattered = true;
            scatter.timer = scatter.duration;
        }
    }

    // Bone golem: periodically detaches a skeleton minion while below the
    // HP threshold, up to maxDetach times.
    for (auto entity : registry.view<BoneDetach, Position, Health>()) {
        auto& detach = registry.get<BoneDetach>(entity);
        const auto& pos = registry.get<Position>(entity);
        auto& health = registry.get<Health>(entity);
        if (health.current <= 0) continue;
        if (detach.detached >= detach.maxDetach) continue;

        if (healthFraction(health) >= detach.hpThreshold) continue;

        detach.timer -= dt;
        if (detach.timer <= 0.0f) {
            detach.timer = detach.interval;
            if (const auto* skeletonTmpl = findTemplateById("deadlands_skeleton")) {
                std::uniform_real_distribution<float> offsetDist(-40.0f, 40.0f);
                const sf::Vector2f spawnPos{pos.x + offsetDist(rng()), pos.y + offsetDist(rng())};
                const entt::entity minion = EntityFactory::createEnemy(registry, *skeletonTmpl, spawnPos);
                spawnedEnemies.push_back(minion);
                ++detach.detached;
            }
        }
    }

    // Sand spirit: periodically spawns a quicksand zone near the player,
    // capped at MAX_CONCURRENT_QUICKSAND concurrent zones on the map.
    for (auto entity : registry.view<QuicksandSpawner, Health>()) {
        auto& spawner = registry.get<QuicksandSpawner>(entity);
        auto& health = registry.get<Health>(entity);
        if (health.current <= 0) continue;

        spawner.timer -= dt;
        if (spawner.timer <= 0.0f) {
            spawner.timer = spawner.cooldown;
            const auto quicksandCount = registry.view<QuicksandTag>().size();
            if (quicksandCount < static_cast<std::size_t>(MAX_CONCURRENT_QUICKSAND)) {
                std::uniform_real_distribution<float> offsetDist(-100.0f, 100.0f);
                const entt::entity zone = registry.create();
                registry.emplace<Position>(zone, playerPos.x + offsetDist(rng()), playerPos.y + offsetDist(rng()));
                registry.emplace<Renderable>(zone, sf::Color(210, 180, 100, 130), spawner.size);
                registry.emplace<QuicksandTag>(zone);
                registry.emplace<ZoneDuration>(zone, spawner.duration);
            }
        }
    }

    // Snow witch / Ice spirit: periodically teleports near the player,
    // leaving a slowing ice zone at its previous position.
    for (auto entity : registry.view<TeleportAbility, Position, Health>()) {
        auto& teleport = registry.get<TeleportAbility>(entity);
        auto& health = registry.get<Health>(entity);
        if (health.current <= 0) continue;

        teleport.timer -= dt;
        if (teleport.timer <= 0.0f) {
            teleport.timer = teleport.cooldown;
            auto& pos = registry.get<Position>(entity);

            const entt::entity zone = registry.create();
            registry.emplace<Position>(zone, pos.x, pos.y);
            registry.emplace<Renderable>(zone, sf::Color(160, 220, 255, 130), sf::Vector2f{60.0f, 60.0f});
            registry.emplace<IceZoneTag>(zone);
            registry.emplace<ZoneDuration>(zone, 4.0f);

            std::uniform_real_distribution<float> offsetDist(TELEPORT_MIN_OFFSET, TELEPORT_MAX_OFFSET);
            std::uniform_real_distribution<float> angleDist(0.0f, 6.2831853f);
            const float angle = angleDist(rng());
            const float offset = offsetDist(rng());
            pos.x = playerPos.x + std::cos(angle) * offset;
            pos.y = playerPos.y + std::sin(angle) * offset;
        }
    }

    // Ice spirit: periodically fires a frost pulse that freezes the player if
    // they are within range. This is the ONLY source of ICE freeze from the
    // ice spirit — regular melee hits carry no elemental on this enemy.
    for (auto entity : registry.view<IcePulseAbility, Position, Health>()) {
        auto& pulse = registry.get<IcePulseAbility>(entity);
        auto& health = registry.get<Health>(entity);
        if (health.current <= 0) continue;

        pulse.timer -= dt;
        if (pulse.timer <= 0.0f) {
            pulse.timer = pulse.cooldown;
            if (distanceBetween(registry.get<Position>(entity), playerPos) <= pulse.range) {
                const auto* resist = registry.try_get<ElementalResist>(player);
                if (!resist || resist->slowResist < 1.0f) {
                    const float duration = pulse.slowDuration * (resist ? (1.0f - resist->slowResist) : 1.0f);
                    registry.emplace_or_replace<StatusEffect>(player, StatusEffect::SLOW, 0.0f, duration, duration);
                    registry.emplace_or_replace<IceChill>(player, duration);
                }
            }
        }
    }

    // Ghost: periodically releases accumulated absorbed damage as a burst
    // against the player if still nearby.
    for (auto entity : registry.view<AbsorbChance, Position, Health>()) {
        auto& absorb = registry.get<AbsorbChance>(entity);
        const auto& pos = registry.get<Position>(entity);
        auto& health = registry.get<Health>(entity);
        if (health.current <= 0) continue;

        absorb.releaseTimer -= dt;
        if (absorb.releaseTimer <= 0.0f) {
            absorb.releaseTimer = absorb.releaseInterval;
            if (absorb.accumulatedDamage > 0.0f) {
                if (distanceBetween(pos, playerPos) <= ABSORB_RELEASE_RADIUS
                    && !registry.all_of<Invulnerable>(player)) {
                    auto& playerHealth = registry.get<Health>(player);
                    const int burst = static_cast<int>(absorb.accumulatedDamage);
                    playerHealth.current = std::max(playerHealth.current - burst, 0);
                    registry.emplace_or_replace<HitFlash>(player);
                }
                absorb.accumulatedDamage = 0.0f;
            }
        }
    }
}

void AbilitySystem::handleDeathEffects(entt::registry& registry, entt::entity player, float dt)
{
    const bool playerValid = registry.valid(player);
    const Position playerPos = playerValid ? registry.get<Position>(player) : Position{};

    // Ant: spawns a ring of traps at its death position.
    for (auto entity : registry.view<TrapSpawner, Health, Position>()) {
        auto& health = registry.get<Health>(entity);
        if (health.current > 0) continue;
        const auto& pos = registry.get<Position>(entity);
        spawnTrapRing(registry, {pos.x, pos.y}, registry.get<TrapSpawner>(entity));
    }

    // Ice goblin: on death, freezes/stuns the player if within range.
    for (auto entity : registry.view<FreezeOnDeath, Health, Position>()) {
        auto& health = registry.get<Health>(entity);
        if (health.current > 0) continue;
        if (!playerValid) continue;

        const auto& pos = registry.get<Position>(entity);
        if (distanceBetween(pos, playerPos) <= registry.get<FreezeOnDeath>(entity).stunRadius) {
            const auto& freeze = registry.get<FreezeOnDeath>(entity);
            registry.emplace_or_replace<Stunned>(player, freeze.stunDuration);
            const auto* resist = registry.try_get<ElementalResist>(player);
            if (!resist || resist->slowResist < 1.0f) {
                registry.emplace_or_replace<StatusEffect>(player, StatusEffect::SLOW, 0.0f, freeze.freezeDuration, freeze.freezeDuration);
            }
        }
    }

    // Mummy: telegraphs death with a few blinks (staying alive at 1 HP,
    // invulnerable) before exploding into an AOE stun.
    for (auto entity : registry.view<MummyDeathBomb, Health, Position>()) {
        auto& bomb = registry.get<MummyDeathBomb>(entity);
        auto& health = registry.get<Health>(entity);
        if (bomb.triggered) continue;

        if (bomb.timer <= 0.0f && health.current <= 0) {
            bomb.timer = bomb.delay;
            bomb.currentBlink = 0;
            bomb.blinkTimer = bomb.blinkInterval;
            health.current = 1;
            registry.emplace_or_replace<Invulnerable>(entity);
            continue;
        }

        if (bomb.timer > 0.0f) {
            bomb.timer -= dt;
            bomb.blinkTimer -= dt;
            if (bomb.blinkTimer <= 0.0f) {
                bomb.blinkTimer = bomb.blinkInterval;
                ++bomb.currentBlink;
            }
            health.current = 1;

            if (bomb.timer <= 0.0f) {
                bomb.triggered = true;
                registry.remove<Invulnerable>(entity);
                health.current = 0;

                const auto& pos = registry.get<Position>(entity);
                if (playerValid && distanceBetween(pos, playerPos) <= bomb.radius) {
                    registry.emplace_or_replace<Stunned>(player, bomb.stunDuration);
                }
            }
        }
    }
}

} // namespace tc
