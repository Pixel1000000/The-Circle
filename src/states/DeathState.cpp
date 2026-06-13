#include "states/DeathState.hpp"

#include <string>

#include "Game.hpp"
#include "core/TextUtils.hpp"
#include "states/MetaUpgradeState.hpp"

namespace tc {

namespace {
constexpr float PANEL_X = 440.0f;
constexpr float CONTINUE_Y = 520.0f;
}

DeathState::DeathState(Game& game, const RunSummary& summary, bool victory, int pointsEarned)
    : IGameState(game)
    , summary(summary)
    , victory(victory)
    , pointsEarned(pointsEarned)
{
    fontLoaded = game.getFontManager().isLoaded();

    continueButton.setSize({400.0f, 60.0f});
    continueButton.setPosition(PANEL_X, CONTINUE_Y);
    continueButton.setFillColor(sf::Color(70, 70, 90));

    if (fontLoaded) {
        const sf::Font& font = game.getFontManager().getFont(game.getLocalization().getCurrentLanguage());
        continueText.setFont(font);
        continueText.setCharacterSize(22);
        continueText.setFillColor(sf::Color::White);
        continueText.setString(toSfString(game.getLocalization().get("death.continue")));

        const sf::FloatRect bounds = continueText.getLocalBounds();
        continueText.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
        continueText.setPosition(continueButton.getPosition() + continueButton.getSize() / 2.0f);
    }
}

void DeathState::handleInput(const sf::Event& event)
{
    if (event.type != sf::Event::MouseButtonPressed || event.mouseButton.button != sf::Mouse::Left) {
        return;
    }

    const sf::Vector2f point = game.getWindow().mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
    if (continueButton.getGlobalBounds().contains(point)) {
        game.changeState(std::make_unique<MetaUpgradeState>(game));
    }
}

void DeathState::update(float dt)
{
    (void)dt;
}

void DeathState::render(sf::RenderWindow& window)
{
    window.clear(sf::Color(15, 15, 20));
    window.draw(continueButton);

    if (!fontLoaded) {
        return;
    }

    const auto& localization = game.getLocalization();
    const sf::Font& font = game.getFontManager().getFont(localization.getCurrentLanguage());

    sf::Text title;
    title.setFont(font);
    title.setCharacterSize(48);
    title.setFillColor(victory ? sf::Color(220, 200, 120) : sf::Color(220, 80, 80));
    title.setString(toSfString(localization.get(victory ? "death.victory" : "death.defeat")));
    title.setPosition(PANEL_X, 100.0f);
    window.draw(title);

    const std::string lines[] = {
        localization.get("death.kills") + ": " + std::to_string(summary.kills),
        localization.get("death.biomes") + ": " + std::to_string(summary.biomesCleared),
        localization.get("death.fragments") + ": " + std::to_string(summary.keyFragments),
        localization.get("death.boss") + ": " + localization.get(summary.bossDefeated ? "common.yes" : "common.no"),
        localization.get("death.points") + ": " + std::to_string(pointsEarned),
    };

    float y = 200.0f;
    for (const auto& line : lines) {
        sf::Text text;
        text.setFont(font);
        text.setCharacterSize(24);
        text.setFillColor(sf::Color::White);
        text.setString(toSfString(line));
        text.setPosition(PANEL_X, y);
        window.draw(text);
        y += 40.0f;
    }

    window.draw(continueText);
}

} // namespace tc
