#include "states/PlayState.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <string>

#include "Game.hpp"
#include "config/ConfigLoader.hpp"
#include "ecs/EntityFactory.hpp"
#include "states/MainMenuState.hpp"
#include "states/DeathState.hpp"

#ifdef TC_DEBUG
#include "debug/DebugContext.hpp"
#endif

namespace tc {

namespace {

sf::Color backgroundColorFor(BiomeType type)
{
    switch (type) {
    case BiomeType::FOREST: return sf::Color(20, 50, 25);
    case BiomeType::DESERT: return sf::Color(80, 65, 30);
    case BiomeType::WINTER: return sf::Color(40, 55, 70);
    case BiomeType::DEADLANDS: return sf::Color(45, 20, 20);
    }
    return sf::Color::Black;
}

std::string musicTrackFor(BiomeType type)
{
    switch (type) {
    case BiomeType::FOREST: return "forest";
    case BiomeType::DESERT: return "desert";
    case BiomeType::WINTER: return "winter";
    case BiomeType::DEADLANDS: return "deadlands";
    }
    return "forest";
}

} // namespace

PlayState::PlayState(Game& game)
    : IGameState(game)
{
    world.generate();

    const auto& playerConfig = ConfigLoader::get().getPlayerConfig();
    const auto& metaStats = game.getMetaProgression().getStats();

    player = EntityFactory::createPlayer(registry, {Game::LOGICAL_WIDTH / 2.0f, Game::LOGICAL_HEIGHT / 2.0f},
        playerConfig, metaStats);
    EntityFactory::applyEquipmentStats(registry, player, playerConfig,
        ConfigLoader::get().getEquipmentConfig(), metaStats);

    spawnBiomeEnemies();

    game.getAudio().playMusic(musicTrackFor(world.getCurrentBiome().getType()));

#ifdef TC_DEBUG
    debugOverlay.init(game);
#endif
}

sf::Vector2f PlayState::readMovementInput() const
{
    sf::Vector2f direction{0.0f, 0.0f};

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) direction.x -= 1.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) direction.x += 1.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) direction.y -= 1.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) direction.y += 1.0f;

    const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length > 0.0001f) {
        direction /= length;
    }

    return direction;
}

void PlayState::spawnBiomeEnemies()
{
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> xDist(64.0f, static_cast<float>(Game::LOGICAL_WIDTH) - 64.0f);
    std::uniform_real_distribution<float> yDist(64.0f, static_cast<float>(Game::LOGICAL_HEIGHT) - 64.0f);

    constexpr int ENEMIES_PER_TEMPLATE = 3;
    constexpr float MIN_SPAWN_DIST_SQ = 200.0f * 200.0f;
    constexpr int MAX_SPAWN_ATTEMPTS = 20;

    const auto& playerPosition = registry.get<Position>(player);
    const sf::Vector2f playerPos{playerPosition.x, playerPosition.y};

    auto& biome = world.getCurrentBiome();
    const auto templates = ConfigLoader::get().getEnemyConfig().getEnemiesForBiome(biome.getType());

    for (const auto& tmpl : templates) {
        for (int i = 0; i < ENEMIES_PER_TEMPLATE; ++i) {
            sf::Vector2f position;
            int attempts = 0;
            do {
                position = {xDist(rng), yDist(rng)};
                const float dx = position.x - playerPos.x;
                const float dy = position.y - playerPos.y;
                if (dx * dx + dy * dy >= MIN_SPAWN_DIST_SQ) {
                    break;
                }
            } while (++attempts < MAX_SPAWN_ATTEMPTS);

            const entt::entity entity = EntityFactory::createEnemy(registry, tmpl, position);
            biome.getEnemies().push_back(entity);
        }
    }
}

void PlayState::clearCurrentBiomeEnemies()
{
    auto& enemies = world.getCurrentBiome().getEnemies();
    for (auto entity : enemies) {
        if (registry.valid(entity)) {
            registry.destroy(entity);
        }
    }
    enemies.clear();
}

