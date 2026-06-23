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
    // When true, the projectile can hit either the player or any enemy
    // (whichever it touches first), ignoring ownerBiome faction rules.
    // Used by the ice goblin's death-explosion shards.
    bool hitsAnyUnit = false;
};

// Applied to a projectile entity; on hit, this SLOW duration is applied to
// the struck target in addition to its damage (ice goblin death shards).
struct SlowOnHit {
    float duration = 2.0f;
};

// Debug-only: overrides the attack damage used in combat without touching
// the stored Damage.value, so equipment recalculations (which overwrite
// Damage.value from gear stats) can't silently clobber the One Shot toggle.
struct DamageOverride {
    int value = 0;
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
    // Chance (0–1) to actually apply this effect on each hit; only meaningful
    // on the source enemy entity, not on the active copy placed on the target.
    float applicationChance = 1.0f;
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
    // Chance (0–1) to trigger the elemental on-hit effect; always 1 for the
    // player's weapon, may be <1 for enemy elemental attacks.
    float applicationChance = 1.0f;
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

// Wolf: dashes in a straight line at burst speed, briefly invulnerable.
// The dash direction is locked in at activation (the player's last known
// position), not homed onto the player while dashing. `timer` is seeded
// with a random per-wolf offset so a pack doesn't dash in lockstep.
struct DashAbility {
    float cooldown = 4.0f;
    float timer = 0.0f;
    float duration = 0.3f;
    float durationTimer = 0.0f;
    bool dashing = false;
    float dirX = 0.0f;
    float dirY = 0.0f;
};

// Scorpion: when HP first drops below hpThreshold (fraction of max), rolls
// burrowChance once to decide whether it burrows at all (one opportunity
// per life either way). On a successful roll it teleports a medium
// distance in a random direction, leaves a small quicksand zone at its old
// spot, and regenerates HP while burrowed/invulnerable. It force-surfaces
// and resumes chasing after maxDuration seconds, or surfaces immediately
// and dashes once in a straight line to the player's last known position
// if the player gets within dashRange while it's still burrowed.
struct BurrowAbility {
    float hpThreshold = 0.3f;
    float burrowChance = 0.5f;
    float teleportMinOffset = 280.0f;
    float teleportMaxOffset = 360.0f;
    float maxDuration = 5.0f;
    float regenPercentPerSecond = 0.05f;
    float dashSpeed = 480.0f;
    float dashDuration = 0.3f;
    float dashRange = dashSpeed * dashDuration;
    float timer = 0.0f;
    float regenAccumulator = 0.0f;
    bool burrowed = false;
    bool usedOnce = false;
    bool dashing = false;
    float dashDirX = 0.0f;
    float dashDirY = 0.0f;
};

// Ant: spawns `count` traps in a ring around its death position.
struct TrapSpawner {
    int count = 3;
    float radius = 60.0f;
    float stunDuration = 1.0f;
};

// Wasp swarm: scatters away from the player briefly after taking damage.
struct SwarmScatter {
    float duration = 0.6f;
    float timer = 0.0f;
    bool scattered = false;
};

// Naga rider: periodically charges in a straight line at the player.
struct ChargeAbility {
    float cooldown = 3.0f;
    float timer = 0.0f;
    float speed = 400.0f;
    bool charging = false;
    sf::Vector2f chargeDir = {0.0f, 0.0f};
    float stunOnFail = 0.5f;
};

// Ice goblin: before dying, stands still and turns white for `delay` seconds.
// If undisturbed, the corpse explodes into `shardCount` projectile shards
// fired in all directions, each dealing shardDamage and applying a slow.
// Any further damage taken while telegraphing cancels the explosion (the
// goblin just dies normally).
struct IceGoblinExplosion {
    float delay = 2.0f;
    float timer = 0.0f;
    bool telegraphing = false;
    bool exploded = false;
    bool cancelled = false;
    int shardCount = 8;
    int shardDamage = 10;
    float shardSpeed = 260.0f;
    float slowDuration = 2.0f;
};

// Ice spirit: periodically teleports near the player.
struct TeleportAbility {
    float cooldown = 5.0f;
    float timer = 0.0f;
};

// Snow witch: periodically spawns a decoy clone of herself (1 HP). Destroying
// the clone slows the player; if the witch dies first, remaining clones just
// vanish. Capped at maxClones concurrently alive; while at the cap, the
// witch's ability is locked.
struct WitchCloneAbility {
    float cooldown = 5.0f;
    float timer = 0.0f;
    int maxClones = 2;
    float cloneSlowDuration = 2.5f;
    std::vector<entt::entity> activeClones;
};

// Tag on a snow witch decoy clone entity. On the clone's death, the player
// is slowed for slowDuration (handled in AbilitySystem::handleDeathEffects).
struct WitchCloneTag {
    float slowDuration = 2.5f;
};

// Tag: this entity's movement/melee targeting picks the nearest unit
// (player or enemy), not always the player (Yeti berserk aggro).
struct AggroNearestUnit {};

// Yeti: enrages (speed + damage multiplier) once HP drops below hpThreshold.
struct RageAbility {
    float hpThreshold = 0.4f;
    float speedMult = 1.5f;
    float damageMult = 1.5f;
    bool enraged = false;
};

// Ghost: chance to absorb incoming damage instead of taking it, releasing the
// accumulated damage back as a burst every releaseInterval seconds.
struct AbsorbChance {
    float chance = 0.3f;
    float accumulatedDamage = 0.0f;
    float releaseTimer = 0.0f;
    float releaseInterval = 2.0f;
};

// Bone golem: periodically detaches a skeleton minion once HP drops below
// hpThreshold, up to maxDetach times.
struct BoneDetach {
    float hpThreshold = 0.6f;
    float interval = 3.0f;
    float timer = 0.0f;
    int detached = 0;
    int maxDetach = 2;
};

// Skeleton warrior: after reviving (ReviveOnce), gains temporary invulnerability
// and a damage bonus.
struct SkeletonReviveBonus {
    float invulDuration = 1.5f;
    float invulTimer = 0.0f;
    float damageMult = 1.5f;
    bool bonusActive = false;
};

// Sand spirit: periodically spawns a quicksand zone (global limit enforced by
// ZoneSystem/AbilitySystem, not this component).
struct QuicksandSpawner {
    float cooldown = 4.0f;
    float timer = 0.0f;
    sf::Vector2f size = {70.0f, 70.0f};
    float duration = 5.0f;
};

// Mummy: telegraphs death with a few blinks before exploding, stunning nearby
// targets within radius.
struct MummyDeathBomb {
    float delay = 1.0f;
    float timer = 0.0f;
    bool triggered = false;
    float radius = 90.0f;
    float stunDuration = 1.0f;
    int blinkCount = 4;
    int currentBlink = 0;
    float blinkInterval = 0.2f;
    float blinkTimer = 0.0f;
};

// Tag: a trap zone left behind by an Ant's TrapSpawner.
struct TrapTag {};

// Tag: a quicksand zone spawned by a Sand spirit (global limit enforced
// elsewhere, e.g. 3 concurrent on the map).
struct QuicksandTag {};

// Tag: an ice zone left by a Snow witch's TeleportAbility.
struct IceZoneTag {};

// Shared expiry timer for effect zones (TrapTag/QuicksandTag/IceZoneTag).
struct ZoneDuration {
    float timer = 0.0f;
};

// Stun duration carried by a trap zone, captured from the Ant's TrapSpawner
// at spawn time (the Ant itself is destroyed once it dies).
struct TrapZone {
    float stunDuration = 1.0f;
};

// Prevents movement for `timer` seconds (traps, failed charges).
struct Stunned {
    float timer = 0.0f;
};

// Goblin archer: emergency summon of a wolf once HP drops below 30%.
struct EmergencySummon {
    float hpThreshold = 0.3f;
    bool triggered = false;
};

// Ice spirit: periodically fires an icy pulse that freezes (SLOWs) the player
// if they are within range. This is the ability path for applying freeze;
// regular melee hits do NOT carry an elemental on this enemy.
struct IcePulseAbility {
    float range = 130.0f;
    float cooldown = 5.0f;
    float timer = 0.0f;
    float slowDuration = 2.5f;
};

// Skeleton corpse-prop left behind on first death, with half the skeleton's
// max HP. If destroyed within `timer` seconds, the skeleton stays dead. If
// it survives, the skeleton revives (handled by AbilitySystem).
struct SkeletonBones {
    std::string templateId;
    float timer = 5.0f;
};

// Meta-progression, persists across runs via meta_save.json
struct MetaStats {
    int strength = 0;
    int endurance = 0;
    int health = 0;
    int points = 0;
};

} // namespace tc
