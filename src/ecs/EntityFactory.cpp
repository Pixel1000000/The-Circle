#include "ecs/EntityFactory.hpp"

#include <algorithm>
#include <cmath>

namespace tc {

namespace {

constexpr int HEALTH_PER_META_LEVEL = 10;
constexpr int ARMOR_PER_ENDURANCE = 1;
constexpr int DAMAGE_PER_STRENGTH = 1;

const ArmorPieceStats& pieceStats(const std::array<ArmorPieceStats, TIER_COUNT>& tiers, int tier)
{
    static const ArmorPieceStats empty{};
    if (tier < 1 || tier > TIER_COUNT) return empty;
    return tiers[static_cast<std::size_t>(tier - 1)];
}

const WeaponStats& weaponStats(const std::array<WeaponStats, TIER_COUNT>& tiers, int tier)
{
    static const WeaponStats empty{};
    if (tier < 1 || tier > TIER_COUNT) return empty;
    return tiers[static_cast<std::size_t>(tier - 1)];
}

} // namespace

entt::entity EntityFactory::createPlayer(entt::registry& registry, sf::Vector2f position,
    const PlayerConfig& playerConfig, const MetaStats& metaStats)
{
    entt::entity entity = registry.create();

    registry.emplace<PlayerTag>(entity);
    registry.emplace<Position>(entity, position.x, position.y);
    registry.emplace<Velocity>(entity, 0.0f, 0.0f);
    registry.emplace<Facing>(entity, 0.0f, 1.0f);
    registry.emplace<Speed>(entity, playerConfig.baseSpeed);
    registry.emplace<Armor>(entity, 0);
    registry.emplace<Damage>(entity, 0);
    registry.emplace<Renderable>(entity, sf::Color(80, 220, 160), sf::Vector2f(32.0f, 32.0f));
    registry.emplace<KeyFragmentHolder>(entity, 0);
    registry.emplace<Equipment>(entity);
    registry.emplace<Potion>(entity, playerConfig.potionHealAmount, 0, playerConfig.potionMaxCharges,
        playerConfig.potionKillsPerCharge, 0);

    const int maxHealth = playerConfig.baseHealth + metaStats.health * HEALTH_PER_META_LEVEL;
    registry.emplace<Health>(entity, maxHealth, maxHealth);

    applyEquipmentStats(registry, entity, playerConfig, EquipmentConfig{}, metaStats);

    return entity;
}

entt::entity EntityFactory::createEnemy(entt::registry& registry, const EnemyTemplate& tmpl, sf::Vector2f position)
{
    entt::entity entity = registry.create();

    registry.emplace<EnemyTag>(entity, biomeTier(tmpl.biome));
    registry.emplace<Position>(entity, position.x, position.y);
    registry.emplace<Velocity>(entity, 0.0f, 0.0f);
    registry.emplace<Speed>(entity, tmpl.speed);
    registry.emplace<Health>(entity, tmpl.health, tmpl.health);
    registry.emplace<Armor>(entity, tmpl.armor);
    registry.emplace<Damage>(entity, tmpl.damage);
    registry.emplace<AIBehavior>(entity, tmpl.behavior);
    registry.emplace<Renderable>(entity, tmpl.color, tmpl.size);
    registry.emplace<KeyFragmentDrop>(entity, tmpl.keyFragmentDropChance);

    if (tmpl.isRanged) {
        registry.emplace<RangedCombat>(entity, tmpl.range, tmpl.projectileSpeed, tmpl.cooldown, 0.0f);
    } else {
        registry.emplace<MeleeCombat>(entity, tmpl.range, tmpl.cooldown, 0.0f);
    }

    if (tmpl.appliesStatusEffect) {
        registry.emplace<StatusEffect>(entity, tmpl.statusEffectType, tmpl.statusEffectDps, tmpl.statusEffectDuration, 0.0f);
    }

    if (tmpl.revivesOnce) {
        registry.emplace<ReviveOnce>(entity, false);
    }

    if (tmpl.phaseThrough) {
        registry.emplace<PhaseThrough>(entity);
    }

    return entity;
}

entt::entity EntityFactory::createBoss(entt::registry& registry, const BossTemplate& tmpl, sf::Vector2f position)
{
    entt::entity entity = registry.create();

    registry.emplace<EnemyTag>(entity, biomeTier(tmpl.theme));
    registry.emplace<Position>(entity, position.x, position.y);
    registry.emplace<Velocity>(entity, 0.0f, 0.0f);
    registry.emplace<Speed>(entity, tmpl.speed);
    registry.emplace<Health>(entity, tmpl.health, tmpl.health);
    registry.emplace<Armor>(entity, tmpl.armor);
    registry.emplace<Damage>(entity, tmpl.damage);
    registry.emplace<AIBehavior>(entity, tmpl.behavior);
    registry.emplace<Renderable>(entity, tmpl.color, tmpl.size);
    registry.emplace<MeleeCombat>(entity, std::max(tmpl.size.x, tmpl.size.y), 1.0f, 0.0f);

    return entity;
}

entt::entity EntityFactory::createProjectile(entt::registry& registry, sf::Vector2f position,
    sf::Vector2f direction, float speed, int damage, int ownerBiome)
{
    entt::entity entity = registry.create();

    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length < 0.0001f) {
        direction = {1.0f, 0.0f};
        length = 1.0f;
    }
    direction /= length;

