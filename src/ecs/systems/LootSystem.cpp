#include "ecs/systems/LootSystem.hpp"

#include <algorithm>
#include <random>
#include <vector>

#include "config/ConfigLoader.hpp"
#include "ecs/Components.hpp"

namespace tc {

namespace {

// Element rolled for an equipment drop, based on the biome the enemy died in
// (EnemyTag::biome, 1-4 == Forest/Desert/Winter/Deadlands).
Element elementForBiomeTier(int tier)
{
    switch (tier) {
    case 1: return Element::NATURE;
    case 2: return Element::FIRE;
    case 3: return Element::ICE;
    case 4: return Element::DECAY;
    default: return Element::NONE;
    }
}

} // namespace

LootResult LootSystem::update(entt::registry& registry, entt::entity player, const std::vector<entt::entity>& waveEnemies,
    int fragmentsAlreadyCollected, int fragmentsRequired)
{
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> roll(0.0f, 1.0f);

    LootResult result;
    std::vector<entt::entity> toDestroy;

    auto view = registry.view<EnemyTag, Health>();
    for (auto entity : view) {
        auto& health = view.get<Health>(entity);
        if (health.current > 0) continue;

        if (auto* revive = registry.try_get<ReviveOnce>(entity)) {
            if (!revive->used) {
                revive->used = true;
                health.current = health.max / 2;
                continue;
            }
        }

        toDestroy.push_back(entity);
    }

    // If killing everything in `toDestroy` empties the current wave, the last
    // one processed below is guaranteed a key fragment and an equipment drop.
    bool waveCleared = false;
    if (!toDestroy.empty() && !waveEnemies.empty()) {
        const std::size_t remaining = std::count_if(waveEnemies.begin(), waveEnemies.end(),
            [&](entt::entity e) {
                return registry.valid(e) && std::find(toDestroy.begin(), toDestroy.end(), e) == toDestroy.end();
            });
        waveCleared = remaining == 0;
    }

    for (std::size_t i = 0; i < toDestroy.size(); ++i) {
        const entt::entity entity = toDestroy[i];
        const bool guaranteedDrop = waveCleared && i == toDestroy.size() - 1;

        if (const auto* drop = registry.try_get<KeyFragmentDrop>(entity)) {
            const bool fragmentsStillNeeded = fragmentsAlreadyCollected + result.fragmentsCollected < fragmentsRequired;
            if (fragmentsStillNeeded && (guaranteedDrop || roll(rng) <= drop->chance)) {
                ++registry.get<KeyFragmentHolder>(player).count;
                ++result.fragmentsCollected;
            }
        }

        if (const auto* drop = registry.try_get<EquipmentDrop>(entity)) {
            if (guaranteedDrop || roll(rng) <= drop->chance) {
                const int tier = registry.get<EnemyTag>(entity).biome;
                const int tierIndex = tier - 1;

                std::vector<int> availableSlots;
                for (int candidate = 0; candidate < 4; ++candidate) {
                    if (tierIndex < 0 || tierIndex >= TIER_COUNT || !droppedSlots[tierIndex][candidate]) {
                        availableSlots.push_back(candidate);
                    }
                }
                // All four slots already dropped at this tier - fall back to
                // any slot rather than withholding the drop entirely.
                if (availableSlots.empty()) {
                    for (int candidate = 0; candidate < 4; ++candidate) {
                        availableSlots.push_back(candidate);
                    }
                }

                std::uniform_int_distribution<std::size_t> slotDist(0, availableSlots.size() - 1);
                const int slot = availableSlots[slotDist(rng)];

                if (tierIndex >= 0 && tierIndex < TIER_COUNT) {
                    droppedSlots[tierIndex][slot] = true;
                }

                Element element = Element::NONE;
                float percent = 0.0f;
                if (tierIndex >= 0 && tierIndex < TIER_COUNT) {
                    element = elementForBiomeTier(tier);
                    const auto& range = ConfigLoader::get().getElementalConfig().dropPercentRanges[tierIndex];
                    std::uniform_real_distribution<float> percentDist(range.minPercent, range.maxPercent);
                    percent = percentDist(rng);
                }

                result.equipmentDropped = true;
                result.droppedSlot = slot;
                result.droppedTier = tier;
                result.droppedElement = element;
                result.droppedPercent = percent;
            }
        }

        ++result.kills;
    }

    for (auto entity : toDestroy) {
        registry.destroy(entity);
    }

    return result;
}

} // namespace tc
