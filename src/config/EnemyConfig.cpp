#include "config/EnemyConfig.hpp"

#include <algorithm>

namespace tc {

std::vector<EnemyTemplate> EnemyConfig::getEnemiesForBiome(BiomeType biome) const
{
    std::vector<EnemyTemplate> result;
    std::copy_if(enemies.begin(), enemies.end(), std::back_inserter(result),
        [biome](const EnemyTemplate& tmpl) { return tmpl.biome == biome; });
    return result;
}

const BossTemplate* EnemyConfig::getBossForTheme(BiomeType theme) const
{
    auto it = std::find_if(bosses.begin(), bosses.end(),
        [theme](const BossTemplate& tmpl) { return tmpl.theme == theme; });
    return it != bosses.end() ? &(*it) : nullptr;
}

} // namespace tc
