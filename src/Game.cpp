#include "Game.hpp"

#include "config/ConfigLoader.hpp"
#include "states/MainMenuState.hpp"

namespace tc {

Game::Game()
    : window(sf::VideoMode(LOGICAL_WIDTH, LOGICAL_HEIGHT), "The Circle")
    , view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(LOGICAL_WIDTH), static_cast<float>(LOGICAL_HEIGHT)))
{
    window.setFramerateLimit(60);
    window.setView(view);

    ConfigLoader::get().loadAll();
    localization.load("en");
    metaProgression.load();
    fontManager.load();

    states.push(std::make_unique<MainMenuState>(*this));

    applyLetterboxView();
}

void Game::run()
{
    sf::Clock clock;

    while (running && window.isOpen()) {
        applyPendingStateChange();

        const float dt = clock.restart().asSeconds();

        processInput();
        update(dt);
        render();
    }
}

void Game::quit()
{
    running = false;
    window.close();
}

void Game::pushState(std::unique_ptr<IGameState> state)
{
    pendingAction = PendingAction::Push;
    pendingState = std::move(state);
}

void Game::popState()
{
    pendingAction = PendingAction::Pop;
    pendingState.reset();
}

void Game::changeState(std::unique_ptr<IGameState> state)
{
    pendingAction = PendingAction::Change;
    pendingState = std::move(state);
}

sf::RenderWindow& Game::getWindow()
{
    return window;
}

Localization& Game::getLocalization()
{
    return localization;
}

AudioManager& Game::getAudio()
{
    return audio;
}

MetaProgression& Game::getMetaProgression()
{
    return metaProgression;
}

FontManager& Game::getFontManager()
{
    return fontManager;
}

void Game::applyPendingStateChange()
{
    switch (pendingAction) {
    case PendingAction::Push:
        states.push(std::move(pendingState));
        break;
    case PendingAction::Pop:
        if (!states.empty()) {
            states.pop();
        }
        break;
    case PendingAction::Change:
        if (!states.empty()) {
            states.pop();
        }
        states.push(std::move(pendingState));
        break;
    case PendingAction::None:
    default:
        break;
    }

    pendingAction = PendingAction::None;
    pendingState.reset();
}

void Game::processInput()
{
    sf::Event event{};
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            quit();
            continue;
        }

        if (event.type == sf::Event::Resized) {
            applyLetterboxView();
        }

        if (!states.empty()) {
            states.top()->handleInput(event);
        }
    }
}

void Game::update(float dt)
{
    if (!states.empty()) {
        states.top()->update(dt);
    }
}

void Game::render()
{
    if (!states.empty()) {
        states.top()->render(window);
    }
    window.display();
}

void Game::applyLetterboxView()
{
    const sf::Vector2u windowSize = window.getSize();
    if (windowSize.x == 0 || windowSize.y == 0) {
        return;
    }

    const float windowRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
    const float viewRatio = static_cast<float>(LOGICAL_WIDTH) / static_cast<float>(LOGICAL_HEIGHT);

    float sizeX = 1.0f;
    float sizeY = 1.0f;
    float posX = 0.0f;
    float posY = 0.0f;

    if (windowRatio > viewRatio) {
        sizeX = viewRatio / windowRatio;
        posX = (1.0f - sizeX) / 2.0f;
    } else {
        sizeY = windowRatio / viewRatio;
        posY = (1.0f - sizeY) / 2.0f;
    }

    view.setViewport(sf::FloatRect(posX, posY, sizeX, sizeY));
    window.setView(view);
}

} // namespace tc
