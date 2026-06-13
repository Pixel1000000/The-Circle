#pragma once

#include <entt/entt.hpp>
#include <SFML/Graphics.hpp>

#include "ecs/systems/AISystem.hpp"
#include "ecs/systems/CombatSystem.hpp"
#include "ecs/systems/LootSystem.hpp"
#include "ecs/systems/MovementSystem.hpp"
#include "ecs/systems/RenderSystem.hpp"
#include "ecs/systems/StatusEffectSystem.hpp"
#include "meta/MetaProgression.hpp"
#include "states/IGameState.hpp"
#include "ui/HUD.hpp"
#include "ui/TutorialHint.hpp"
#include "world/World.hpp"

namespace tc {

// The core gameplay loop: drives the ECS systems, biome progression
// (key fragments -> next biome -> boss room) and run-ending conditions.
class PlayState : public IGameState {
public:
    explicit PlayState(Game& game);

    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    void spawnBiomeEnemies();
    void clearCurrentBiomeEnemies();
    void advanceToNextBiome();
    void enterBossRoom();
    void finishRun(bool victory);

    sf::Vector2f readMovementInput() const;

    entt::registry registry;
    World world;

    entt::entity player;

    MovementSystem movementSystem;
    AISystem aiSystem;
    CombatSystem combatSystem;
    StatusEffectSystem statusEffectSystem;
    LootSystem lootSystem;
    RenderSystem renderSystem;

    HUD hud;
    TutorialHint tutorialHint;

    RunSummary runSummary;
    bool inBossRoom = false;
    float lastDt = 0.0f;
};

} // namespace tc
