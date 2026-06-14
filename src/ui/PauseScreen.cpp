#include "ui/PauseScreen.hpp"

#include "core/TextUtils.hpp"

namespace tc {

namespace {
constexpr float SCREEN_W = 1280.0f;
constexpr float SCREEN_H = 720.0f;

constexpr float BUTTON_W = 320.0f;
constexpr float BUTTON_H = 60.0f;
constexpr float BUTTON_X = (SCREEN_W - BUTTON_W) / 2.0f;
constexpr float RESUME_Y = 340.0f;
constexpr float MAIN_MENU_Y = 420.0f;

void centerTextInRect(sf::Text& text, const sf::RectangleShape& rect)
{
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
    text.setPosition(rect.getPosition() + rect.getSize() / 2.0f);
}
} // namespace

PauseScreen::PauseScreen()
{
    overlay.setSize({SCREEN_W, SCREEN_H});
    overlay.setFillColor(sf::Color(0, 0, 0, 160));

    resumeButton.setSize({BUTTON_W, BUTTON_H});
    resumeButton.setPosition(BUTTON_X, RESUME_Y);
    resumeButton.setFillColor(sf::Color(90, 160, 90));

    mainMenuButton.setSize({BUTTON_W, BUTTON_H});
    mainMenuButton.setPosition(BUTTON_X, MAIN_MENU_Y);
    mainMenuButton.setFillColor(sf::Color(160, 90, 90));
}

void PauseScreen::render(sf::RenderWindow& window, const Localization& localization, const FontManager& fontManager)
{
    window.draw(overlay);
    window.draw(resumeButton);
    window.draw(mainMenuButton);

    if (!fontManager.isLoaded()) {
        return;
    }

    const sf::Font& font = fontManager.getFont(localization.getCurrentLanguage());

    sf::Text title;
    title.setFont(font);
    title.setCharacterSize(48);
    title.setFillColor(sf::Color::White);
    title.setString(toSfString(localization.get("pause.title")));
    const sf::FloatRect titleBounds = title.getLocalBounds();
    title.setOrigin(titleBounds.left + titleBounds.width / 2.0f, titleBounds.top + titleBounds.height / 2.0f);
    title.setPosition(SCREEN_W / 2.0f, 240.0f);
    window.draw(title);

    sf::Text resumeText;
    resumeText.setFont(font);
    resumeText.setCharacterSize(24);
    resumeText.setFillColor(sf::Color::White);
    resumeText.setString(toSfString(localization.get("pause.resume")));
    centerTextInRect(resumeText, resumeButton);
    window.draw(resumeText);

    sf::Text mainMenuText;
    mainMenuText.setFont(font);
    mainMenuText.setCharacterSize(24);
    mainMenuText.setFillColor(sf::Color::White);
    mainMenuText.setString(toSfString(localization.get("pause.mainmenu")));
    centerTextInRect(mainMenuText, mainMenuButton);
    window.draw(mainMenuText);
}

PauseScreen::Button PauseScreen::getButtonAt(sf::Vector2f point) const
{
    if (resumeButton.getGlobalBounds().contains(point)) return Button::RESUME;
    if (mainMenuButton.getGlobalBounds().contains(point)) return Button::MAIN_MENU;
    return Button::NONE;
}

} // namespace tc
