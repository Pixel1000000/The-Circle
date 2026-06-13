#pragma once

#include <SFML/Graphics.hpp>

#include "meta/MetaProgression.hpp"
#include "states/IGameState.hpp"

namespace tc {

// Shown when a run ends (player death or boss defeated). Summarizes the run
// and the meta points earned, then continues on to MetaUpgradeState.
class DeathState : public IGameState {
public:
    DeathState(Game& game, const RunSummary& summary, bool victory, int pointsEarned);

    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    RunSummary summary;
    bool victory;
    int pointsEarned;

    bool fontLoaded = false;

    sf::RectangleShape continueButton;
    sf::Text continueText;
};

} // namespace tc
