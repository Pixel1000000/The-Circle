#pragma once

#ifdef TC_DEBUG

#include "core/FontManager.hpp"
#include "core/Localization.hpp"
#include "debug/DebugContext.hpp"
#include "debug/DebugDisplayPanel.hpp"
#include "debug/DebugPlayerPanel.hpp"
#include "debug/DebugSpawnPanel.hpp"
#include "debug/DebugWidgets.hpp"
#include "debug/DebugWorldPanel.hpp"

namespace tc {

class Game;

// Full-screen overlay opened with F1 from PlayState. Owns the four debug
// tabs and routes input/render calls to whichever one is active.
class DebugOverlay {
public:
    // Builds every widget. Must be called once the FontManager has finished
    // loading (Game loads fonts before any state is pushed, so this is safe
    // to call from PlayState's constructor).
    void init(Game& game);

    // Resets all panels to mirror the current game state. Call right after
    // setting debugOpen = true.
    void onOpen(DebugContext& ctx);

    void handleInput(const sf::Event& event, sf::RenderWindow& window, DebugContext& ctx);
    void render(sf::RenderWindow& window, DebugContext& ctx);

    // True exactly once after the close (x) button was clicked; PlayState
    // should clear debugOpen in response and not call this again until the
    // overlay is reopened.
    bool consumeCloseRequest();

private:
    enum class Tab { Spawn, Player, World, Display };

    void renderTabs(sf::RenderWindow& window) const;

    sf::RectangleShape background;
    sf::RectangleShape closeButton;
    sf::Text closeLabel;

    DebugButton spawnTabButton;
    DebugButton playerTabButton;
    DebugButton worldTabButton;
    DebugButton displayTabButton;

    Tab currentTab = Tab::Spawn;
    bool closeRequested = false;

    DebugSpawnPanel spawnPanel;
    DebugPlayerPanel playerPanel;
    DebugWorldPanel worldPanel;
    DebugDisplayPanel displayPanel;
};

} // namespace tc

#endif // TC_DEBUG
