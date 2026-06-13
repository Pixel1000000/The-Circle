#pragma once

namespace tc {

enum class BiomeType {
    FOREST = 0,
    DESERT = 1,
    WINTER = 2,
    DEADLANDS = 3
};

constexpr int BIOME_COUNT = 4;

// Equipment tier matches the biome index (biome 1 -> tier 1, ...)
constexpr int biomeTier(BiomeType type) {
    return static_cast<int>(type) + 1;
}

} // namespace tc
