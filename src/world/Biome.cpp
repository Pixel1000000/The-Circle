#include "world/Biome.hpp"

namespace tc {

Biome::Biome(BiomeType type, int tier)
    : type(type)
    , tier(tier)
{
}

BiomeType Biome::getType() const
{
    return type;
}

int Biome::getTier() const
{
    return tier;
}

void Biome::addKeyFragment()
{
    if (keyFragmentsCollected < KEY_FRAGMENTS_REQUIRED) {
        ++keyFragmentsCollected;
    }
}

int Biome::getKeyFragmentsCollected() const
{
    return keyFragmentsCollected;
}

bool Biome::isUnlocked() const
{
    return keyFragmentsCollected >= KEY_FRAGMENTS_REQUIRED;
}

void Biome::reset()
{
    keyFragmentsCollected = 0;
    enemies.clear();
}

std::vector<entt::entity>& Biome::getEnemies()
{
    return enemies;
}

const std::vector<entt::entity>& Biome::getEnemies() const
{
    return enemies;
}

} // namespace tc
