#pragma once

#ifdef TC_DEBUG

#include "debug/DebugContext.hpp"
#include "debug/DebugWidgets.hpp"

namespace tc {

// Player tab: HP, invulnerability, one-shot damage and live stat sliders.
class DebugPlayerPanel {
public:
    void init(const sf::Font& font, sf::Vector2f origin);
    void onOpen(DebugContext& ctx);

    void handleMousePressed(sf::Vector2f point, DebugContext& ctx);
    void handleMouseMoved(sf::Vector2f point, DebugContext& ctx);
    void handleMouseReleased();

    void render(sf::RenderWindow& window, DebugContext& ctx);

private:
    void applySlidersToEntity(DebugContext& ctx);

    sf::Text hpText;

    DebugButton fillHpButton;
    DebugCheckbox godModeCheckbox;
    DebugCheckbox oneShotCheckbox;

    DebugSlider speedSlider;
    DebugSlider armorSlider;
    DebugSlider damageSlider;

    DebugButton fillPotionButton;

    int savedDamageValue = 0;
    bool oneShotActive = false;
};

} // namespace tc

#endif // TC_DEBUG
