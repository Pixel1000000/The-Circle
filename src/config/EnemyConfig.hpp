#pragma once

#include <string>
#include <vector>

#include "ecs/Components.hpp"
#include "world/BiomeType.hpp"

namespace tc {

// Blueprint for a regular enemy. One entry per mob type in assets/config/enemies.json
struct EnemyTemplate {
    std::string id;
    BiomeType biome = BiomeType::FOREST;

    int health = 10;
    int damage = 1;
    int armor = 0;
    float speed = 80.0f;

    AIBehavior::Type behavior = AIBehavior::CHASE;

    bool isRanged = false;
    float range = 40.0f;
    float cooldown = 1.0f;
    float projectileSpeed = 0.0f;

    bool appliesStatusEffect = false;
    StatusEffect::Type statusEffectType = StatusEffect::POISON;
    float statusEffectDps = 0.0f;
    float statusEffectDuration = 0.0f;

    bool revivesOnce = false;
    bool phaseThrough = false;
    bool explodesOnDeath = false;

    Element element = Element::NONE;
    float elementPercent = 0.0f;

    int cost = 3;

    float keyFragmentDropChance = 0.0f;
    float equipmentDropChance = 0.1f;

    sf::Color color = sf::Color::Red;
    sf::Vector2f size = {32.0f, 32.0f};
};

// Blueprint for a final-boss-room boss. One entry per theme in assets/config/bosses.json
struct BossTemplate {
    std::string id;
    BiomeType theme = BiomeType::FOREST;

    int health = 500;
    int damage = 20;
    int armor = 5;
    float speed = 60.0f;

    AIBehavior::Type behavior = AIBehavior::BOSS_DRUID;

    sf::Color color = sf::Color::Magenta;
    sf::Vector2f size = {64.0f, 64.0f};
};

// Loaded from assets/config/enemies.json and assets/config/bosses.json
struct EnemyConfig {
    std::vector<EnemyTemplate> enemies;
    std::vector<BossTemplate> bosses;

    std::vector<EnemyTemplate> getEnemiesForBiome(BiomeType biome) const;
    const BossTemplate* getBossForTheme(BiomeType theme) const;
};

} // namespace tc
