#pragma once

#include <SFML/Graphics.hpp>

#include "states/IGameState.hpp"

namespace tc {

// First screen shown on launch: start a new run, switch language or quit.
class MainMenuState : public IGameState {
public:
    explicit MainMenuState(Game& game);

    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    void refreshTexts();

    bool fontLoaded = false;

    sf::Text titleText;
    sf::Text newGameText;
    sf::Text quitText;

    sf::RectangleShape newGameButton;
    sf::RectangleShape quitButton;
    sf::RectangleShape langRuButton;
    sf::RectangleShape langEnButton;

    sf::Text langRuText;
    sf::Text langEnText;
};

} // namespace tc
