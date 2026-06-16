#include "ecs/systems/BlizzardSystem.hpp"

#include <cmath>
#include <random>

#include "Game.hpp"
#include "ecs/Components.hpp"

namespace tc {

namespace {

constexpr float DRIFT_SPEED = 20.0f;
constexpr float DIR_CHANGE_MIN = 3.0f;
constexpr float DIR_CHANGE_MAX = 5.0f;
constexpr float SLOW_DURATION = 0.2f;

sf::Vector2f randomDir(std::mt19937& rng)
{
    std::uniform_real_distribution<float> ang(0.0f, 6.2832f);
    const float a = ang(rng);
    return {std::cos(a), std::sin(a)};
}

} // namespace

void BlizzardSystem::update(entt::registry& registry, entt::entity player, float dt)
{
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> changeDist(DIR_CHANGE_MIN, DIR_CHANGE_MAX);

    const float W = static_cast<float>(Game::LOGICAL_WIDTH);
    const float H = static_cast<float>(Game::LOGICAL_HEIGHT);

    const auto& playerPos = registry.get<Position>(player);
    const auto& playerSize = registry.get<Renderable>(player).size;

    bool playerInZone = false;

    for (auto entity : registry.view<BlizzardZoneTag, BlizzardZone, Position, Renderable>()) {
        auto& zone = registry.get<BlizzardZone>(entity);
        auto& pos  = registry.get<Position>(entity);
        const auto& size = registry.get<Renderable>(entity).size;

        if (zone.drifting) {
            zone.dirTimer -= dt;
            if (zone.dirTimer <= 0.0f) {
                zone.driftDir = randomDir(rng);
                zone.dirTimer = changeDist(rng);
            }
            pos.x += zone.driftDir.x * DRIFT_SPEED * dt;
            pos.y += zone.driftDir.y * DRIFT_SPEED * dt;

            // Bounce off edges
            const float hw = size.x * 0.5f;
            const float hh = size.y * 0.5f;
            if (pos.x - hw < 0.0f)  { pos.x = hw;     zone.driftDir.x =  std::abs(zone.driftDir.x); }
            if (pos.x + hw > W)     { pos.x = W - hw;  zone.driftDir.x = -std::abs(zone.driftDir.x); }
            if (pos.y - hh < 0.0f)  { pos.y = hh;     zone.driftDir.y =  std::abs(zone.driftDir.y); }
            if (pos.y + hh > H)     { pos.y = H - hh;  zone.driftDir.y = -std::abs(zone.driftDir.y); }
        }

        // AABB overlap with player
        const float overlapX = (playerSize.x + size.x) * 0.5f - std::abs(playerPos.x - pos.x);
        const float overlapY = (playerSize.y + size.y) * 0.5f - std::abs(playerPos.y - pos.y);
        if (overlapX > 0.0f && overlapY > 0.0f) {
            playerInZone = true;
        }
    }

    if (playerInZone) {
        const auto* resist = registry.try_get<ElementalResist>(player);
        if (!resist || resist->slowResist < 1.0f) {
            registry.emplace_or_replace<StatusEffect>(player, StatusEffect::SLOW, 0.0f, SLOW_DURATION, SLOW_DURATION);
        }
    }
}

} // namespace tc
