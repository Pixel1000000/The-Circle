#pragma once

#ifdef TC_DEBUG

#include "debug/DebugContext.hpp"
#include "debug/DebugWidgets.hpp"
#include "ecs/Components.hpp"

namespace tc {

// World tab: key fragments, direct equipment grants, in-memory meta-stat
// overrides and forced biome/boss-room progression.
class DebugWorldPanel {
public:
    void init(const sf::Font& font, sf::Vector2f origin);
    void onOpen(DebugContext& ctx);

    void handleMousePressed(sf::Vector2f point, DebugContext& ctx);
    void handleMouseMoved(sf::Vector2f point);
    void handleMouseReleased();
    bool handleDropdownClick(sf::Vector2f point);

    void render(sf::RenderWindow& window) const;

private:
    DebugButton addFragmentButton;
    DebugButton maxFragmentsButton;

    DebugDropdown slotDropdown;
    DebugDropdown tierDropdown;
    DebugDropdown elementDropdown;
    DebugSlider effectPercentSlider;
    DebugButton giveEquipmentButton;

    DebugSlider strengthSlider;
    DebugSlider enduranceSlider;
    DebugSlider healthSlider;
    DebugButton applyMetaButton;

    DebugButton nextBiomeButton;
    DebugButton enterBossRoomButton;

    // In-memory only; never written to meta_save.json.
    MetaStats debugMetaStats;
};

} // namespace tc

#endif // TC_DEBUG
