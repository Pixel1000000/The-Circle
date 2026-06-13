#include "config/ConfigLoader.hpp"

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

namespace tc {

namespace {

nlohmann::json loadJson(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[ConfigLoader] Could not open " << path << ", using defaults\n";
        return nlohmann::json::object();
    }

    try {
        nlohmann::json json;
        file >> json;
        return json;
    } catch (const std::exception& e) {
        std::cerr << "[ConfigLoader] Failed to parse " << path << ": " << e.what() << "\n";
        return nlohmann::json::object();
    }
}

BiomeType biomeFromString(const std::string& name)
{
    if (name == "desert") return BiomeType::DESERT;
    if (name == "winter") return BiomeType::WINTER;
    if (name == "deadlands") return BiomeType::DEADLANDS;
    return BiomeType::FOREST;
}

AIBehavior::Type behaviorFromString(const std::string& name)
{
    if (name == "RANGED_KEEP_DISTANCE") return AIBehavior::RANGED_KEEP_DISTANCE;
    if (name == "SWARM") return AIBehavior::SWARM;
    if (name == "BOSS_DRUID") return AIBehavior::BOSS_DRUID;
    if (name == "BOSS_NAGA") return AIBehavior::BOSS_NAGA;
    if (name == "BOSS_ELEMENTAL") return AIBehavior::BOSS_ELEMENTAL;
    if (name == "BOSS_LICH") return AIBehavior::BOSS_LICH;
    return AIBehavior::CHASE;
}

StatusEffect::Type statusEffectFromString(const std::string& name)
{
    if (name == "SLOW") return StatusEffect::SLOW;
    return StatusEffect::POISON;
}

sf::Color colorFromJson(const nlohmann::json& json, sf::Color fallback)
{
    if (!json.is_array() || json.size() < 3) {
        return fallback;
    }
    return sf::Color(
        json.at(0).get<sf::Uint8>(),
        json.at(1).get<sf::Uint8>(),
        json.at(2).get<sf::Uint8>());
}

sf::Vector2f vec2FromJson(const nlohmann::json& json, sf::Vector2f fallback)
{
    if (!json.is_array() || json.size() < 2) {
        return fallback;
    }
    return sf::Vector2f(json.at(0).get<float>(), json.at(1).get<float>());
}

ArmorPieceStats armorPieceFromJson(const nlohmann::json& json)
{
    ArmorPieceStats stats;
    stats.armor = json.value("armor", stats.armor);
    stats.healthBonus = json.value("healthBonus", stats.healthBonus);
    stats.speedBonus = json.value("speedBonus", stats.speedBonus);
    return stats;
}

WeaponStats weaponStatsFromJson(const nlohmann::json& json)
{
    WeaponStats stats;
    stats.damage = json.value("damage", stats.damage);
    stats.attackSpeed = json.value("attackSpeed", stats.attackSpeed);
    stats.blockReduction = json.value("blockReduction", stats.blockReduction);
    stats.range = json.value("range", stats.range);
    return stats;
}

} // namespace

ConfigLoader& ConfigLoader::get()
{
    static ConfigLoader instance;
    return instance;
}

void ConfigLoader::loadAll(const std::string& configDir)
{
    playerConfig = loadPlayer(configDir + "/player.json");
    equipmentConfig = loadEquipment(configDir + "/equipment.json");
    enemyConfig = loadEnemies(configDir + "/enemies.json", configDir + "/bosses.json");
}

const PlayerConfig& ConfigLoader::getPlayerConfig() const
{
    return playerConfig;
}

const EquipmentConfig& ConfigLoader::getEquipmentConfig() const
{
    return equipmentConfig;
}

const EnemyConfig& ConfigLoader::getEnemyConfig() const
{
    return enemyConfig;
}

PlayerConfig ConfigLoader::loadPlayer(const std::string& path) const
{
    PlayerConfig config;
    nlohmann::json json = loadJson(path);

    config.baseSpeed = json.value("baseSpeed", config.baseSpeed);
    config.baseHealth = json.value("baseHealth", config.baseHealth);
    config.meleeRange = json.value("meleeRange", config.meleeRange);
    config.meleeCooldown = json.value("meleeCooldown", config.meleeCooldown);
    config.meleeOffset = json.value("meleeOffset", config.meleeOffset);
    config.baseDamage = json.value("baseDamage", config.baseDamage);
    config.rangedRange = json.value("rangedRange", config.rangedRange);
    config.rangedCooldown = json.value("rangedCooldown", config.rangedCooldown);
    config.rangedProjectileSpeed = json.value("rangedProjectileSpeed", config.rangedProjectileSpeed);
    config.potionHealAmount = json.value("potionHealAmount", config.potionHealAmount);
    config.potionMaxCharges = json.value("potionMaxCharges", config.potionMaxCharges);
    config.potionKillsPerCharge = json.value("potionKillsPerCharge", config.potionKillsPerCharge);

    return config;
}

