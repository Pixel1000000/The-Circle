#include "ecs/systems/ZoneSystem.hpp"

#include <cmath>
#include <vector>

#include "ecs/Components.hpp"

namespace tc {

namespace {

bool overlaps(const Position& a, const sf::Vector2f& sizeA, const Position& b, const sf::Vector2f& sizeB)
{
    const float overlapX = (sizeA.x + sizeB.x) * 0.5f - std::abs(a.x - b.x);
    const float overlapY = (sizeA.y + sizeB.y) * 0.5f - std::abs(a.y - b.y);
    return overlapX > 0.0f && overlapY > 0.0f;
}

} // namespace

void ZoneSystem::update(entt::registry& registry, entt::entity player, float dt)
{
    for (auto entity : registry.view<Stunned>()) {
        auto& stunned = registry.get<Stunned>(entity);
        stunned.timer -= dt;
        if (stunned.timer <= 0.0f) {
            registry.remove<Stunned>(entity);
        }
    }

    if (!registry.valid(player)) return;
    const auto& playerPos = registry.get<Position>(player);
    const auto& playerSize = registry.get<Renderable>(player).size;

    std::vector<entt::entity> expiredTraps;
    for (auto entity : registry.view<TrapTag, TrapZone, ZoneDuration, Position, Renderable>()) {
        auto& duration = registry.get<ZoneDuration>(entity);
        const auto& pos = registry.get<Position>(entity);
        const auto& size = registry.get<Renderable>(entity).size;
        const auto& trap = registry.get<TrapZone>(entity);

        if (overlaps(playerPos, playerSize, pos, size) && !registry.all_of<Invulnerable>(player)) {
            registry.emplace_or_replace<Stunned>(player, trap.stunDuration);
            expiredTraps.push_back(entity);
            continue;
        }

        // Traps are environmental hazards: any enemy that wanders into one
        // gets stunned too, not just the player.
        bool caughtEnemy = false;
        for (auto enemy : registry.view<EnemyTag, Position, Renderable, Health>()) {
            const auto& enemyHealth = registry.get<Health>(enemy);
            if (enemyHealth.current <= 0 || registry.all_of<Invulnerable>(enemy)) continue;
            const auto& enemyPos = registry.get<Position>(enemy);
            const auto& enemySize = registry.get<Renderable>(enemy).size;
            if (overlaps(enemyPos, enemySize, pos, size)) {
                registry.emplace_or_replace<Stunned>(enemy, trap.stunDuration);
                caughtEnemy = true;
                break;
            }
        }
        if (caughtEnemy) {
            expiredTraps.push_back(entity);
            continue;
        }

        duration.timer -= dt;
        if (duration.timer <= 0.0f) {
            expiredTraps.push_back(entity);
        }
    }
    for (auto entity : expiredTraps) {
        registry.destroy(entity);
    }

    std::vector<entt::entity> expiredQuicksand;
    for (auto entity : registry.view<QuicksandTag, ZoneDuration, Position, Renderable>()) {
        auto& duration = registry.get<ZoneDuration>(entity);
        const auto& pos = registry.get<Position>(entity);
        const auto& size = registry.get<Renderable>(entity).size;

        if (overlaps(playerPos, playerSize, pos, size)) {
            const auto* resist = registry.try_get<ElementalResist>(player);
            if (!resist || resist->slowResist < 1.0f) {
                registry.emplace_or_replace<StatusEffect>(player, StatusEffect::SLOW, 0.0f, 0.2f, 0.2f);
            }
        }

        duration.timer -= dt;
        if (duration.timer <= 0.0f) {
            expiredQuicksand.push_back(entity);
        }
    }
    for (auto entity : expiredQuicksand) {
        registry.destroy(entity);
    }

    std::vector<entt::entity> expiredIce;
    for (auto entity : registry.view<IceZoneTag, ZoneDuration, Position, Renderable>()) {
        auto& duration = registry.get<ZoneDuration>(entity);
        const auto& pos = registry.get<Position>(entity);
        const auto& size = registry.get<Renderable>(entity).size;

        if (overlaps(playerPos, playerSize, pos, size)) {
            const auto* resist = registry.try_get<ElementalResist>(player);
            if (!resist || resist->slowResist < 1.0f) {
                registry.emplace_or_replace<StatusEffect>(player, StatusEffect::SLOW, 0.0f, 0.2f, 0.2f);
            }
        }

        duration.timer -= dt;
        if (duration.timer <= 0.0f) {
            expiredIce.push_back(entity);
        }
    }
    for (auto entity : expiredIce) {
        registry.destroy(entity);
    }
}

} // namespace tc
