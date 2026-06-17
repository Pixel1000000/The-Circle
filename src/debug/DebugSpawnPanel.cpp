#include "debug/DebugSpawnPanel.hpp"

#ifdef TC_DEBUG

#include <cmath>
#include <random>
#include <vector>

#include "Game.hpp"
#include "config/ConfigLoader.hpp"
#include "ecs/Components.hpp"
#include "ecs/EntityFactory.hpp"
#include "world/World.hpp"

namespace tc {

namespace {
constexpr float MIN_SPAWN_DIST = 150.0f;
constexpr int MAX_SPAWN_ATTEMPTS = 20;

sf::Vector2f randomSpawnPosition(sf::Vector2f playerPos, std::mt19937& rng)
{
    std::uniform_real_distribution<float> xDist(64.0f, static_cast<float>(Game::LOGICAL_WIDTH) - 64.0f);
    std::uniform_real_distribution<float> yDist(64.0f, static_cast<float>(Game::LOGICAL_HEIGHT) - 64.0f);

    sf::Vector2f position;
    int attempts = 0;
    do {
        position = {xDist(rng), yDist(rng)};
        const float dx = position.x - playerPos.x;
        const float dy = position.y - playerPos.y;
        if (std::sqrt(dx * dx + dy * dy) >= MIN_SPAWN_DIST) {
            break;
        }
    } while (++attempts < MAX_SPAWN_ATTEMPTS);
    return position;
}
} // namespace

void DebugSpawnPanel::init(const sf::Font& font, sf::Vector2f origin)
{
    constexpr float COL_X = 0.0f;
    constexpr float WIDGET_W = 260.0f;

    enemyDropdown.setup(font, origin + sf::Vector2f(COL_X, 30.0f), {WIDGET_W, 26.0f});
    spawnEnemyButton.setup(font, "Spawn Enemy", origin + sf::Vector2f(COL_X, 64.0f), {160.0f, 30.0f});
    countSlider.setup(font, "Count", origin + sf::Vector2f(COL_X, 124.0f), {WIDGET_W, 16.0f}, 1.0f, 10.0f, 1.0f, true);
    clearEnemiesButton.setup(font, "Clear All Enemies", origin + sf::Vector2f(COL_X, 168.0f), {200.0f, 30.0f},
        sf::Color(110, 60, 60));

    bossDropdown.setup(font, origin + sf::Vector2f(COL_X, 230.0f), {WIDGET_W, 26.0f});
    spawnBossButton.setup(font, "Spawn Boss", origin + sf::Vector2f(COL_X, 264.0f), {160.0f, 30.0f},
        sf::Color(110, 70, 110));

    const auto& enemyConfig = ConfigLoader::get().getEnemyConfig();

    std::vector<std::string> enemyIds;
    for (const auto& tmpl : enemyConfig.enemies) {
        enemyIds.push_back(tmpl.id);
    }
    enemyDropdown.setItems(enemyIds);

    std::vector<std::string> bossIds;
    for (const auto& tmpl : enemyConfig.bosses) {
        bossIds.push_back(tmpl.id);
    }
    bossDropdown.setItems(bossIds);
}

void DebugSpawnPanel::onOpen(DebugContext&)
{
    countSlider.setValue(1.0f);
}

void DebugSpawnPanel::handleMousePressed(sf::Vector2f point, DebugContext& ctx)
{
    countSlider.handleMousePressed(point);

    if (spawnEnemyButton.isPressed(point)) {
        const auto& enemyConfig = ConfigLoader::get().getEnemyConfig();
        const std::string& selectedId = enemyDropdown.getSelected();

        const EnemyTemplate* selectedTemplate = nullptr;
        for (const auto& tmpl : enemyConfig.enemies) {
            if (tmpl.id == selectedId) {
                selectedTemplate = &tmpl;
                break;
            }
        }
        if (!selectedTemplate) return;

        static std::mt19937 rng{std::random_device{}()};
        const auto& playerPosition = ctx.registry.get<Position>(ctx.player);
        const sf::Vector2f playerPos{playerPosition.x, playerPosition.y};

        const int count = static_cast<int>(std::round(countSlider.getValue()));
        auto& enemies = ctx.world.getCurrentBiome().getEnemies();
        for (int i = 0; i < count; ++i) {
            const sf::Vector2f position = randomSpawnPosition(playerPos, rng);
            const entt::entity entity = EntityFactory::createEnemy(ctx.registry, *selectedTemplate, position);
            enemies.push_back(entity);
        }
        return;
    }

    if (clearEnemiesButton.isPressed(point)) {
        std::vector<entt::entity> toDestroy;
        for (auto entity : ctx.registry.view<EnemyTag>()) toDestroy.push_back(entity);
        for (auto entity : ctx.registry.view<BossTag>()) toDestroy.push_back(entity);
        for (auto entity : toDestroy) {
            if (ctx.registry.valid(entity)) ctx.registry.destroy(entity);
        }
        ctx.world.getCurrentBiome().getEnemies().clear();
        return;
    }

    if (spawnBossButton.isPressed(point)) {
        const auto& enemyConfig = ConfigLoader::get().getEnemyConfig();
        const std::string& selectedId = bossDropdown.getSelected();

        const BossTemplate* selectedTemplate = nullptr;
        for (const auto& tmpl : enemyConfig.bosses) {
            if (tmpl.id == selectedId) {
                selectedTemplate = &tmpl;
                break;
            }
        }
        if (!selectedTemplate) return;

        EntityFactory::createBoss(ctx.registry, *selectedTemplate,
            {Game::LOGICAL_WIDTH / 2.0f, Game::LOGICAL_HEIGHT / 2.0f});
        return;
    }
}

void DebugSpawnPanel::handleMouseMoved(sf::Vector2f point)
{
    countSlider.handleMouseMoved(point);
}

void DebugSpawnPanel::handleMouseReleased()
{
    countSlider.handleMouseReleased();
}

bool DebugSpawnPanel::handleDropdownClick(sf::Vector2f point)
{
    if (enemyDropdown.handleClick(point)) return true;
    if (bossDropdown.handleClick(point)) return true;
    return false;
}

bool DebugSpawnPanel::handleScroll(sf::Vector2f point, float delta)
{
    if (enemyDropdown.handleScroll(point, delta)) return true;
    if (bossDropdown.handleScroll(point, delta)) return true;
    return false;
}

void DebugSpawnPanel::render(sf::RenderWindow& window) const
{
    enemyDropdown.renderHeader(window);
    spawnEnemyButton.render(window);
    countSlider.render(window);
    clearEnemiesButton.render(window);

    bossDropdown.renderHeader(window);
    spawnBossButton.render(window);
}

void DebugSpawnPanel::renderDropdownOverlays(sf::RenderWindow& window) const
{
    enemyDropdown.renderExpandedList(window);
    bossDropdown.renderExpandedList(window);
}

} // namespace tc

#endif // TC_DEBUG
