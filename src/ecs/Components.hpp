#pragma once

#include <SFML/Graphics.hpp>

namespace tc {

// Position in world space
struct Position {
    float x = 0.0f;
    float y = 0.0f;
};

// Normalized movement direction. Actual displacement = direction * Speed * dt
struct Velocity {
    float dx = 0.0f;
    float dy = 0.0f;
};

// Normalized direction the entity is currently facing, used to aim melee
// attacks in front of the entity. Retains the last non-zero movement
// direction while idle.
struct Facing {
    float dx = 0.0f;
    float dy = 1.0f;
};

// Health
struct Health {
    int current = 1;
    int max = 1;
};

// Flat damage reduction applied before incoming damage
struct Armor {
    int value = 0;
};

// Damage dealt by an attack or projectile
struct Damage {
    int value = 0;
};

// Movement speed in units/second
struct Speed {
    float value = 0.0f;
};

// Tag: controlled by the player
struct PlayerTag {};

// Tag: enemy, tied to the biome it spawned in
struct EnemyTag {
    int biome = 1;
};

// Tag: projectile
struct ProjectileTag {
    int ownerBiome = 0;
};

// Render - placeholder rectangle until sprites are added
struct Renderable {
    sf::Color color = sf::Color::White;
    sf::Vector2f size = {32.0f, 32.0f};
};

// Melee combat: attacks the nearest hostile entity within range of a point
// `offset` units in front of the attacker, along its Facing direction.
// Enemies use offset = 0, attacking in a radius centered on themselves.
struct MeleeCombat {
    float range = 40.0f;
    float cooldown = 1.0f;
    float timer = 0.0f;
    float offset = 0.0f;
};

// Ranged combat: spawns a projectile aimed at the nearest hostile entity
struct RangedCombat {
    float range = 250.0f;
    float projectileSpeed = 300.0f;
    float cooldown = 1.0f;
    float timer = 0.0f;
};

// Block ability - sword + shield only
struct BlockAbility {
    float reductionPercent = 0.0f;
    bool isBlocking = false;
};

// Potion - Path of Exile style charge-on-kill healing
struct Potion {
    int healAmount = 0;
    int charges = 0;
    int maxCharges = 0;
    int killsPerCharge = 1;
    int killCounter = 0;
};

// Brief red flash applied to an entity's Renderable when it takes damage
struct HitFlash {
    static constexpr float DURATION = 0.15f;
    float timer = DURATION;
};

// Status effect applied to an entity (poison DoT or slow)
struct StatusEffect {
    enum Type { POISON, SLOW } type = POISON;
    float dps = 0.0f;
    float duration = 0.0f;
    float timer = 0.0f;
};

// Key fragment drop chance, attached to enemies
struct KeyFragmentDrop {
    float chance = 0.0f;
};

// Key fragment counter, attached to the player
struct KeyFragmentHolder {
    int count = 0;
};

// Marks an enemy that revives once after its first death (skeletons)
struct ReviveOnce {
    bool used = false;
};

// Marks an entity that ignores wall collisions (ghosts)
struct PhaseThrough {};

// AI behavior driving the AISystem
struct AIBehavior {
    enum Type {
        CHASE,
        RANGED_KEEP_DISTANCE,
        SWARM,
        BOSS_DRUID,
        BOSS_NAGA,
        BOSS_ELEMENTAL,
        BOSS_LICH
    } type = CHASE;
};

// Player equipment - tiers correspond to biome difficulty (1-4, 0 = none)
struct Equipment {
    int helmetTier = 0;
    int chestTier = 0;
    int leggingsTier = 0;
    int weaponTier = 0;

    enum WeaponType { NONE, SWORD, BOW } weaponType = NONE;
};

// Meta-progression, persists across runs via meta_save.json
struct MetaStats {
    int strength = 0;
    int endurance = 0;
    int health = 0;
    int points = 0;
};

} // namespace tc
