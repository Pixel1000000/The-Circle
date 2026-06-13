#include "states/MainMenuState.hpp"

#include "Game.hpp"
#include "core/TextUtils.hpp"
#include "states/PlayState.hpp"

namespace tc {

namespace {
void centerTextInRect(sf::Text& text, const sf::RectangleShape& rect)
{
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
    text.setPosition(rect.getPosition() + rect.getSize() / 2.0f);
}
}

MainMenuState::MainMenuState(Game& game)
    : IGameState(game)
{
    fontLoaded = game.getFontManager().isLoaded();

    newGameButton.setSize({400.0f, 70.0f});
    newGameButton.setPosition(440.0f, 320.0f);
    newGameButton.setFillColor(sf::Color(70, 130, 90));

    quitButton.setSize({400.0f, 70.0f});
    quitButton.setPosition(440.0f, 420.0f);
    quitButton.setFillColor(sf::Color(130, 70, 70));

    langRuButton.setSize({50.0f, 34.0f});
    langRuButton.setPosition(1140.0f, 20.0f);
    langRuButton.setFillColor(sf::Color(60, 90, 160));

    langEnButton.setSize({50.0f, 34.0f});
    langEnButton.setPosition(1200.0f, 20.0f);
    langEnButton.setFillColor(sf::Color(60, 90, 160));

    if (fontLoaded) {
        titleText.setCharacterSize(64);
        newGameText.setCharacterSize(28);
        quitText.setCharacterSize(28);
        langRuText.setCharacterSize(18);
        langEnText.setCharacterSize(18);

        titleText.setFillColor(sf::Color::White);
        newGameText.setFillColor(sf::Color::White);
        quitText.setFillColor(sf::Color::White);
        langRuText.setFillColor(sf::Color::White);
        langEnText.setFillColor(sf::Color::White);

        titleText.setString("The Circle");
        langRuText.setString("RU");
        langEnText.setString("EN");
    }

    refreshTexts();
}

void MainMenuState::refreshTexts()
{
    if (!fontLoaded) return;

    const auto& localization = game.getLocalization();
    const sf::Font& font = game.getFontManager().getFont(localization.getCurrentLanguage());

    titleText.setFont(font);
    newGameText.setFont(font);
    quitText.setFont(font);
    langRuText.setFont(font);
    langEnText.setFont(font);

    newGameText.setString(toSfString(localization.get("menu.newgame")));
    quitText.setString(toSfString(localization.get("menu.quit")));

    const sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin(titleBounds.left + titleBounds.width / 2.0f, titleBounds.top + titleBounds.height / 2.0f);
    titleText.setPosition(Game::LOGICAL_WIDTH / 2.0f, 150.0f);

    centerTextInRect(newGameText, newGameButton);
    centerTextInRect(quitText, quitButton);
    centerTextInRect(langRuText, langRuButton);
    centerTextInRect(langEnText, langEnButton);
}

void MainMenuState::handleInput(const sf::Event& event)
{
    if (event.type != sf::Event::MouseButtonPressed || event.mouseButton.button != sf::Mouse::Left) {
        return;
    }

    const sf::Vector2f point = game.getWindow().mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});

    if (newGameButton.getGlobalBounds().contains(point)) {
        game.changeState(std::make_unique<PlayState>(game));
    } else if (quitButton.getGlobalBounds().contains(point)) {
        game.quit();
    } else if (langRuButton.getGlobalBounds().contains(point)) {
        game.getLocalization().load("ru");
        refreshTexts();
    } else if (langEnButton.getGlobalBounds().contains(point)) {
        game.getLocalization().load("en");
        refreshTexts();
    }
}

void MainMenuState::update(float dt)
{
    (void)dt;
}

void MainMenuState::render(sf::RenderWindow& window)
{
    window.clear(sf::Color(18, 20, 28));

    window.draw(newGameButton);
    window.draw(quitButton);
    window.draw(langRuButton);
    window.draw(langEnButton);

    if (fontLoaded) {
        window.draw(titleText);
        window.draw(newGameText);
        window.draw(quitText);
        window.draw(langRuText);
        window.draw(langEnText);
    }
}

} // namespace tc
