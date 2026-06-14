#include "ecs/systems/AISystem.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include "config/ConfigLoader.hpp"
#include "ecs/Components.hpp"
#include "ecs/EntityFactory.hpp"

namespace tc {

namespace {

constexpr float WORLD_WIDTH = 1280.0f;
constexpr float WORLD_HEIGHT = 720.0f;

constexpr float DRUID_PHASE_DURATION = 3.0f;
constexpr float NAGA_PHASE_DURATION = 4.0f;
constexpr float ELEMENTAL_SLOW_INTERVAL = 4.0f;
constexpr float ELEMENTAL_SLOW_DURATION = 2.0f;
constexpr float LICH_SUMMON_INTERVAL = 5.0f;

struct PendingSpawn {
    EnemyTemplate tmpl;
    sf::Vector2f position;
};

// Distance a RANGED_KEEP_DISTANCE-style entity tries to maintain from the player
float keepDistanceFor(const entt::registry& registry, entt::entity entity, float fallback)
{
    if (const auto* ranged = registry.try_get<RangedCombat>(entity)) {
        return ranged->range * 0.8f;
    }
    return fallback;
}

void applyKeepDistance(Velocity& velocity, float distance, float nx, float ny, float keepDistance)
{
    if (distance < keepDistance - 10.0f) {
        velocity.dx = -nx;
        velocity.dy = -ny;
    } else if (distance > keepDistance + 10.0f) {
        velocity.dx = nx;
        velocity.dy = ny;
    } else {
        velocity.dx = 0.0f;
        velocity.dy = 0.0f;
    }
}

} // namespace

void AISystem::update(entt::registry& registry, float dt)
{
    auto playerView = registry.view<PlayerTag, Position>();
    if (playerView.begin() == playerView.end()) return;

    const entt::entity player = *playerView.begin();
    const auto playerPos = playerView.get<Position>(player);

    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> offsetDist(-80.0f, 80.0f);
    std::uniform_int_distribution<int> summonCountDist(2, 3);

    std::vector<PendingSpawn> pendingSpawns;

    auto view = registry.view<EnemyTag, AIBehavior, Position, Velocity>();
    for (auto entity : view) {
        const auto& pos = view.get<Position>(entity);
        auto& velocity = view.get<Velocity>(entity);
        const auto& behavior = view.get<AIBehavior>(entity);

        const float dx = playerPos.x - pos.x;
        const float dy = playerPos.y - pos.y;
        const float distance = std::sqrt(dx * dx + dy * dy);
        const float nx = distance > 0.0001f ? dx / distance : 0.0f;
        const float ny = distance > 0.0001f ? dy / distance : 0.0f;

        if (auto* bossAI = registry.try_get<BossAI>(entity)) {
            switch (behavior.type) {
            case AIBehavior::BOSS_DRUID: {
                bossAI->phaseTimer += dt;
                if (bossAI->phaseTimer >= DRUID_PHASE_DURATION) {
                    bossAI->phaseTimer = 0.0f;
                    bossAI->phaseIndex = (bossAI->phaseIndex + 1) % 3;

                    float speedMultiplier = 1.0f;
                    float meleeRange = 55.0f;
                    switch (bossAI->phaseIndex) {
                    case 0: speedMultiplier = 0.6f; meleeRange = 90.0f; break; // Bear: slow, wide swipe
                    case 1: speedMultiplier = 1.8f; meleeRange = 35.0f; break; // Hawk: fast, short reach
                    case 2: speedMultiplier = 1.1f; meleeRange = 55.0f; break; // Fox: balanced
                    default: break;
                    }

                    if (auto* speed = registry.try_get<Speed>(entity)) {
                        speed->value = bossAI->baseSpeed * speedMultiplier;
                    }
                    if (auto* melee = registry.try_get<MeleeCombat>(entity)) {
                        melee->range = meleeRange;
                    }
                }
                break;
            }
            case AIBehavior::BOSS_NAGA: {
                bossAI->phaseTimer += dt;
                if (bossAI->phaseTimer >= NAGA_PHASE_DURATION) {
                    bossAI->phaseTimer = 0.0f;
                    bossAI->rangedMode = !bossAI->rangedMode;

                    if (bossAI->rangedMode) {
                        registry.remove<MeleeCombat>(entity);
                        registry.emplace_or_replace<RangedCombat>(entity, 260.0f, 320.0f, 1.2f, 0.0f);
                    } else {
                        registry.remove<RangedCombat>(entity);
                        const auto& renderable = registry.get<Renderable>(entity);
                        registry.emplace_or_replace<MeleeCombat>(entity, std::max(renderable.size.x, renderable.size.y), 1.0f, 0.0f);
                    }
                }
                break;
            }
            case AIBehavior::BOSS_ELEMENTAL: {
                bossAI->phaseTimer += dt;
                if (bossAI->phaseTimer >= ELEMENTAL_SLOW_INTERVAL) {
                    bossAI->phaseTimer = 0.0f;
                    registry.emplace_or_replace<StatusEffect>(player, StatusEffect::SLOW, 0.0f, ELEMENTAL_SLOW_DURATION, ELEMENTAL_SLOW_DURATION);
                }
                break;
            }
            case AIBehavior::BOSS_LICH: {
                bossAI->phaseTimer += dt;
                if (bossAI->phaseTimer >= LICH_SUMMON_INTERVAL) {
                    bossAI->phaseTimer = 0.0f;

                    const auto templates = ConfigLoader::get().getEnemyConfig().getEnemiesForBiome(BiomeType::DEADLANDS);
                    if (!templates.empty()) {
                        const int count = summonCountDist(rng);
                        std::uniform_int_distribution<std::size_t> templateDist(0, templates.size() - 1);
                        for (int i = 0; i < count; ++i) {
                            const sf::Vector2f spawnPos{
                                std::clamp(pos.x + offsetDist(rng), 40.0f, WORLD_WIDTH - 40.0f),
                                std::clamp(pos.y + offsetDist(rng), 40.0f, WORLD_HEIGHT - 40.0f)};
                            pendingSpawns.push_back({templates[templateDist(rng)], spawnPos});
                        }
                    }
                }
                break;
            }
            default:
                break;
            }
        }

        if (distance < 0.0001f) {
            velocity.dx = 0.0f;
            velocity.dy = 0.0f;
            continue;
        }

        switch (behavior.type) {
        case AIBehavior::RANGED_KEEP_DISTANCE:
        case AIBehavior::BOSS_ELEMENTAL:
            applyKeepDistance(velocity, distance, nx, ny, keepDistanceFor(registry, entity, 220.0f));
            break;
        case AIBehavior::SWARM: {
            float vx = nx - ny * 0.5f;
            float vy = ny + nx * 0.5f;
            const float length = std::sqrt(vx * vx + vy * vy);
            if (length > 0.0001f) {
                vx /= length;
                vy /= length;
            }
            velocity.dx = vx;
            velocity.dy = vy;
            break;
        }
        case AIBehavior::BOSS_NAGA: {
            const auto* bossAI = registry.try_get<BossAI>(entity);
            if (bossAI && bossAI->rangedMode) {
                applyKeepDistance(velocity, distance, nx, ny, keepDistanceFor(registry, entity, 220.0f));
            } else {
                velocity.dx = nx;
                velocity.dy = ny;
            }
            break;
        }
        case AIBehavior::BOSS_LICH:
            velocity.dx = 0.0f;
            velocity.dy = 0.0f;
            break;
        case AIBehavior::CHASE:
        case AIBehavior::BOSS_DRUID:
        default:
            velocity.dx = nx;
            velocity.dy = ny;
            break;
        }
    }

    for (const auto& spawn : pendingSpawns) {
        EntityFactory::createEnemy(registry, spawn.tmpl, spawn.position);
    }
}

} // namespace tc
