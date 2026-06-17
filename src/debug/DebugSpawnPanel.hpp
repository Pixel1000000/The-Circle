#pragma once

#ifdef TC_DEBUG

#include "debug/DebugContext.hpp"
#include "debug/DebugWidgets.hpp"

namespace tc {

// Spawn tab: create enemies/bosses from the loaded config templates, or
// wipe every enemy/boss currently in the world.
class DebugSpawnPanel {
public:
    void init(const sf::Font& font, sf::Vector2f origin);
    void onOpen(DebugContext& ctx);

    void handleMousePressed(sf::Vector2f point, DebugContext& ctx);
    void handleMouseMoved(sf::Vector2f point);
    void handleMouseReleased();
    bool handleDropdownClick(sf::Vector2f point);
    bool handleScroll(sf::Vector2f point, float delta);

    void render(sf::RenderWindow& window) const;
    // Draws expanded dropdown lists. Call after render() so an open list is
    // always on top, even over widgets/dropdowns positioned below it.
    void renderDropdownOverlays(sf::RenderWindow& window) const;

private:
    DebugDropdown enemyDropdown;
    DebugButton spawnEnemyButton;
    DebugSlider countSlider;
    DebugButton clearEnemiesButton;

    DebugDropdown bossDropdown;
    DebugButton spawnBossButton;
};

} // namespace tc

#endif // TC_DEBUG
