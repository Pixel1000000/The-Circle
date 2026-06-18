#include "ecs/systems/CombatSystem.hpp"

#include <algorithm>
#include <random>
#include <vector>

#include "ecs/Components.hpp"
#include "ecs/ElementalUtils.hpp"
#include "ecs/EntityFactory.hpp"

namespace tc {

namespace {
constexpr float PROJECTILE_HIT_RADIUS = 20.0f;
constexpr float WORLD_WIDTH = 1280.0f;
constexpr float WORLD_HEIGHT = 720.0f;
constexpr float MAX_DAMAGE_REDUCTION = 0.9f;

std::mt19937& debuffRng()
{
    static std::mt19937 instance{std::random_device{}()};
    return instance;
}

bool rollChance(float chance)
{
    if (chance >= 1.0f) return true;
    if (chance <= 0.0f) return false;
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(debuffRng()) < chance;
}

// Debug One Shot toggle stores its override here instead of mutating
// Damage.value directly, so equipment recalculations can't silently clobber
// it (see DebugPlayerPanel).
int attackDamageOf(entt::registry& registry, entt::entity attacker, int storedDamage)
{
    if (const auto* override_ = registry.try_get<DamageOverride>(attacker)) {
        return override_->value;
    }
    return storedDamage;
}
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

        const float rangeSq = melee.range * melee.range;
        const int damage = attackDamageOf(registry, entity, view.get<Damage>(entity).value);

        if (isPlayer) {
            // The player's melee attack hits every enemy within range, not
            // just the nearest one.
            bool hitAny = false;
            for (auto candidate : registry.view<EnemyTag, Position, Health>()) {
                const auto& tpos = registry.get<Position>(candidate);
                const float dx = tpos.x - origin.x;
                const float dy = tpos.y - origin.y;
                if (dx * dx + dy * dy > rangeSq) continue;

                applyDamage(registry, entity, candidate, damage);
                if (const auto* effect = registry.try_get<StatusEffect>(entity)) {
                    registry.emplace_or_replace<StatusEffect>(candidate, effect->type, effect->dps, effect->duration, effect->duration);
                }
                hitAny = true;
            }

            if (!hitAny) continue;
        } else {
            entt::entity target = entt::null;
            float bestDistSq = rangeSq;

            for (auto candidate : registry.view<PlayerTag, Position, Health>()) {
                const auto& tpos = registry.get<Position>(candidate);
                const float dx = tpos.x - origin.x;
                const float dy = tpos.y - origin.y;
                const float distSq = dx * dx + dy * dy;
                if (distSq <= bestDistSq) {
                    bestDistSq = distSq;
                    target = candidate;
                }
            }

            // Yeti berserk: also consider other enemies as melee targets,
            // attacking whichever unit (player or enemy) is nearest.
            if (registry.all_of<AggroNearestUnit>(entity)) {
                for (auto candidate : registry.view<EnemyTag, Position, Health>()) {
                    if (candidate == entity) continue;
                    const auto& candidateHealth = registry.get<Health>(candidate);
                    if (candidateHealth.current <= 0) continue;
                    const auto& tpos = registry.get<Position>(candidate);
                    const float dx = tpos.x - origin.x;
                    const float dy = tpos.y - origin.y;
                    const float distSq = dx * dx + dy * dy;
                    if (distSq <= bestDistSq) {
                        bestDistSq = distSq;
                        target = candidate;
                    }
                }
            }

            if (target == entt::null) continue;

            applyDamage(registry, entity, target, damage);
            if (const auto* effect = registry.try_get<StatusEffect>(entity)) {
                if (rollChance(effect->applicationChance)) {
                    registry.emplace_or_replace<StatusEffect>(target, effect->type, effect->dps, effect->duration, effect->duration);
                }
            }
        }

        melee.timer = melee.cooldown;
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

        ElementalEffect projectileElement{};
        if (const auto* equip = registry.try_get<Equipment>(entity)) {
            projectileElement.element = equip->weaponElement;
            projectileElement.percent = equip->weaponElementPercent;
        } else if (const auto* effect = registry.try_get<ElementalEffect>(entity)) {
            projectileElement = *effect;
        }

        EntityFactory::createProjectile(registry, {pos.x, pos.y}, direction,
            ranged.projectileSpeed, attackDamageOf(registry, entity, view.get<Damage>(entity).value),
            ownerBiome, projectileElement);

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

        if (pos.x < 0.0f || pos.x > WORLD_WIDTH || pos.y < 0.0f || pos.y > WORLD_HEIGHT) {
            toDestroy.push_back(entity);
            continue;
        }

