#include "ecs/systems/BlizzardSystem.hpp"

#include <algorithm>
#include <cmath>

#include "Game.hpp"
#include "config/ConfigLoader.hpp"
#include "ecs/Components.hpp"

namespace tc {

namespace {
constexpr float SLOW_DURATION = 0.2f;
} // namespace

void BlizzardSystem::update(entt::registry& registry, entt::entity player, float dt)
{
    const float driftSpeed = ConfigLoader::get().getElementalConfig().blizzard.driftSpeed;

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
            // Large zone slowly chases the player.
            const float dx = playerPos.x - pos.x;
            const float dy = playerPos.y - pos.y;
            const float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > 1.0f) {
                zone.driftDir = {dx / dist, dy / dist};
            }
            pos.x += zone.driftDir.x * driftSpeed * dt;
            pos.y += zone.driftDir.y * driftSpeed * dt;

            // Clamp to screen bounds
            const float hw = size.x * 0.5f;
            const float hh = size.y * 0.5f;
            pos.x = std::clamp(pos.x, hw, W - hw);
            pos.y = std::clamp(pos.y, hh, H - hh);
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