void PlayState::advanceToNextBiome()
{
    clearCurrentBiomeEnemies();
    ++runSummary.biomesCleared;

    if (world.getCurrentBiome().getType() == BiomeType::DEADLANDS) {
        enterBossRoom();
        return;
    }

    const auto nextBiome = static_cast<BiomeType>(static_cast<int>(world.getCurrentBiome().getType()) + 1);
    world.setCurrentBiome(nextBiome);
    spawnBiomeEnemies();

    game.getAudio().playMusic(musicTrackFor(world.getCurrentBiome().getType()));
}

void PlayState::enterBossRoom()
{
    inBossRoom = true;

    const auto* bossTemplate = ConfigLoader::get().getEnemyConfig().getBossForTheme(world.getBossRoomTheme());
    if (!bossTemplate) {
        finishRun(true);
        return;
    }

    EntityFactory::createBoss(registry, *bossTemplate,
        {Game::LOGICAL_WIDTH / 2.0f, Game::LOGICAL_HEIGHT / 2.0f});

    game.getAudio().playMusic("boss");
}

void PlayState::finishRun(bool victory)
{
    runSummary.bossDefeated = victory;

    auto& meta = game.getMetaProgression();
    const int pointsEarned = meta.computePointsEarned(runSummary);
    meta.addPoints(pointsEarned);
    meta.save();

    game.changeState(std::make_unique<DeathState>(game, runSummary, victory, pointsEarned));
}

void PlayState::handleInput(const sf::Event& event)
{
#ifdef TC_DEBUG
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F1) {
        debugOpen = !debugOpen;
        if (debugOpen) {
            DebugContext ctx{registry, player, world, game, *this, renderSystem, lastDt};
            debugOverlay.onOpen(ctx);
        }
        return;
    }

    if (debugOpen) {
        DebugContext ctx{registry, player, world, game, *this, renderSystem, lastDt};
        debugOverlay.handleInput(event, game.getWindow(), ctx);
        if (debugOverlay.consumeCloseRequest()) {
            debugOpen = false;
        }
        return;
    }
#endif

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        const sf::Vector2f point = game.getWindow().mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});

        if (inventoryOpen) {
            switch (inventoryScreen.getButtonAt(point)) {
            case InventoryScreen::Button::HELP:
                inventoryScreen.toggleHelp();
                break;
            default:
                break;
            }
            return;
        }

        if (itemChoiceOpen) {
            auto& equipment = registry.get<Equipment>(player);
            switch (itemChoiceScreen.getButtonAt(point)) {
            case ItemChoiceScreen::Button::TAKE_NEW:
                ItemUpgrader::takeNew(equipment, pendingDrop.slot, pendingDrop.tier, pendingDrop.element, pendingDrop.percent);
                if (itemChoiceScreen.isDontAskAgain()) {
                    ItemUpgrader::setAutoUpgrade(equipment, pendingDrop.slot, false);
                }
                EntityFactory::applyEquipmentStats(registry, player, ConfigLoader::get().getPlayerConfig(),
                    ConfigLoader::get().getEquipmentConfig(), game.getMetaProgression().getStats());
                hud.showEquipmentDrop(pendingDrop.slot, ItemUpgrader::getTier(equipment, pendingDrop.slot));
                itemChoiceOpen = false;
                break;
            case ItemChoiceScreen::Button::UPGRADE_CURRENT:
                ItemUpgrader::upgradeCurrent(equipment, pendingDrop.slot, pendingDrop.tier, pendingDrop.element, pendingDrop.percent);
                if (itemChoiceScreen.isDontAskAgain()) {
                    ItemUpgrader::setAutoUpgrade(equipment, pendingDrop.slot, true);
                }
                EntityFactory::applyEquipmentStats(registry, player, ConfigLoader::get().getPlayerConfig(),
                    ConfigLoader::get().getEquipmentConfig(), game.getMetaProgression().getStats());
                hud.showEquipmentDrop(pendingDrop.slot, ItemUpgrader::getTier(equipment, pendingDrop.slot));
                itemChoiceOpen = false;
                break;
            case ItemChoiceScreen::Button::CHECKBOX:
                itemChoiceScreen.toggleDontAskAgain();
                break;
            default:
                break;
            }
            return;
        }

        if (paused) {
            switch (pauseScreen.getButtonAt(point)) {
            case PauseScreen::Button::RESUME:
                paused = false;
                break;
            case PauseScreen::Button::MAIN_MENU:
                game.changeState(std::make_unique<MainMenuState>(game));
                break;
            default:
                break;
            }
            return;
        }

        tutorialHint.handleClick(point);
        return;
    }

    if (event.type != sf::Event::KeyPressed) {
        return;
    }

    if (event.key.code == sf::Keyboard::Tab) {
        if (!paused && !itemChoiceOpen) {
            inventoryOpen = !inventoryOpen;
            if (!inventoryOpen) inventoryScreen.resetHelp();
        }
        return;
    }

    if (event.key.code == sf::Keyboard::Escape) {
        if (inventoryOpen) {
            inventoryOpen = false;
            inventoryScreen.resetHelp();
            return;
        }
        if (itemChoiceOpen) {
            return;
        }
        paused = !paused;
        return;
    }

    if (paused || inventoryOpen) {
        return;
    }

    if (event.key.code == sf::Keyboard::E || event.key.code == sf::Keyboard::Space) {
        combatSystem.usePotion(registry, player);
    } else if (event.key.code == sf::Keyboard::F) {
        if (!inBossRoom && world.getCurrentBiome().isUnlocked()) {
            advanceToNextBiome();
        }
    }
}

