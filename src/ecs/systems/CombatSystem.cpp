#include "ecs/systems/CombatSystem.hpp"

#include <algorithm>
#include <vector>

#include "ecs/Components.hpp"
#include "ecs/EntityFactory.hpp"

namespace tc {

namespace {
constexpr float PROJECTILE_HIT_RADIUS = 20.0f;
}

void CombatSystem::update(entt::registry& registry, float dt)
{
    updateMelee(registry, dt);
    updateRanged(registry, dt);
    updateProjectiles(registry);
}

void CombatSystem::updateMelee(entt::registry& registry, float dt)
{
    auto view = registry.view<MeleeCombat, Position, Damage>();
    for (auto entity : view) {
        auto& melee = view.get<MeleeCombat>(entity);
        melee.timer -= dt;
        if (melee.timer > 0.0f) continue;

        const auto& pos = view.get<Position>(entity);
        const bool isPlayer = registry.all_of<PlayerTag>(entity);

        Position origin = pos;
        if (melee.offset != 0.0f) {
            if (const auto* facing = registry.try_get<Facing>(entity)) {
                origin.x += facing->dx * melee.offset;
                origin.y += facing->dy * melee.offset;
            }
        }

        entt::entity target = entt::null;
        float bestDistSq = melee.range * melee.range;

        auto tryTarget = [&](entt::entity candidate, const Position& tpos) {
            const float dx = tpos.x - origin.x;
            const float dy = tpos.y - origin.y;
            const float distSq = dx * dx + dy * dy;
            if (distSq <= bestDistSq) {
                bestDistSq = distSq;
                target = candidate;
            }
        };

        if (isPlayer) {
            for (auto candidate : registry.view<EnemyTag, Position, Health>()) {
                tryTarget(candidate, registry.get<Position>(candidate));
            }
        } else {
            for (auto candidate : registry.view<PlayerTag, Position, Health>()) {
                tryTarget(candidate, registry.get<Position>(candidate));
            }
        }

        if (target == entt::null) continue;

        applyDamage(registry, entity, target, view.get<Damage>(entity).value);
        melee.timer = melee.cooldown;

        if (const auto* effect = registry.try_get<StatusEffect>(entity)) {
            registry.emplace_or_replace<StatusEffect>(target, effect->type, effect->dps, effect->duration, effect->duration);
        }
    }
}

void CombatSystem::updateRanged(entt::registry& registry, float dt)
{
    auto view = registry.view<RangedCombat, Position, Damage>();
    for (auto entity : view) {
        auto& ranged = view.get<RangedCombat>(entity);
        ranged.timer -= dt;
        if (ranged.timer > 0.0f) continue;

        const auto& pos = view.get<Position>(entity);
        const bool isPlayer = registry.all_of<PlayerTag>(entity);

        entt::entity target = entt::null;
        float bestDistSq = ranged.range * ranged.range;
        Position targetPos{};

        auto tryTarget = [&](entt::entity candidate, const Position& tpos) {
            const float dx = tpos.x - pos.x;
            const float dy = tpos.y - pos.y;
            const float distSq = dx * dx + dy * dy;
            if (distSq <= bestDistSq) {
                bestDistSq = distSq;
                target = candidate;
                targetPos = tpos;
            }
        };

        if (isPlayer) {
            for (auto candidate : registry.view<EnemyTag, Position, Health>()) {
                tryTarget(candidate, registry.get<Position>(candidate));
            }
        } else {
            for (auto candidate : registry.view<PlayerTag, Position, Health>()) {
                tryTarget(candidate, registry.get<Position>(candidate));
            }
        }

        if (target == entt::null) continue;

        const sf::Vector2f direction{targetPos.x - pos.x, targetPos.y - pos.y};
        const int ownerBiome = isPlayer ? 0 : registry.get<EnemyTag>(entity).biome;

        EntityFactory::createProjectile(registry, {pos.x, pos.y}, direction,
            ranged.projectileSpeed, view.get<Damage>(entity).value, ownerBiome);

        ranged.timer = ranged.cooldown;
    }
}

void CombatSystem::updateProjectiles(entt::registry& registry)
{
    std::vector<entt::entity> toDestroy;
    constexpr float hitDistSq = PROJECTILE_HIT_RADIUS * PROJECTILE_HIT_RADIUS;

    auto view = registry.view<ProjectileTag, Position, Damage>();
    for (auto entity : view) {
        const auto& pos = view.get<Position>(entity);
        const int ownerBiome = view.get<ProjectileTag>(entity).ownerBiome;
        const int damage = view.get<Damage>(entity).value;

        entt::entity hit = entt::null;

        if (ownerBiome == 0) {
            for (auto candidate : registry.view<EnemyTag, Position, Health>()) {
                const auto& tpos = registry.get<Position>(candidate);
                const float dx = tpos.x - pos.x;
                const float dy = tpos.y - pos.y;
                if (dx * dx + dy * dy <= hitDistSq) {
                    hit = candidate;
                    break;
                }
            }
        } else {
            for (auto candidate : registry.view<PlayerTag, Position, Health>()) {
                const auto& tpos = registry.get<Position>(candidate);
                const float dx = tpos.x - pos.x;
                const float dy = tpos.y - pos.y;
                if (dx * dx + dy * dy <= hitDistSq) {
                    hit = candidate;
                    break;
                }
            }
        }

        if (hit != entt::null) {
            applyDamage(registry, entity, hit, damage);
            toDestroy.push_back(entity);
        }
    }

    for (auto entity : toDestroy) {
        registry.destroy(entity);
    }
}

void CombatSystem::applyDamage(entt::registry& registry, entt::entity attacker, entt::entity target, int rawDamage)
{
    int armor = 0;
    if (const auto* a = registry.try_get<Armor>(target)) {
        armor = a->value;
    }

    float reduction = 0.0f;
    if (const auto* block = registry.try_get<BlockAbility>(target)) {
        if (block->isBlocking) {
            reduction = block->reductionPercent;
        }
    }

    int mitigated = static_cast<int>(static_cast<float>(rawDamage - armor) * (1.0f - reduction));
    mitigated = std::max(mitigated, 1);

    auto& health = registry.get<Health>(target);
    health.current = std::max(health.current - mitigated, 0);

    registry.emplace_or_replace<HitFlash>(target);

    if (health.current == 0) {
        if (auto* potion = registry.try_get<Potion>(attacker)) {
            ++potion->killCounter;
            if (potion->killCounter >= potion->killsPerCharge) {
                potion->killCounter = 0;
                potion->charges = std::min(potion->charges + 1, potion->maxCharges);
            }
        }
    }
}

bool CombatSystem::usePotion(entt::registry& registry, entt::entity player)
{
    auto* potion = registry.try_get<Potion>(player);
    if (!potion || potion->charges <= 0) return false;

    auto& health = registry.get<Health>(player);
    if (health.current >= health.max) return false;

    --potion->charges;
    health.current = std::min(health.current + potion->healAmount, health.max);
    return true;
}

} // namespace tc
