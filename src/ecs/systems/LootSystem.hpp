#pragma once

#include <array>
#include <vector>

#include <entt/entt.hpp>

#include "config/EquipmentConfig.hpp"

namespace tc {

// Outcome of one LootSystem update, used by PlayState to track run progress.
struct LootResult {
    int kills = 0;
    int fragmentsCollected = 0;

    // Equipment slot indices: 0 = helmet, 1 = chest, 2 = leggings, 3 = weapon
    bool equipmentDropped = false;
    int droppedSlot = 0;
    int droppedTier = 0;
};

// Removes dead enemies, handling ReviveOnce and rolling KeyFragmentDrop onto
// the player's KeyFragmentHolder. `waveEnemies` is the current biome's enemy
// list; if killing the dying entities empties it, the last one to die is
// guaranteed a key fragment and an equipment drop.
class LootSystem {
public:
    LootResult update(entt::registry& registry, entt::entity player, const std::vector<entt::entity>& waveEnemies);

private:
    // Tracks which equipment slots have already dropped per tier, so a
    // successful EquipmentDrop roll picks a slot/tier combo the player
    // hasn't received yet (slot 0 = helmet, 1 = chest, 2 = leggings, 3 = weapon).
    std::array<std::array<bool, 4>, TIER_COUNT> droppedSlots{};
};

} // namespace tc