void PlayState::update(float dt)
{
#ifdef TC_DEBUG
    if (debugOpen) {
        lastDt = 0.0f;
        return;
    }
#endif

    if (paused || inventoryOpen || itemChoiceOpen) {
        lastDt = 0.0f;
        return;
    }

    lastDt = dt;

    const sf::Vector2f moveDir = readMovementInput();
    auto& velocity = registry.get<Velocity>(player);
    velocity.dx = moveDir.x;
    velocity.dy = moveDir.y;

    if (moveDir.x != 0.0f || moveDir.y != 0.0f) {
        auto& facing = registry.get<Facing>(player);
        facing.dx = moveDir.x;
        facing.dy = moveDir.y;
    }

    movementSystem.update(registry, dt);
    collisionSystem.update(registry);

    auto& playerPos = registry.get<Position>(player);
    const auto& playerSize = registry.get<Renderable>(player).size;
    playerPos.x = std::clamp(playerPos.x, playerSize.x / 2.0f, static_cast<float>(Game::LOGICAL_WIDTH) - playerSize.x / 2.0f);
    playerPos.y = std::clamp(playerPos.y, playerSize.y / 2.0f, static_cast<float>(Game::LOGICAL_HEIGHT) - playerSize.y / 2.0f);

    for (auto entity : registry.view<EnemyTag, Position, Renderable>()) {
        auto& enemyPos = registry.get<Position>(entity);
        const auto& enemySize = registry.get<Renderable>(entity).size;
        enemyPos.x = std::clamp(enemyPos.x, enemySize.x / 2.0f, static_cast<float>(Game::LOGICAL_WIDTH) - enemySize.x / 2.0f);
        enemyPos.y = std::clamp(enemyPos.y, enemySize.y / 2.0f, static_cast<float>(Game::LOGICAL_HEIGHT) - enemySize.y / 2.0f);
    }

    aiSystem.update(registry, dt);
    combatSystem.update(registry, dt);
    statusEffectSystem.update(registry, dt);

    const LootResult lootResult = lootSystem.update(registry, player, world.getCurrentBiome().getEnemies());
    runSummary.kills += lootResult.kills;
    if (lootResult.fragmentsCollected > 0) {
        for (int i = 0; i < lootResult.fragmentsCollected; ++i) {
            world.getCurrentBiome().addKeyFragment();
        }
        runSummary.keyFragments += lootResult.fragmentsCollected;
    }
    if (lootResult.equipmentDropped) {
        auto& equipment = registry.get<Equipment>(player);
        const int slot = lootResult.droppedSlot;

        if (ItemUpgrader::getTier(equipment, slot) == 0) {
            ItemUpgrader::takeNew(equipment, slot, lootResult.droppedTier, lootResult.droppedElement, lootResult.droppedPercent);
            EntityFactory::applyEquipmentStats(registry, player, ConfigLoader::get().getPlayerConfig(),
                ConfigLoader::get().getEquipmentConfig(), game.getMetaProgression().getStats());
            hud.showEquipmentDrop(slot, lootResult.droppedTier);
        } else if (ItemUpgrader::getAutoUpgrade(equipment, slot)) {
            ItemUpgrader::upgradeCurrent(equipment, slot, lootResult.droppedTier, lootResult.droppedElement, lootResult.droppedPercent);
            EntityFactory::applyEquipmentStats(registry, player, ConfigLoader::get().getPlayerConfig(),
                ConfigLoader::get().getEquipmentConfig(), game.getMetaProgression().getStats());
            hud.showEquipmentDrop(slot, ItemUpgrader::getTier(equipment, slot));
        } else {
            pendingDrop = {slot, lootResult.droppedTier, lootResult.droppedElement, lootResult.droppedPercent};
            itemChoiceScreen.open();
            itemChoiceOpen = true;
        }
    }

    if (!inBossRoom) {
        auto& enemies = world.getCurrentBiome().getEnemies();
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
            [this](entt::entity e) { return !registry.valid(e); }), enemies.end());
        if (enemies.empty()) {
            spawnBiomeEnemies();
        }
    }

    const auto& playerHealth = registry.get<Health>(player);
    if (playerHealth.current <= 0) {
        finishRun(false);
        return;
    }

    if (inBossRoom) {
        bool bossAlive = false;
        for (auto entity : registry.view<BossTag, Health>()) {
            if (registry.get<Health>(entity).current > 0) {
                bossAlive = true;
                break;
            }
        }
        if (!bossAlive) {
            finishRun(true);
            return;
        }
    }
}

