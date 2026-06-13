#pragma once

#include <SFML/Graphics.hpp>

#include "core/FontManager.hpp"
#include "core/Localization.hpp"

namespace tc {

// Collapsible controls reminder shown on the right edge of the screen at the
// start of a run. Click the tab to slide it in/out.
class TutorialHint {
public:
    TutorialHint();

    // Returns true if the click landed on the toggle tab (and was consumed).
    bool handleClick(sf::Vector2f point);

    void render(sf::RenderWindow& window, const Localization& localization, const FontManager& fontManager);

private:
    sf::FloatRect tabBounds() const;

    bool visible = true;

    sf::RectangleShape panel;
    sf::RectangleShape tab;
};

} // namespace tc
