#include "ui/TutorialHint.hpp"

#include "core/TextUtils.hpp"

namespace tc {

namespace {
constexpr float PANEL_W = 280.0f;
constexpr float PANEL_H = 190.0f;
constexpr float PANEL_Y = 80.0f;
constexpr float MARGIN = 16.0f;
constexpr float TAB_W = 28.0f;
constexpr float TAB_H = 60.0f;
constexpr float SCREEN_W = 1280.0f;
}

TutorialHint::TutorialHint()
{
    panel.setSize({PANEL_W, PANEL_H});
    panel.setPosition(SCREEN_W - PANEL_W - MARGIN, PANEL_Y);
    panel.setFillColor(sf::Color(20, 20, 30, 200));
    panel.setOutlineColor(sf::Color(90, 90, 120));
    panel.setOutlineThickness(2.0f);

    tab.setSize({TAB_W, TAB_H});
    tab.setFillColor(sf::Color(40, 40, 60, 220));
    tab.setOutlineColor(sf::Color(90, 90, 120));
    tab.setOutlineThickness(2.0f);
}

sf::FloatRect TutorialHint::tabBounds() const
{
    const float y = PANEL_Y + PANEL_H / 2.0f - TAB_H / 2.0f;
    if (visible) {
        return sf::FloatRect(SCREEN_W - PANEL_W - MARGIN - TAB_W, y, TAB_W, TAB_H);
    }
    return sf::FloatRect(SCREEN_W - TAB_W, y, TAB_W, TAB_H);
}

bool TutorialHint::handleClick(sf::Vector2f point)
{
    if (tabBounds().contains(point)) {
        visible = !visible;
        return true;
    }
    return false;
}

void TutorialHint::render(sf::RenderWindow& window, const Localization& localization, const FontManager& fontManager)
{
    const sf::FloatRect tabRect = tabBounds();
    tab.setPosition(tabRect.left, tabRect.top);
    window.draw(tab);

    if (fontManager.isLoaded()) {
        const sf::Font& font = fontManager.getFont(localization.getCurrentLanguage());

        if (!textInitialized) {
            arrowText.setFont(font);
            arrowText.setCharacterSize(20);
            arrowText.setFillColor(sf::Color::White);

            titleText.setFont(font);
            titleText.setCharacterSize(20);
            titleText.setFillColor(sf::Color(220, 200, 120));

            for (auto& line : lineTexts) {
                line.setFont(font);
                line.setCharacterSize(15);
                line.setFillColor(sf::Color::White);
            }

            textInitialized = true;
        }

        arrowText.setString(visible ? ">" : "<");
        const sf::FloatRect bounds = arrowText.getLocalBounds();
        arrowText.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
        arrowText.setPosition(tabRect.left + tabRect.width / 2.0f, tabRect.top + tabRect.height / 2.0f);
        window.draw(arrowText);
    }

    if (!visible) {
        return;
    }

    window.draw(panel);

    if (!fontManager.isLoaded()) {
        return;
    }

    const float textX = panel.getPosition().x + 14.0f;
    float y = panel.getPosition().y + 12.0f;

    titleText.setString(toSfString(localization.get("tutorial.title")));
    titleText.setPosition(textX, y);
    window.draw(titleText);
    y += 36.0f;

    const char* lineKeys[] = {
        "tutorial.move",
        "tutorial.combat",
        "tutorial.potion",
        "tutorial.menu",
    };

    for (size_t i = 0; i < lineTexts.size(); ++i) {
        lineTexts[i].setString(toSfString(localization.get(lineKeys[i])));
        lineTexts[i].setPosition(textX, y);
        window.draw(lineTexts[i]);
        y += 30.0f;
    }
}

} // namespace tc
