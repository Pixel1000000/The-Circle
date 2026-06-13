#include "meta/MetaProgression.hpp"

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

namespace tc {

void MetaProgression::load(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[MetaProgression] No save found at " << path << ", starting fresh\n";
        return;
    }

    try {
        nlohmann::json json;
        file >> json;
        stats.strength = json.value("strength", stats.strength);
        stats.endurance = json.value("endurance", stats.endurance);
        stats.health = json.value("health", stats.health);
        stats.points = json.value("points", stats.points);
    } catch (const std::exception& e) {
        std::cerr << "[MetaProgression] Failed to parse " << path << ": " << e.what() << "\n";
    }
}

void MetaProgression::save(const std::string& path) const
{
    nlohmann::json json;
    json["strength"] = stats.strength;
    json["endurance"] = stats.endurance;
    json["health"] = stats.health;
    json["points"] = stats.points;

    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "[MetaProgression] Could not write " << path << "\n";
        return;
    }
    file << json.dump(2);
}

const MetaStats& MetaProgression::getStats() const
{
    return stats;
}

int MetaProgression::computePointsEarned(const RunSummary& summary) const
{
    int points = (summary.kills / 10) * POINTS_PER_10_KILLS;
    points += summary.biomesCleared * POINTS_PER_BIOME;
    if (summary.bossDefeated) {
        points += POINTS_FOR_BOSS;
    }
    return points;
}

void MetaProgression::addPoints(int amount)
{
    stats.points += amount;
}

bool MetaProgression::spendPointOnStrength()
{
    if (stats.points <= 0) return false;
    --stats.points;
    ++stats.strength;
    return true;
}

bool MetaProgression::spendPointOnEndurance()
{
    if (stats.points <= 0) return false;
    --stats.points;
    ++stats.endurance;
    return true;
}

bool MetaProgression::spendPointOnHealth()
{
    if (stats.points <= 0) return false;
    --stats.points;
    ++stats.health;
    return true;
}

} // namespace tc
