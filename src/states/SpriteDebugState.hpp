#pragma once

#ifdef TC_DEBUG

#include <vector>

#include <entt/entt.hpp>

#include "ecs/Components.hpp"
#include "sprite/AnimationSystem.hpp"
#include "sprite/RenderSystem.hpp"
#include "sprite/SpriteTypes.hpp"
#include "states/IGameState.hpp"

namespace tc {

// Dev-only sprite/animation viewer (debug-mode entry point from the main
// menu). Lets you cycle currentAnimation and Facing with the keyboard and
// see the dead_swordsman body react live - the manual verification step for
// SpriteSheetLoader/AnimationSystem/RenderSystem called out in the sprite
// foundation's Definition of Done.
class SpriteDebugState : public IGameState {
public:
    explicit SpriteDebugState(Game& game);

    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    entt::registry registry;
    entt::entity previewEntity;

    sprite::AnimationSystem animationSystem;
    sprite::RenderSystem renderSystem;

    std::vector<std::string> animationNames = {"idle", "walk"};
    std::size_t animationIndex = 0;

    sf::Text instructionsText;
    sf::Text statusText;
    bool fontLoaded = false;
};

} // namespace tc

#endif // TC_DEBUG
