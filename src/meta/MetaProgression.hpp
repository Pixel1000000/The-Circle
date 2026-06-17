#pragma once

#include <string>

#include "ecs/Components.hpp"

namespace tc {

// Stats collected during a single run, used to compute meta points on death.
struct RunSummary {
    int kills = 0;
    int biomesCleared = 0;
    int keyFragments = 0;
    bool bossDefeated = false;
};

// Persists MetaStats (Strength / Endurance / Health) to meta_save.json between runs.
class MetaProgression {
public:
    static constexpr int KILLS_PER_POINT = 20;
    static constexpr int POINTS_PER_BIOME = 1;
    static constexpr int POINTS_FOR_BOSS = 5;

    // Each meta-progression point grants a percentage bonus rather than a
    // flat amount, so equipment upgrades remain meaningful at every tier.
    static constexpr float DAMAGE_PERCENT_PER_STRENGTH = 0.02f;
    static constexpr float DAMAGE_REDUCTION_PERCENT_PER_ENDURANCE = 0.02f;
    static constexpr float HEALTH_PERCENT_PER_LEVEL = 0.02f;

    void load(const std::string& path = "meta_save.json");
    void save(const std::string& path = "meta_save.json") const;

    const MetaStats& getStats() const;

    // Computes the points earned for a finished run, without applying them.
    int computePointsEarned(const RunSummary& summary) const;

    void addPoints(int amount);

    // Spends a point on one of the meta stats (1 point = 1 level). Returns
    // false if there are no points left to spend.
    bool spendPointOnStrength();
    bool spendPointOnEndurance();
    bool spendPointOnHealth();

private:
    MetaStats stats;
};

} // namespace tc
