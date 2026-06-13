#include "world/World.hpp"

#include <random>

namespace tc {

World::World()
{
    for (int i = 0; i < BIOME_COUNT; ++i) {
        biomes[i] = Biome(static_cast<BiomeType>(i), i + 1);
    }
}

void World::generate()
{
    for (auto& biome : biomes) {
        biome.reset();
    }

    currentBiome = BiomeType::FOREST;

    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, BIOME_COUNT - 1);
    bossRoomTheme = static_cast<BiomeType>(dist(rng));
}

Biome& World::getBiome(BiomeType type)
{
    return biomes[static_cast<std::size_t>(type)];
}

const Biome& World::getBiome(BiomeType type) const
{
    return biomes[static_cast<std::size_t>(type)];
}

Biome& World::getCurrentBiome()
{
    return getBiome(currentBiome);
}

const Biome& World::getCurrentBiome() const
{
    return getBiome(currentBiome);
}

void World::setCurrentBiome(BiomeType type)
{
    currentBiome = type;
}

BiomeType World::getBossRoomTheme() const
{
    return bossRoomTheme;
}

bool World::isBossRoomUnlocked() const
{
    return getBiome(BiomeType::DEADLANDS).isUnlocked();
}

} // namespace tc