void PlayState::render(sf::RenderWindow& window)
{
    const BiomeType displayedBiome = inBossRoom ? world.getBossRoomTheme() : world.getCurrentBiome().getType();
    window.clear(backgroundColorFor(displayedBiome));

    const bool showNextBiomeHint = !inBossRoom && world.getCurrentBiome().isUnlocked();

    renderSystem.update(registry, window, game.getLocalization(), game.getFontManager(), lastDt);

    hud.render(window, registry, player, game.getLocalization(), game.getFontManager(), Biome::KEY_FRAGMENTS_REQUIRED,
        showNextBiomeHint, lastDt);
    tutorialHint.render(window, game.getLocalization(), game.getFontManager());

    if (paused) {
        pauseScreen.render(window, game.getLocalization(), game.getFontManager());
    }

    if (inventoryOpen) {
        inventoryScreen.render(window, game.getLocalization(), game.getFontManager(), registry, player);
    }

    if (itemChoiceOpen) {
        const auto& equipment = registry.get<Equipment>(player);
        const auto [currentElement, currentPercent] = ItemUpgrader::getElement(equipment, pendingDrop.slot);
        itemChoiceScreen.render(window, game.getLocalization(), game.getFontManager(),
            pendingDrop.slot, pendingDrop.tier, pendingDrop.element, pendingDrop.percent,
            ItemUpgrader::getTier(equipment, pendingDrop.slot), currentElement, currentPercent);
    }

#ifdef TC_DEBUG
    if (debugOpen) {
        DebugContext ctx{registry, player, world, game, *this, renderSystem, lastDt};
        debugOverlay.render(window, ctx);
    }
#endif
}

} // namespace tc
