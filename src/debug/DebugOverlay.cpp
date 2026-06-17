#include "debug/DebugOverlay.hpp"

#ifdef TC_DEBUG

#include "Game.hpp"

namespace tc {

namespace {
constexpr float OVERLAY_W = 800.0f;
constexpr float OVERLAY_H = 560.0f;
constexpr float OVERLAY_X = (Game::LOGICAL_WIDTH - OVERLAY_W) / 2.0f;
constexpr float OVERLAY_Y = (Game::LOGICAL_HEIGHT - OVERLAY_H) / 2.0f;

constexpr float TAB_Y = OVERLAY_Y + 12.0f;
constexpr float TAB_W = 130.0f;
constexpr float TAB_H = 32.0f;

constexpr float CONTENT_X = OVERLAY_X + 24.0f;
constexpr float CONTENT_Y = TAB_Y + TAB_H + 20.0f;

constexpr float CLOSE_SIZE = 28.0f;
} // namespace

void DebugOverlay::init(Game& game)
{
    background.setSize({OVERLAY_W, OVERLAY_H});
    background.setPosition(OVERLAY_X, OVERLAY_Y);
    background.setFillColor(sf::Color(15, 15, 20, 235));
    background.setOutlineColor(sf::Color(120, 130, 150));
    background.setOutlineThickness(2.0f);

    const sf::Font& font = game.getFontManager().getFont("en");

    closeButton.setSize({CLOSE_SIZE, CLOSE_SIZE});
    closeButton.setPosition(OVERLAY_X + OVERLAY_W - CLOSE_SIZE - 10.0f, OVERLAY_Y + 10.0f);
    closeButton.setFillColor(sf::Color(110, 60, 60));
    closeLabel.setFont(font);
    closeLabel.setCharacterSize(18);
    closeLabel.setFillColor(sf::Color::White);
    closeLabel.setString("X");
    {
        const sf::FloatRect bounds = closeLabel.getLocalBounds();
        closeLabel.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
        closeLabel.setPosition(closeButton.getPosition() + closeButton.getSize() / 2.0f);
    }

    spawnTabButton.setup(font, "Spawn", {OVERLAY_X + 12.0f, TAB_Y}, {TAB_W, TAB_H});
    playerTabButton.setup(font, "Player", {OVERLAY_X + 12.0f + TAB_W, TAB_Y}, {TAB_W, TAB_H});
    worldTabButton.setup(font, "World", {OVERLAY_X + 12.0f + TAB_W * 2.0f, TAB_Y}, {TAB_W, TAB_H});
    displayTabButton.setup(font, "Display", {OVERLAY_X + 12.0f + TAB_W * 3.0f, TAB_Y}, {TAB_W, TAB_H});

    const sf::Vector2f contentOrigin{CONTENT_X, CONTENT_Y};
    spawnPanel.init(font, contentOrigin);
    playerPanel.init(font, contentOrigin);
    worldPanel.init(font, contentOrigin);
    displayPanel.init(font, contentOrigin);
}

void DebugOverlay::onOpen(DebugContext& ctx)
{
    spawnPanel.onOpen(ctx);
    playerPanel.onOpen(ctx);
    worldPanel.onOpen(ctx);
    displayPanel.onOpen(ctx);
}

void DebugOverlay::handleInput(const sf::Event& event, sf::RenderWindow& window, DebugContext& ctx)
{
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        const sf::Vector2f point = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});

        if (closeButton.getGlobalBounds().contains(point)) {
            closeRequested = true;
            return;
        }

        if (spawnTabButton.isPressed(point)) { currentTab = Tab::Spawn; return; }
        if (playerTabButton.isPressed(point)) { currentTab = Tab::Player; return; }
        if (worldTabButton.isPressed(point)) { currentTab = Tab::World; return; }
        if (displayTabButton.isPressed(point)) { currentTab = Tab::Display; return; }

        switch (currentTab) {
        case Tab::Spawn:
            if (spawnPanel.handleDropdownClick(point)) return;
            spawnPanel.handleMousePressed(point, ctx);
            break;
        case Tab::Player:
            playerPanel.handleMousePressed(point, ctx);
            break;
        case Tab::World:
            if (worldPanel.handleDropdownClick(point)) return;
            worldPanel.handleMousePressed(point, ctx);
            break;
        case Tab::Display:
            displayPanel.handleMousePressed(point, ctx);
            break;
        }
        return;
    }

    if (event.type == sf::Event::MouseMoved) {
        const sf::Vector2f point = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
        switch (currentTab) {
        case Tab::Spawn:
            spawnPanel.handleMouseMoved(point);
            break;
        case Tab::Player:
            playerPanel.handleMouseMoved(point, ctx);
            break;
        case Tab::World:
            worldPanel.handleMouseMoved(point);
            break;
        case Tab::Display:
            break;
        }
        return;
    }

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        switch (currentTab) {
        case Tab::Spawn:
            spawnPanel.handleMouseReleased();
            break;
        case Tab::Player:
            playerPanel.handleMouseReleased();
            break;
        case Tab::World:
            worldPanel.handleMouseReleased();
            break;
        case Tab::Display:
            break;
        }
        return;
    }
}

void DebugOverlay::renderTabs(sf::RenderWindow& window) const
{
    spawnTabButton.render(window);
    playerTabButton.render(window);
    worldTabButton.render(window);
    displayTabButton.render(window);
}

void DebugOverlay::render(sf::RenderWindow& window, DebugContext& ctx)
{
    window.draw(background);
    window.draw(closeButton);
    window.draw(closeLabel);
    renderTabs(window);

    switch (currentTab) {
    case Tab::Spawn:
        spawnPanel.render(window);
        break;
    case Tab::Player:
        playerPanel.render(window, ctx);
        break;
    case Tab::World:
        worldPanel.render(window);
        break;
    case Tab::Display:
        displayPanel.render(window, ctx);
        break;
    }
}

bool DebugOverlay::consumeCloseRequest()
{
    const bool wasRequested = closeRequested;
    closeRequested = false;
    return wasRequested;
}

} // namespace tc

#endif // TC_DEBUG
