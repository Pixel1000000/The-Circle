#pragma once

#include <string>

#include "config/ElementalConfig.hpp"
#include "config/EnemyConfig.hpp"
#include "config/EquipmentConfig.hpp"
#include "config/PlayerConfig.hpp"

namespace tc {

// Loads all gameplay JSON configs from assets/config/ and exposes them as a
// global singleton. Re-running loadAll() (e.g. after editing the JSON files)
// refreshes the values without recompiling.
class ConfigLoader {
public:
    static ConfigLoader& get();

    void loadAll(const std::string& configDir = "assets/config");

    const PlayerConfig& getPlayerConfig() const;
    const EquipmentConfig& getEquipmentConfig() const;
    const EnemyConfig& getEnemyConfig() const;
    const ElementalConfig& getElementalConfig() const;

private:
    ConfigLoader() = default;

    PlayerConfig loadPlayer(const std::string& path) const;
    EquipmentConfig loadEquipment(const std::string& path) const;
    EnemyConfig loadEnemies(const std::string& enemiesPath, const std::string& bossesPath) const;
    ElementalConfig loadElemental(const std::string& path) const;

    PlayerConfig playerConfig;
    EquipmentConfig equipmentConfig;
    EnemyConfig enemyConfig;
    ElementalConfig elementalConfig;
};

} // namespace tc
