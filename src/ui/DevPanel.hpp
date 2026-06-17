#pragma once

#include <SFML/Graphics.hpp>

#include "config/EnemyConfig.hpp"
#include "core/FontManager.hpp"
#include "core/Localization.hpp"

namespace tc {

// Developer panel overlay (compiled only when TC_DEV is defined).
// Open/close with F1. Tabs: Spawn | Player | HUD.
class DevPanel {
public:
    struct PlayerStats {
        int   health    = 0;
        int   maxHealth = 0;
        int   damage    = 0;
        int   armor     = 0;
        float speed     = 0.0f;
    };

    // Returns true if the click was consumed by this panel.
    bool handleClick(sf::Vector2f pos);
    void handleScroll(float delta);

    void render(sf::RenderWindow& window, const FontManager& fontManager,
                const Localization& localization, const EnemyConfig& enemyConfig,
                const PlayerStats& stats);

    // Spawn request — PlayState reads, then sets spawnPending = false.
    bool spawnPending = false;
    bool spawnIsBoss  = false;
    int  spawnIndex   = -1;

    bool playerInvincible = false;

    // Stat deltas — PlayState applies and resets to 0 each frame.
    int   healthDelta    = 0;
    int   maxHealthDelta = 0;
    int   damageDelta    = 0;
    int   armorDelta     = 0;
    float speedDelta     = 0.0f;

    // HUD visibility flags — PlayState syncs these to HUD each frame.
    bool showHudHealth      = true;
    bool showHudFragments   = true;
    bool showHudPotions     = true;
    bool showHudStatusIcons = true;

private:
    enum class Tab { SPAWN, PLAYER, HUD };
    Tab activeTab    = Tab::SPAWN;
    int scrollOffset = 0;

    // Cached from the last render() call, used in handleClick/handleScroll.
    int cachedEnemyCount = 0;
    int cachedBossCount  = 0;

    void applyStatDelta(int statIdx, int dir);
};

} // namespace tc
