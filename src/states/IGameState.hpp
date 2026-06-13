#pragma once

#include <SFML/Graphics.hpp>

namespace tc {

class Game;

// Base interface for every screen in the game's state machine
// (MainMenu, Play, Death, MetaUpgrade). Owned by Game on a std::stack.
class IGameState {
public:
    explicit IGameState(Game& game) : game(game) {}
    virtual ~IGameState() = default;

    virtual void handleInput(const sf::Event& event) = 0;
    virtual void update(float dt) = 0;
    virtual void render(sf::RenderWindow& window) = 0;

protected:
    Game& game;
};

} // namespace tc
