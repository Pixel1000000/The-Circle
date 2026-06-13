#pragma once

#include <array>

#include "world/Biome.hpp"
#include "world/BiomeType.hpp"

namespace tc {

// Owns the four biomes laid out around the circle and the (randomly themed)
// boss room at the center. Entity placement is done by EntityFactory/PlayState,
// World only tracks progression state.
class World {
public:
    World();

    // Resets all biomes and rolls a new boss room theme. Call at the start of a run.
    void generate();

    Biome& getBiome(BiomeType type);
    const Biome& getBiome(BiomeType type) const;

    Biome& getCurrentBiome();
    const Biome& getCurrentBiome() const;
    void setCurrentBiome(BiomeType type);

    BiomeType getBossRoomTheme() const;

    // The boss room is reachable only once Biome 4 (Deadlands) is unlocked
    bool isBossRoomUnlocked() const;

private:
    std::array<Biome, BIOME_COUNT> biomes;
    BiomeType currentBiome = BiomeType::FOREST;
    BiomeType bossRoomTheme = BiomeType::FOREST;
};

} // namespace tc
