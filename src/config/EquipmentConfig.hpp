#pragma once

#include <array>

namespace tc {

constexpr int TIER_COUNT = 4;

// Per-tier stats for a single armor slot (helmet, chest or leggings)
struct ArmorPieceStats {
    int armor = 0;
    int healthBonus = 0;
    float speedBonus = 0.0f;
};

// Per-tier stats for a weapon (sword+shield or bow)
struct WeaponStats {
    int damage = 0;
    float attackSpeed = 1.0f;      // attacks per second, fixed per weapon type
    float blockReduction = 0.0f;   // sword+shield only
    float range = 0.0f;
};

// Loaded from assets/config/equipment.json. Index 0 = Tier 1, ... index 3 = Tier 4
struct EquipmentConfig {
    std::array<ArmorPieceStats, TIER_COUNT> helmet;
    std::array<ArmorPieceStats, TIER_COUNT> chest;
    std::array<ArmorPieceStats, TIER_COUNT> leggings;

    std::array<WeaponStats, TIER_COUNT> sword;
    std::array<WeaponStats, TIER_COUNT> bow;
};

} // namespace tc