EquipmentConfig ConfigLoader::loadEquipment(const std::string& path) const
{
    EquipmentConfig config;
    nlohmann::json json = loadJson(path);

    auto loadArmorArray = [&json](const char* key, std::array<ArmorPieceStats, TIER_COUNT>& out) {
        if (!json.contains(key)) return;
        const auto& arr = json.at(key);
        for (std::size_t i = 0; i < TIER_COUNT && i < arr.size(); ++i) {
            out[i] = armorPieceFromJson(arr.at(i));
        }
    };

    auto loadWeaponArray = [&json](const char* key, std::array<WeaponStats, TIER_COUNT>& out) {
        if (!json.contains(key)) return;
        const auto& arr = json.at(key);
        for (std::size_t i = 0; i < TIER_COUNT && i < arr.size(); ++i) {
            out[i] = weaponStatsFromJson(arr.at(i));
        }
    };

    loadArmorArray("helmet", config.helmet);
    loadArmorArray("chest", config.chest);
    loadArmorArray("leggings", config.leggings);
    loadWeaponArray("sword", config.sword);
    loadWeaponArray("bow", config.bow);

    return config;
}

EnemyConfig ConfigLoader::loadEnemies(const std::string& enemiesPath, const std::string& bossesPath) const
{
    EnemyConfig config;

    nlohmann::json enemiesJson = loadJson(enemiesPath);
    if (enemiesJson.contains("enemies")) {
        for (const auto& entry : enemiesJson.at("enemies")) {
            EnemyTemplate tmpl;
            tmpl.id = entry.value("id", tmpl.id);
            tmpl.biome = biomeFromString(entry.value("biome", std::string("forest")));
            tmpl.health = entry.value("health", tmpl.health);
            tmpl.damage = entry.value("damage", tmpl.damage);
            tmpl.armor = entry.value("armor", tmpl.armor);
            tmpl.speed = entry.value("speed", tmpl.speed);
            tmpl.behavior = behaviorFromString(entry.value("behavior", std::string("CHASE")));

            tmpl.isRanged = entry.value("isRanged", tmpl.isRanged);
            tmpl.range = entry.value("range", tmpl.range);
            tmpl.cooldown = entry.value("cooldown", tmpl.cooldown);
            tmpl.projectileSpeed = entry.value("projectileSpeed", tmpl.projectileSpeed);

            if (entry.contains("statusEffect")) {
                const auto& effect = entry.at("statusEffect");
                tmpl.appliesStatusEffect = true;
                tmpl.statusEffectType = statusEffectFromString(effect.value("type", std::string("POISON")));
                tmpl.statusEffectDps = effect.value("dps", 0.0f);
                tmpl.statusEffectDuration = effect.value("duration", 0.0f);
            }

            tmpl.revivesOnce = entry.value("revivesOnce", tmpl.revivesOnce);
            tmpl.phaseThrough = entry.value("phaseThrough", tmpl.phaseThrough);
            tmpl.explodesOnDeath = entry.value("explodesOnDeath", tmpl.explodesOnDeath);
            tmpl.keyFragmentDropChance = entry.value("keyFragmentDropChance", tmpl.keyFragmentDropChance);

            tmpl.color = colorFromJson(entry.value("color", nlohmann::json::array()), tmpl.color);
            tmpl.size = vec2FromJson(entry.value("size", nlohmann::json::array()), tmpl.size);

            config.enemies.push_back(tmpl);
        }
    }

    nlohmann::json bossesJson = loadJson(bossesPath);
    if (bossesJson.contains("bosses")) {
        for (const auto& entry : bossesJson.at("bosses")) {
            BossTemplate tmpl;
            tmpl.id = entry.value("id", tmpl.id);
            tmpl.theme = biomeFromString(entry.value("theme", std::string("forest")));
            tmpl.health = entry.value("health", tmpl.health);
            tmpl.damage = entry.value("damage", tmpl.damage);
            tmpl.armor = entry.value("armor", tmpl.armor);
            tmpl.speed = entry.value("speed", tmpl.speed);
            tmpl.behavior = behaviorFromString(entry.value("behavior", std::string("BOSS_DRUID")));
            tmpl.color = colorFromJson(entry.value("color", nlohmann::json::array()), tmpl.color);
            tmpl.size = vec2FromJson(entry.value("size", nlohmann::json::array()), tmpl.size);

            config.bosses.push_back(tmpl);
        }
    }

    return config;
}

} // namespace tc
