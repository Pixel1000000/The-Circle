#include "states/SpriteDebugState.hpp"

#ifdef TC_DEBUG

#include <string>

#include "Game.hpp"
#include "sprite/SpriteSheetLoader.hpp"

namespace tc {

namespace {
const char* facingName(sprite::Facing f)
{
    switch (f) {
        case sprite::Facing::South: return "south";
        case sprite::Facing::SouthEast: return "south-east";
        case sprite::Facing::East: return "east";
        case sprite::Facing::NorthEast: return "north-east";
        case sprite::Facing::North: return "north";
        case sprite::Facing::NorthWest: return "north-west";
        case sprite::Facing::West: return "west";
        case sprite::Facing::SouthWest: return "south-west";
    }
    return "?";
}
} // namespace

SpriteDebugState::SpriteDebugState(Game& game)
    : IGameState(game)
{
    fontLoaded = game.getFontManager().isLoaded();

    auto bodySheet = sprite::SpriteSheetLoader::load(
        "assets/characters/dead_swordsman/dead_swordsman_Idle.png",
        "assets/characters/dead_swordsman/dead_swordsman_Idle.json");

    previewEntity = registry.create();
    registry.emplace<Position>(previewEntity, Position{Game::LOGICAL_WIDTH / 2.0f, Game::LOGICAL_HEIGHT / 2.0f});
    registry.emplace<sprite::BaseAppearance>(previewEntity, sprite::BaseAppearance{bodySheet});
    registry.emplace<sprite::AnimationState>(previewEntity);

    if (fontLoaded) {
        const sf::Font& font = game.getFontManager().getFont("en");
        instructionsText.setFont(font);
        instructionsText.setCharacterSize(18);
        instructionsText.setFillColor(sf::Color::White);
        instructionsText.setPosition(20.0f, 20.0f);
        instructionsText.setString(
            "Sprite Debug\n"
            "1/2: switch animation (idle/walk)\n"
            "A/D or Left/Right: rotate facing\n"
            "Esc: back to menu");

        statusText.setFont(font);
        statusText.setCharacterSize(20);
        statusText.setFillColor(sf::Color(200, 220, 255));
        statusText.setPosition(20.0f, Game::LOGICAL_HEIGHT - 40.0f);
    }
}

void SpriteDebugState::handleInput(const sf::Event& event)
{
    if (event.type != sf::Event::KeyPressed) {
        return;
    }

    auto& state = registry.get<sprite::AnimationState>(previewEntity);

    if (event.key.code == sf::Keyboard::Escape) {
        game.popState();
        return;
    }

    if (event.key.code == sf::Keyboard::Num1) {
        animationIndex = 0;
        state.currentAnimation = animationNames[animationIndex];
        state.currentFrame = 0;
        state.finished = false;
    } else if (event.key.code == sf::Keyboard::Num2) {
        animationIndex = 1;
        state.currentAnimation = animationNames[animationIndex];
        state.currentFrame = 0;
        state.finished = false;
    } else if (event.key.code == sf::Keyboard::D || event.key.code == sf::Keyboard::Right) {
        const int next = (static_cast<int>(state.facing) + 1) % 8;
        state.facing = static_cast<sprite::Facing>(next);
    } else if (event.key.code == sf::Keyboard::A || event.key.code == sf::Keyboard::Left) {
        const int prev = (static_cast<int>(state.facing) + 7) % 8;
        state.facing = static_cast<sprite::Facing>(prev);
    }
}

void SpriteDebugState::update(float dt)
{
    animationSystem.update(registry, dt);

    if (fontLoaded) {
        const auto& state = registry.get<sprite::AnimationState>(previewEntity);
        statusText.setString("animation: " + state.currentAnimation + "   facing: " + facingName(state.facing)
            + "   frame: " + std::to_string(state.currentFrame));
    }
}

void SpriteDebugState::render(sf::RenderWindow& window)
{
    window.clear(sf::Color(18, 20, 28));

    renderSystem.render(window, registry);

    if (fontLoaded) {
        window.draw(instructionsText);
        window.draw(statusText);
    }
}

} // namespace tc

#endif // TC_DEBUG
