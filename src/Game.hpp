#pragma once

#include <memory>
#include <stack>

#include <SFML/Graphics.hpp>

#include "core/AudioManager.hpp"
#include "core/FontManager.hpp"
#include "core/Localization.hpp"
#include "meta/MetaProgression.hpp"
#include "states/IGameState.hpp"

namespace tc {

// Owns the window, the state stack and the cross-cutting services
// (localization, audio, meta-progression) shared by every state.
class Game {
public:
    static constexpr unsigned int LOGICAL_WIDTH = 1280;
    static constexpr unsigned int LOGICAL_HEIGHT = 720;

    Game();

    void run();
    void quit();

    // State changes are deferred and applied at the start of the next frame,
    // so it is safe to call these from within a state's update()/handleInput().
    void pushState(std::unique_ptr<IGameState> state);
    void popState();
    void changeState(std::unique_ptr<IGameState> state);

    sf::RenderWindow& getWindow();
    Localization& getLocalization();
    AudioManager& getAudio();
    MetaProgression& getMetaProgression();
    FontManager& getFontManager();

private:
    enum class PendingAction { None, Push, Pop, Change };

    void applyPendingStateChange();
    void processInput();
    void update(float dt);
    void render();
    void applyLetterboxView();

    sf::RenderWindow window;
    sf::View view;
    std::stack<std::unique_ptr<IGameState>> states;

    PendingAction pendingAction = PendingAction::None;
    std::unique_ptr<IGameState> pendingState;

    Localization localization;
    AudioManager audio;
    MetaProgression metaProgression;
    FontManager fontManager;

    bool running = true;
};

} // namespace tc