        const auto& tag = view.get<ProjectileTag>(entity);
        const int ownerBiome = tag.ownerBiome;
        const int damage = view.get<Damage>(entity).value;

        entt::entity hit = entt::null;

        auto findHitAmong = [&](auto candidateView) {
            for (auto candidate : candidateView) {
                const auto& tpos = registry.get<Position>(candidate);
                const float dx = tpos.x - pos.x;
                const float dy = tpos.y - pos.y;
                if (dx * dx + dy * dy <= hitDistSq) {
                    hit = candidate;
                    return;
                }
            }
        };

        if (tag.hitsAnyUnit) {
            findHitAmong(registry.view<PlayerTag, Position, Health>());
            if (hit == entt::null) {
                findHitAmong(registry.view<EnemyTag, Position, Health>());
            }
        } else if (ownerBiome == 0) {
            findHitAmong(registry.view<EnemyTag, Position, Health>());
        } else {
            findHitAmong(registry.view<PlayerTag, Position, Health>());
        }

        if (hit != entt::null) {
            applyDamage(registry, entity, hit, damage);
            if (const auto* slow = registry.try_get<SlowOnHit>(entity)) {
                const auto* resist = registry.try_get<ElementalResist>(hit);
                if (!resist || resist->slowResist < 1.0f) {
                    registry.emplace_or_replace<StatusEffect>(hit, StatusEffect::SLOW, 0.0f, slow->duration, slow->duration);
                }
            }
            toDestroy.push_back(entity);
        }
    }

    for (auto entity : toDestroy) {
        registry.destroy(entity);
    }
}

void CombatSystem::applyDamage(entt::registry& registry, entt::entity attacker, entt::entity target, int rawDamage)
{
    if (registry.all_of<Invulnerable>(target)) return;

    int armor = 0;
    float reduction = 0.0f;
    if (const auto* a = registry.try_get<Armor>(target)) {
        armor = a->value;
        reduction += a->damageReductionPercent;
    }

    if (const auto* block = registry.try_get<BlockAbility>(target)) {
        if (block->isBlocking) {
            reduction += block->reductionPercent;
        }
    }
    reduction = std::min(reduction, MAX_DAMAGE_REDUCTION);

    int mitigated = static_cast<int>(static_cast<float>(rawDamage - armor) * (1.0f - reduction));
    mitigated = std::max(mitigated, 1);

    // Ghost: a chance to absorb the hit entirely, banking it for a periodic
    // AOE release handled by AbilitySystem, instead of losing HP.
    if (auto* absorb = registry.try_get<AbsorbChance>(target)) {
        static std::mt19937 absorbRng{std::random_device{}()};
        std::uniform_real_distribution<float> roll(0.0f, 1.0f);
        if (roll(absorbRng) < absorb->chance) {
            absorb->accumulatedDamage += static_cast<float>(mitigated);
            registry.emplace_or_replace<HitFlash>(target);
            return;
        }
    }

    auto& health = registry.get<Health>(target);
    health.current = std::max(health.current - mitigated, 0);

    registry.emplace_or_replace<HitFlash>(target);

    // Yeti berserk: recoils 50% of the damage it deals back onto itself.
    if (const auto* rage = registry.try_get<RageAbility>(attacker)) {
        if (rage->enraged && !registry.all_of<Invulnerable>(attacker)) {
            auto& attackerHealth = registry.get<Health>(attacker);
            const int recoil = std::max(1, static_cast<int>(std::round(mitigated * 0.5f)));
            attackerHealth.current = std::max(attackerHealth.current - recoil, 0);
            registry.emplace_or_replace<HitFlash>(attacker);
        }
    }

    if (health.current == 0) {
        if (auto* potion = registry.try_get<Potion>(attacker)) {
            ++potion->killCounter;
            if (potion->killCounter >= potion->killsPerCharge) {
                potion->killCounter = 0;
                potion->charges = std::min(potion->charges + 1, potion->maxCharges);
            }
        }
        return;
    }

    Element element = Element::NONE;
    float percent = 0.0f;
    float elemChance = 1.0f;
    if (const auto* equip = registry.try_get<Equipment>(attacker)) {
        element = equip->weaponElement;
        percent = equip->weaponElementPercent;
        // Player weapon always has full chance
    } else if (const auto* effect = registry.try_get<ElementalEffect>(attacker)) {
        element = effect->element;
        percent = effect->percent;
        elemChance = effect->applicationChance;
    }

    if (element != Element::NONE && rollChance(elemChance)) {
        ElementalUtils::applyOnHit(registry, target, element, percent, rawDamage);
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
