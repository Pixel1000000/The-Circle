#include "states/MetaUpgradeState.hpp"

#include "Game.hpp"
#include "states/MainMenuState.hpp"

namespace tc {

MetaUpgradeState::MetaUpgradeState(Game& game)
    : IGameState(game)
{
}

void MetaUpgradeState::handleInput(const sf::Event& event)
{
    if (event.type != sf::Event::MouseButtonPressed || event.mouseButton.button != sf::Mouse::Left) {
        return;
    }

    const sf::Vector2f point = game.getWindow().mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
    const MetaUpgradeScreen::Button button = screen.getButtonAt(point);

    auto& meta = game.getMetaProgression();

    switch (button) {
    case MetaUpgradeScreen::Button::STRENGTH:
        meta.spendPointOnStrength();
        break;
    case MetaUpgradeScreen::Button::ENDURANCE:
        meta.spendPointOnEndurance();
        break;
    case MetaUpgradeScreen::Button::HEALTH:
        meta.spendPointOnHealth();
        break;
    case MetaUpgradeScreen::Button::CONTINUE:
        meta.save();
        game.changeState(std::make_unique<MainMenuState>(game));
        break;
    case MetaUpgradeScreen::Button::NONE:
    default:
        break;
    }
}

void MetaUpgradeState::update(float dt)
{
    (void)dt;
}

void MetaUpgradeState::render(sf::RenderWindow& window)
{
    window.clear(sf::Color(15, 15, 20));
    screen.render(window, game.getLocalization(), game.getFontManager(), game.getMetaProgression().getStats());
}

} // namespace tc