    registry.emplace<Position>(entity, position.x, position.y);
    registry.emplace<Velocity>(entity, direction.x, direction.y);
    registry.emplace<Speed>(entity, speed);
    registry.emplace<Damage>(entity, damage);
    registry.emplace<ProjectileTag>(entity, ownerBiome);

    const sf::Color color = ownerBiome == 0 ? sf::Color(120, 220, 255) : sf::Color(220, 80, 80);
    registry.emplace<Renderable>(entity, color, sf::Vector2f(8.0f, 8.0f));

    return entity;
}

void EntityFactory::applyEquipmentStats(entt::registry& registry, entt::entity player,
    const PlayerConfig& playerConfig, const EquipmentConfig& equipmentConfig, const MetaStats& metaStats)
{
    const auto& equipment = registry.get<Equipment>(player);

    const ArmorPieceStats& helmet = pieceStats(equipmentConfig.helmet, equipment.helmetTier);
    const ArmorPieceStats& chest = pieceStats(equipmentConfig.chest, equipment.chestTier);
    const ArmorPieceStats& leggings = pieceStats(equipmentConfig.leggings, equipment.leggingsTier);

    const int armorValue = helmet.armor + chest.armor + leggings.armor + metaStats.endurance * ARMOR_PER_ENDURANCE;
    registry.get<Armor>(player).value = armorValue;

    const int healthBonus = helmet.healthBonus + chest.healthBonus + leggings.healthBonus
        + metaStats.health * HEALTH_PER_META_LEVEL;
    const int newMax = playerConfig.baseHealth + healthBonus;

    auto& health = registry.get<Health>(player);
    const int delta = newMax - health.max;
    health.max = newMax;
    health.current = std::clamp(health.current + std::max(delta, 0), 0, health.max);

    const float speedBonus = helmet.speedBonus + leggings.speedBonus;
    registry.get<Speed>(player).value = playerConfig.baseSpeed + speedBonus;

    const int strengthDamage = metaStats.strength * DAMAGE_PER_STRENGTH;

    if (equipment.weaponType == Equipment::BOW) {
        const WeaponStats& bow = weaponStats(equipmentConfig.bow, equipment.weaponTier);

        const int baseDamage = bow.damage > 0 ? bow.damage : playerConfig.baseDamage;
        registry.get<Damage>(player).value = baseDamage + strengthDamage;

        const float cooldown = bow.attackSpeed > 0.0f ? 1.0f / bow.attackSpeed : playerConfig.rangedCooldown;
        const float range = bow.range > 0.0f ? bow.range : playerConfig.rangedRange;
        registry.emplace_or_replace<RangedCombat>(player, range, playerConfig.rangedProjectileSpeed, cooldown, 0.0f);

        registry.remove<MeleeCombat>(player);
        registry.remove<BlockAbility>(player);
    } else {
        const WeaponStats& sword = weaponStats(equipmentConfig.sword, equipment.weaponTier);

        const int baseDamage = sword.damage > 0 ? sword.damage : playerConfig.baseDamage;
        registry.get<Damage>(player).value = baseDamage + strengthDamage;

        const float cooldown = sword.attackSpeed > 0.0f ? 1.0f / sword.attackSpeed : playerConfig.meleeCooldown;
        const float range = sword.range > 0.0f ? sword.range : playerConfig.meleeRange;
        registry.emplace_or_replace<MeleeCombat>(player, range, cooldown, 0.0f, playerConfig.meleeOffset);

        registry.emplace_or_replace<BlockAbility>(player, sword.blockReduction, false);
        registry.remove<RangedCombat>(player);
    }
}

} // namespace tc
