#pragma once

#include <array>

#include <SFML/System/Vector2.hpp>

#include "config/EquipmentConfig.hpp"

namespace tc {

// Min/max % roll for an equipment drop's elemental effect, indexed by tier-1
struct ElementalDropRange {
    float minPercent = 0.0f;
    float maxPercent = 0.0f;
};

// BOSS_ELEMENTAL blizzard zone parameters
struct BlizzardConfig {
    float interval = 8.0f;
    sf::Vector2f bigSize = {150.0f, 150.0f};
    sf::Vector2f smallSize = {60.0f, 60.0f};
    int smallCountMin = 3;
    int smallCountMax = 6;
    float driftSpeed = 20.0f;
    float driftChangeMin = 3.0f;
    float driftChangeMax = 5.0f;
    float slowDuration = 0.2f;
};

// Loaded from assets/config/elements.json
struct ElementalConfig {
    std::array<ElementalDropRange, TIER_COUNT> dropPercentRanges;

    float natureBaseDps = 5.0f;
    float natureDuration = 4.0f;

    float fireBaseDps = 6.0f;
    float fireDuration = 3.0f;

    float iceSlowDuration = 2.0f;

    float decayDuration = 5.0f;

    BlizzardConfig blizzard;
};

} // namespace tc
