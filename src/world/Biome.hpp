#pragma once

#include <vector>

#include <entt/entt.hpp>

#include "world/BiomeType.hpp"

namespace tc {

// One of the four world quadrants. Tracks its own enemies and key fragment progress.
class Biome {
public:
    static constexpr int KEY_FRAGMENTS_REQUIRED = 5;

    Biome() = default;
    Biome(BiomeType type, int tier);

    BiomeType getType() const;
    int getTier() const;

    void addKeyFragment();
    int getKeyFragmentsCollected() const;
    bool isUnlocked() const;

    void reset();

    std::vector<entt::entity>& getEnemies();
    const std::vector<entt::entity>& getEnemies() const;

private:
    BiomeType type = BiomeType::FOREST;
    int tier = 1;
    int keyFragmentsCollected = 0;
    std::vector<entt::entity> enemies;
};

} // namespace tc
