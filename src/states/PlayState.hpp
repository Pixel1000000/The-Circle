#pragma once

#include <entt/entt.hpp>
#include <SFML/Graphics.hpp>

#include "ecs/ItemUpgrader.hpp"
#include "ecs/systems/AbilitySystem.hpp"
#include "ecs/systems/AISystem.hpp"
#include "ecs/systems/BlizzardSystem.hpp"
#include "ecs/systems/CollisionSystem.hpp"
#include "ecs/systems/CombatSystem.hpp"
#include "ecs/systems/LootSystem.hpp"
#include "ecs/systems/MovementSystem.hpp"
#include "ecs/systems/RenderSystem.hpp"
#include "ecs/systems/StatusEffectSystem.hpp"
#include "ecs/systems/ZoneSystem.hpp"
#include "meta/MetaProgression.hpp"
#include "states/IGameState.hpp"
#include "ui/HUD.hpp"
#include "ui/InventoryScreen.hpp"
#include "ui/ItemChoiceScreen.hpp"
#include "ui/PauseScreen.hpp"
#include "ui/SettingsScreen.hpp"
#include "ui/TutorialHint.hpp"
#include "world/World.hpp"

#ifdef TC_DEBUG
#include "debug/DebugOverlay.hpp"
#endif

namespace tc {

// The core gameplay loop: drives the ECS systems, biome progression
// (key fragments -> next biome -> boss room) and run-ending conditions.
class PlayState : public IGameState {
public:
    explicit PlayState(Game& game);

    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

#ifdef TC_DEBUG
    // Thin public wrappers so DebugWorldPanel can trigger progression
    // without widening access to the real (private) gameplay methods.
    void debugAdvanceToNextBiome() { advanceToNextBiome(); }
    void debugEnterBossRoom() { enterBossRoom(); }
    void debugRespawnObstacles() { spawnObstacles(); }
#endif

private:
    void spawnBiomeEnemies();
    void spawnObstacles();
    void clearCurrentBiomeEnemies();
    void advanceToNextBiome();
    void enterBossRoom();
    void finishRun(bool victory);

    sf::Vector2f readMovementInput() const;

    entt::registry registry;
    World world;

    entt::entity player;

    MovementSystem movementSystem;
    CollisionSystem collisionSystem;
    AISystem aiSystem;
    BlizzardSystem blizzardSystem;
    AbilitySystem abilitySystem;
    ZoneSystem zoneSystem;
    CombatSystem combatSystem;
    StatusEffectSystem statusEffectSystem;
    LootSystem lootSystem;
    RenderSystem renderSystem;

    HUD hud;
    TutorialHint tutorialHint;
    PauseScreen pauseScreen;
    SettingsScreen settingsScreen;
    InventoryScreen inventoryScreen;
    ItemChoiceScreen itemChoiceScreen;

    RunSummary runSummary;
    bool inBossRoom = false;
    bool paused = false;
    bool settingsOpen = false;
    bool inventoryOpen = false;
    bool itemChoiceOpen = false;
    float lastDt = 0.0f;

#ifdef TC_DEBUG
    DebugOverlay debugOverlay;
    bool debugOpen = false;
#endif

    // Equipment drop awaiting an ItemChoiceScreen decision (slot indices:
    // 0 = helmet, 1 = chest, 2 = leggings, 3 = weapon).
    struct PendingDrop {
        int slot = 0;
        int tier = 0;
        Element element = Element::NONE;
        float percent = 0.0f;
    } pendingDrop;
};

} // namespace tc
