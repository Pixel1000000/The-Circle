#pragma once

#include <entt/entt.hpp>

namespace tc {

// Outcome of one LootSystem update, used by PlayState to track run progress.
struct LootResult {
    int kills = 0;
    int fragmentsCollected = 0;
};

// Removes dead enemies, handling ReviveOnce and rolling KeyFragmentDrop onto
// the player's KeyFragmentHolder.
class LootSystem {
public:
    LootResult update(entt::registry& registry, entt::entity player);
};

} // namespace tc
