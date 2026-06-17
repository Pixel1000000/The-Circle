#pragma once

#ifdef TC_DEBUG

#include "debug/DebugContext.hpp"
#include "debug/DebugWidgets.hpp"

namespace tc {

// Display tab: render-debug toggles plus a live read-only info block.
class DebugDisplayPanel {
public:
    void init(const sf::Font& font, sf::Vector2f origin);
    void onOpen(DebugContext& ctx);

    void handleMousePressed(sf::Vector2f point, DebugContext& ctx);

    void render(sf::RenderWindow& window, DebugContext& ctx);

private:
    DebugCheckbox hitboxesCheckbox;
    DebugCheckbox hpNumbersCheckbox;
    DebugCheckbox velocityVectorsCheckbox;
    DebugCheckbox blizzardZonesCheckbox;

    sf::Text infoText;
};

} // namespace tc

#endif // TC_DEBUG
