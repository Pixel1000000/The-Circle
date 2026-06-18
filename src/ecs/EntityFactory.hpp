#pragma once

#include <entt/entt.hpp>
#include <SFML/System/Vector2.hpp>

#include "config/EnemyConfig.hpp"
#include "config/EquipmentConfig.hpp"
#include "config/PlayerConfig.hpp"
#include "ecs/Components.hpp"

namespace tc {

// Creates fully-formed entities (player, enemies, bosses, projectiles) with
// the component sets expected by the ECS systems.
class EntityFactory {
public:
    static entt::entity createPlayer(entt::registry& registry, sf::Vector2f position,
        const PlayerConfig& playerConfig, const MetaStats& metaStats);

    static entt::entity createEnemy(entt::registry& registry, const EnemyTemplate& tmpl, sf::Vector2f position);

    static entt::entity createBoss(entt::registry& registry, const BossTemplate& tmpl, sf::Vector2f position);

    // ownerBiome == 0 means the projectile was fired by the player and damages enemies;
    // any other value means it was fired by an enemy from that biome and damages the player.
    // `element` carries the firing player's weapon element/percent (if any) so
    // CombatSystem can apply the on-hit elemental effect when the projectile lands.
    static entt::entity createProjectile(entt::registry& registry, sf::Vector2f position,
        sf::Vector2f direction, float speed, int damage, int ownerBiome, ElementalEffect element = {});

    // Ice goblin death-explosion shard: hits whichever unit (player or enemy)
    // it touches first, dealing `damage` and applying a SLOW for `slowDuration`.
    static entt::entity createIceShard(entt::registry& registry, sf::Vector2f position,
        sf::Vector2f direction, float speed, int damage, float slowDuration);

    // Recomputes Armor, Damage, Health, Speed, MeleeCombat/RangedCombat and
    // BlockAbility for the player from their current Equipment, MetaStats and
    // the base PlayerConfig. Call whenever Equipment or MetaStats change.
    static void applyEquipmentStats(entt::registry& registry, entt::entity player,
        const PlayerConfig& playerConfig, const EquipmentConfig& equipmentConfig, const MetaStats& metaStats);
};

} // namespace tc
