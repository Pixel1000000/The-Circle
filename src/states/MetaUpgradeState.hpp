#pragma once

#include <SFML/Graphics.hpp>

#include "states/IGameState.hpp"
#include "ui/MetaUpgradeScreen.hpp"

namespace tc {

// Post-run screen where the player spends meta points on Strength, Endurance
// and Health, then returns to the main menu.
class MetaUpgradeState : public IGameState {
public:
    explicit MetaUpgradeState(Game& game);

    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    MetaUpgradeScreen screen;
};

} // namespace tc
