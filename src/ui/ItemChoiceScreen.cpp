#include "ui/ItemChoiceScreen.hpp"

#include <cmath>
#include <string>

#include "core/TextUtils.hpp"

namespace tc {

namespace {
constexpr float SCREEN_W = 1280.0f;
constexpr float SCREEN_H = 720.0f;

constexpr float BUTTON_W = 320.0f;
constexpr float BUTTON_H = 60.0f;
constexpr float BUTTON_GAP = 40.0f;
constexpr float BUTTONS_Y = 420.0f;
constexpr float LEFT_X = (SCREEN_W - (BUTTON_W * 2.0f + BUTTON_GAP)) / 2.0f;
constexpr float RIGHT_X = LEFT_X + BUTTON_W + BUTTON_GAP;

constexpr float CHECKBOX_SIZE = 28.0f;
constexpr float CHECKBOX_Y = 520.0f;

void centerTextInRect(sf::Text& text, const sf::RectangleShape& rect)
{
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
    text.setPosition(rect.getPosition() + rect.getSize() / 2.0f);
}

std::string slotLabel(const Localization& localization, int slot)
{
    switch (slot) {
    case 0: return localization.get("equipment.helmet");
    case 1: return localization.get("equipment.chest");
    case 2: return localization.get("equipment.legs");
    default: return localization.get("equipment.weapon");
    }
}

std::string elementLabel(const Localization& localization, Element element, float percent)
{
    if (element == Element::NONE) {
        return localization.get("element.none");
    }

    std::string key;
    switch (element) {
    case Element::NATURE: key = "element.nature"; break;
    case Element::FIRE: key = "element.fire"; break;
    case Element::ICE: key = "element.ice"; break;
    case Element::DECAY: key = "element.decay"; break;
    default: key = "element.none"; break;
    }

    const int percentValue = static_cast<int>(std::round(percent * 100.0f));
    return localization.get(key) + " +" + std::to_string(percentValue) + "%";
}

} // namespace

ItemChoiceScreen::ItemChoiceScreen()
{
    overlay.setSize({SCREEN_W, SCREEN_H});
    overlay.setFillColor(sf::Color(0, 0, 0, 160));

    takeNewButton.setSize({BUTTON_W, BUTTON_H});
    takeNewButton.setPosition(LEFT_X, BUTTONS_Y);
    takeNewButton.setFillColor(sf::Color(90, 160, 90));

    upgradeButton.setSize({BUTTON_W, BUTTON_H});
    upgradeButton.setPosition(RIGHT_X, BUTTONS_Y);
    upgradeButton.setFillColor(sf::Color(90, 130, 160));

    checkbox.setSize({CHECKBOX_SIZE, CHECKBOX_SIZE});
    checkbox.setPosition(LEFT_X, CHECKBOX_Y);
    checkbox.setOutlineThickness(2.0f);
    checkbox.setOutlineColor(sf::Color::White);
}

void ItemChoiceScreen::open()
{
    dontAskAgain = false;
}

void ItemChoiceScreen::render(sf::RenderWindow& window, const Localization& localization, const FontManager& fontManager,
    int slot, int newTier, Element newElement, float newPercent,
    int currentTier, Element currentElement, float currentPercent)
{
    window.draw(overlay);
    window.draw(takeNewButton);
    window.draw(upgradeButton);

    checkbox.setFillColor(dontAskAgain ? sf::Color(90, 160, 90) : sf::Color(60, 60, 60));
    window.draw(checkbox);

    if (!fontManager.isLoaded()) {
        return;
    }

    const sf::Font& font = fontManager.getFont(localization.getCurrentLanguage());
    const std::string slotName = slotLabel(localization, slot);

    sf::Text title;
    title.setFont(font);
    title.setCharacterSize(40);
    title.setFillColor(sf::Color::White);
    title.setString(toSfString(localization.get("itemchoice.title")));
    const sf::FloatRect titleBounds = title.getLocalBounds();
    title.setOrigin(titleBounds.left + titleBounds.width / 2.0f, titleBounds.top + titleBounds.height / 2.0f);
    title.setPosition(SCREEN_W / 2.0f, 130.0f);
    window.draw(title);

    sf::Text currentLabel;
    currentLabel.setFont(font);
    currentLabel.setCharacterSize(24);
    currentLabel.setFillColor(sf::Color(200, 200, 200));
    currentLabel.setString(toSfString(localization.get("itemchoice.current") + ": " + slotName + " T" + std::to_string(currentTier)));
    currentLabel.setPosition(LEFT_X, 220.0f);
    window.draw(currentLabel);

    sf::Text currentInfo;
    currentInfo.setFont(font);
    currentInfo.setCharacterSize(22);
    currentInfo.setFillColor(sf::Color::White);
    currentInfo.setString(toSfString(elementLabel(localization, currentElement, currentPercent)));
    currentInfo.setPosition(LEFT_X, 255.0f);
    window.draw(currentInfo);

    sf::Text newLabel;
    newLabel.setFont(font);
    newLabel.setCharacterSize(24);
    newLabel.setFillColor(sf::Color(200, 200, 200));
    newLabel.setString(toSfString(localization.get("itemchoice.new") + ": " + slotName + " T" + std::to_string(newTier)));
    newLabel.setPosition(LEFT_X, 320.0f);
    window.draw(newLabel);

    sf::Text newInfo;
    newInfo.setFont(font);
    newInfo.setCharacterSize(22);
    newInfo.setFillColor(sf::Color(255, 215, 0));
    newInfo.setString(toSfString(elementLabel(localization, newElement, newPercent)));
    newInfo.setPosition(LEFT_X, 355.0f);
    window.draw(newInfo);

    sf::Text takeNewText;
    takeNewText.setFont(font);
    takeNewText.setCharacterSize(22);
    takeNewText.setFillColor(sf::Color::White);
    takeNewText.setString(toSfString(localization.get("itemchoice.take")));
    centerTextInRect(takeNewText, takeNewButton);
    window.draw(takeNewText);

    // Show the elemental percent transition above the upgrade button.
    {
        const float resultPercent = currentPercent + newPercent * 0.5f;
        const Element resultElement = (currentElement != Element::NONE) ? currentElement : newElement;
        const std::string fromStr = elementLabel(localization, currentElement, currentPercent);
        const std::string toStr = elementLabel(localization, resultElement, resultPercent);

        sf::Text transitionText;
        transitionText.setFont(font);
        transitionText.setCharacterSize(18);
        transitionText.setFillColor(sf::Color(180, 220, 255));
        transitionText.setString(toSfString(fromStr + " → " + toStr));
        const sf::FloatRect tb = transitionText.getLocalBounds();
        transitionText.setOrigin(tb.left + tb.width / 2.0f, tb.top + tb.height / 2.0f);
        transitionText.setPosition(RIGHT_X + BUTTON_W / 2.0f, BUTTONS_Y - 22.0f);
        window.draw(transitionText);
    }

    sf::Text upgradeText;
    upgradeText.setFont(font);
    upgradeText.setCharacterSize(22);
    upgradeText.setFillColor(sf::Color::White);
    upgradeText.setString(toSfString(localization.get("itemchoice.upgrade")));
    centerTextInRect(upgradeText, upgradeButton);
    window.draw(upgradeText);

    sf::Text checkboxText;
    checkboxText.setFont(font);
    checkboxText.setCharacterSize(20);
    checkboxText.setFillColor(sf::Color::White);
    checkboxText.setString(toSfString(localization.get("itemchoice.dontask")));
    checkboxText.setPosition(LEFT_X + CHECKBOX_SIZE + 12.0f, CHECKBOX_Y + 2.0f);
    window.draw(checkboxText);
}

ItemChoiceScreen::Button ItemChoiceScreen::getButtonAt(sf::Vector2f point) const
{
    if (takeNewButton.getGlobalBounds().contains(point)) return Button::TAKE_NEW;
    if (upgradeButton.getGlobalBounds().contains(point)) return Button::UPGRADE_CURRENT;
    if (checkbox.getGlobalBounds().contains(point)) return Button::CHECKBOX;
    return Button::NONE;
}

void ItemChoiceScreen::toggleDontAskAgain()
{
    dontAskAgain = !dontAskAgain;
}

bool ItemChoiceScreen::isDontAskAgain() const
{
    return dontAskAgain;
}

} // namespace tc
