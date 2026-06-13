#pragma once

namespace tc {

// Base player stats before meta-progression and equipment bonuses are applied.
// Loaded from assets/config/player.json
struct PlayerConfig {
    float baseSpeed = 200.0f;
    int baseHealth = 100;

    float meleeRange = 40.0f;
    float meleeCooldown = 0.6f;
    float meleeOffset = 20.0f;
    int baseDamage = 8;

    float rangedRange = 300.0f;
    float rangedCooldown = 1.0f;
    float rangedProjectileSpeed = 400.0f;

    int potionHealAmount = 50;
    int potionMaxCharges = 3;
    int potionKillsPerCharge = 5;
};

} // namespace tc
