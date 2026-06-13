#include "ui/MetaUpgradeScreen.hpp"

#include <string>

#include "core/TextUtils.hpp"

namespace tc {

namespace {
constexpr float PANEL_X = 440.0f;
constexpr float ROW_Y[3] = {220.0f, 300.0f, 380.0f};
constexpr float BUTTON_SIZE = 50.0f;
constexpr float BUTTON_X = 760.0f;

constexpr float CONTINUE_X = PANEL_X;
constexpr float CONTINUE_Y = 500.0f;
constexpr float CONTINUE_W = 400.0f;
constexpr float CONTINUE_H = 60.0f;
}

MetaUpgradeScreen::MetaUpgradeScreen()
{
    sf::Vector2f buttonSize(BUTTON_SIZE, BUTTON_SIZE);

    strengthButton.setSize(buttonSize);
    strengthButton.setPosition(BUTTON_X, ROW_Y[0]);
    strengthButton.setFillColor(sf::Color(90, 160, 90));

    enduranceButton.setSize(buttonSize);
    enduranceButton.setPosition(BUTTON_X, ROW_Y[1]);
    enduranceButton.setFillColor(sf::Color(90, 160, 90));

    healthButton.setSize(buttonSize);
    healthButton.setPosition(BUTTON_X, ROW_Y[2]);
    healthButton.setFillColor(sf::Color(90, 160, 90));

    continueButton.setSize({CONTINUE_W, CONTINUE_H});
    continueButton.setPosition(CONTINUE_X, CONTINUE_Y);
    continueButton.setFillColor(sf::Color(70, 70, 90));
}

void MetaUpgradeScreen::render(sf::RenderWindow& window, const Localization& localization,
    const FontManager& fontManager, const MetaStats& stats)
{
    window.draw(strengthButton);
    window.draw(enduranceButton);
    window.draw(healthButton);
    window.draw(continueButton);

    if (!fontManager.isLoaded()) {
        return;
    }

    const sf::Font& font = fontManager.getFont(localization.getCurrentLanguage());

    sf::Text title;
    title.setFont(font);
    title.setCharacterSize(32);
    title.setFillColor(sf::Color::White);
    title.setString(toSfString(localization.get("meta.title")));
    title.setPosition(PANEL_X, 100.0f);
    window.draw(title);

    sf::Text points;
    points.setFont(font);
    points.setCharacterSize(20);
    points.setFillColor(sf::Color(220, 200, 120));
    points.setString(toSfString(localization.get("meta.points") + ": " + std::to_string(stats.points)));
    points.setPosition(PANEL_X, 160.0f);
    window.draw(points);

    const char* rowKeys[3] = {"meta.strength", "meta.endurance", "meta.health"};
    const int rowValues[3] = {stats.strength, stats.endurance, stats.health};

    for (int i = 0; i < 3; ++i) {
        sf::Text label;
        label.setFont(font);
        label.setCharacterSize(22);
        label.setFillColor(sf::Color::White);
        label.setString(toSfString(localization.get(rowKeys[i]) + ": " + std::to_string(rowValues[i])));
        label.setPosition(PANEL_X, ROW_Y[i] + 12.0f);
        window.draw(label);

        sf::Text plus;
        plus.setFont(font);
        plus.setCharacterSize(28);
        plus.setFillColor(sf::Color::White);
        plus.setString("+");
        plus.setPosition(BUTTON_X + BUTTON_SIZE * 0.5f - 8.0f, ROW_Y[i] + 6.0f);
        window.draw(plus);
    }

    sf::Text continueText;
    continueText.setFont(font);
    continueText.setCharacterSize(22);
    continueText.setFillColor(sf::Color::White);
    continueText.setString(toSfString(localization.get("meta.continue")));
    continueText.setPosition(CONTINUE_X + 20.0f, CONTINUE_Y + 14.0f);
    window.draw(continueText);
}

MetaUpgradeScreen::Button MetaUpgradeScreen::getButtonAt(sf::Vector2f point) const
{
    if (strengthButton.getGlobalBounds().contains(point)) return Button::STRENGTH;
    if (enduranceButton.getGlobalBounds().contains(point)) return Button::ENDURANCE;
    if (healthButton.getGlobalBounds().contains(point)) return Button::HEALTH;
    if (continueButton.getGlobalBounds().contains(point)) return Button::CONTINUE;
    return Button::NONE;
}

} // namespace tc
