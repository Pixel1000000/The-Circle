#include "ecs/systems/LootSystem.hpp"

#include <random>
#include <vector>

#include "ecs/Components.hpp"

namespace tc {

LootResult LootSystem::update(entt::registry& registry, entt::entity player)
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

        if (const auto* drop = registry.try_get<KeyFragmentDrop>(entity)) {
            if (roll(rng) <= drop->chance) {
                ++registry.get<KeyFragmentHolder>(player).count;
                ++result.fragmentsCollected;
            }
        }

        ++result.kills;
        toDestroy.push_back(entity);
    }

    for (auto entity : toDestroy) {
        registry.destroy(entity);
    }

    return result;
}

} // namespace tc
