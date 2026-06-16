#pragma once

#include <string>
#include <vector>

#include <SFML/Graphics.hpp>
#include <entt/entt.hpp>

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

// Flat damage reduction applied before incoming damage, plus a percentage
// reduction applied after (meta-progression Endurance bonus)
struct Armor {
    int value = 0;
    float damageReductionPercent = 0.0f;
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

// Display name shown on the nameplate above an enemy's HP bar. `id` is the
// enemy/boss template id and is looked up as localization key "enemy.<id>".
struct Name {
    std::string id;
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
    float damageAccumulator = 0.0f;
};

// Key fragment drop chance, attached to enemies
struct KeyFragmentDrop {
    float chance = 0.0f;
};

// Equipment drop chance, attached to enemies
struct EquipmentDrop {
    float chance = 0.0f;
};

// Marks an entity as a boss
struct BossTag {};

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

// Marks an entity that takes no damage from any source (boss shield phases)
struct Invulnerable {};

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

// Boss-specific AI state: phase timing/cycling and the boss's base movement
// speed (before per-phase multipliers are applied)
struct BossAI {
    float phaseTimer = 0.0f;
    int phaseIndex = 0;
    float baseSpeed = 0.0f;
    bool rangedMode = false;

    // BOSS_LICH: index of the next HP-percentage threshold (75/50/25) that
    // will trigger a summon wave + shield, and the minions spawned by the
    // most recent wave (shield drops once all of them are dead).
    int nextSummonThreshold = 0;
    std::vector<entt::entity> summonedMinions;
};

// Elemental bonus types for weapons and armor
enum class Element { NONE, NATURE, FIRE, ICE, DECAY };

// Describes an elemental bonus: which element and how strong its effect is
struct ElementalEffect {
    Element element = Element::NONE;
    float percent = 0.0f;
};

// Player equipment - tiers correspond to biome difficulty (1-4, 0 = none)
struct Equipment {
    int helmetTier = 0;
    int chestTier = 0;
    int leggingsTier = 0;
    int weaponTier = 0;

    enum WeaponType { NONE, SWORD, BOW } weaponType = NONE;

    Element weaponElement = Element::NONE;
    float weaponElementPercent = 0.0f;

    Element helmetElement = Element::NONE;
    float helmetElementPercent = 0.0f;

    Element chestElement = Element::NONE;
    float chestElementPercent = 0.0f;

    Element leggingsElement = Element::NONE;
    float leggingsElementPercent = 0.0f;

    bool autoUpgradeWeapon = false;
    bool autoUpgradeHelmet = false;
    bool autoUpgradeChest = false;
    bool autoUpgradeLeggings = false;
};

// Stacking nature DoT applied by a NATURE-element weapon. Each hit multiplies
// dpsMultiplier and refreshes the duration.
struct NatureStack {
    float dpsMultiplier = 1.0f;
    float duration = 0.0f;
    float timer = 0.0f;
    int stacks = 0;
    float damageAccumulator = 0.0f;
};

// Burning DoT applied by a FIRE-element weapon. Re-hitting refreshes the
// timer instead of stacking.
struct FireBurn {
    float dps = 0.0f;
    float duration = 0.0f;
    float timer = 0.0f;
    float damageAccumulator = 0.0f;
};

// Marks a target chilled by an ICE-element hit. Tracks the SLOW duration that
// was applied alongside the bonus damage so it can be cleared together with
// the SLOW status effect.
struct IceChill {
    float slowDuration = 0.0f;
};

// Temporary max-HP reduction applied by a DECAY-element weapon. Health.max is
// restored by maxHpReduction when the effect expires.
struct DecayEffect {
    int maxHpReduction = 0;
    float duration = 0.0f;
    float timer = 0.0f;
};

// Elemental resistances granted by armor (helmet/chest element bonuses) and
// leggings ICE immunity. slowResist == 1.0 means full immunity to SLOW.
struct ElementalResist {
    float fireResist = 0.0f;
    float natureResist = 0.0f;
    float slowResist = 0.0f;
    float decayResist = 0.0f;
};

// Fractional HP accumulator for the leggings NATURE lifesteal effect, since
// Speed * percent * dt is usually well under 1 HP per frame.
struct Lifesteal {
    float accumulator = 0.0f;
};

// Static impassable block placed in a biome for tactical cover.
struct Obstacle {};

// Tag for a blizzard slow-zone spawned by BOSS_ELEMENTAL.
struct BlizzardZoneTag {};

// Drift behaviour for a blizzard slow-zone (size lives in Renderable).
struct BlizzardZone {
    bool drifting = false;
    sf::Vector2f driftDir = {0.f, 0.f};
    float dirTimer = 0.f;
};

// Meta-progression, persists across runs via meta_save.json
struct MetaStats {
    int strength = 0;
    int endurance = 0;
    int health = 0;
    int points = 0;
};

} // namespace tc
