#include "debug/DebugDisplayPanel.hpp"

#ifdef TC_DEBUG

#include <string>

#include "ecs/Components.hpp"
#include "ecs/systems/RenderSystem.hpp"
#include "world/Biome.hpp"
#include "world/World.hpp"

namespace tc {

namespace {
std::string biomeName(BiomeType type)
{
    switch (type) {
    case BiomeType::FOREST: return "Forest";
    case BiomeType::DESERT: return "Desert";
    case BiomeType::WINTER: return "Winter";
    case BiomeType::DEADLANDS: return "Deadlands";
    }
    return "Unknown";
}
} // namespace

void DebugDisplayPanel::init(const sf::Font& font, sf::Vector2f origin)
{
    hitboxesCheckbox.setup(font, "Show Hitboxes", origin + sf::Vector2f(0.0f, 0.0f));
    hpNumbersCheckbox.setup(font, "Show HP Numbers", origin + sf::Vector2f(0.0f, 36.0f));
    velocityVectorsCheckbox.setup(font, "Show Velocity Vectors", origin + sf::Vector2f(0.0f, 72.0f));
    blizzardZonesCheckbox.setup(font, "Show Blizzard Zones", origin + sf::Vector2f(0.0f, 108.0f));

    infoText.setFont(font);
    infoText.setCharacterSize(15);
    infoText.setFillColor(sf::Color(200, 220, 255));
    infoText.setPosition(origin.x, origin.y + 160.0f);
}

void DebugDisplayPanel::onOpen(DebugContext& ctx)
{
    hitboxesCheckbox.checked = ctx.renderSystem.showHitboxes;
    hpNumbersCheckbox.checked = ctx.renderSystem.showHpNumbers;
    velocityVectorsCheckbox.checked = ctx.renderSystem.showVelocityVectors;
    blizzardZonesCheckbox.checked = ctx.renderSystem.showBlizzardZones;
}

void DebugDisplayPanel::handleMousePressed(sf::Vector2f point, DebugContext& ctx)
{
    if (hitboxesCheckbox.isClicked(point)) {
        hitboxesCheckbox.toggle();
        ctx.renderSystem.showHitboxes = hitboxesCheckbox.checked;
    } else if (hpNumbersCheckbox.isClicked(point)) {
        hpNumbersCheckbox.toggle();
        ctx.renderSystem.showHpNumbers = hpNumbersCheckbox.checked;
    } else if (velocityVectorsCheckbox.isClicked(point)) {
        velocityVectorsCheckbox.toggle();
        ctx.renderSystem.showVelocityVectors = velocityVectorsCheckbox.checked;
    } else if (blizzardZonesCheckbox.isClicked(point)) {
        blizzardZonesCheckbox.toggle();
        ctx.renderSystem.showBlizzardZones = blizzardZonesCheckbox.checked;
    }
}

void DebugDisplayPanel::render(sf::RenderWindow& window, DebugContext& ctx)
{
    hitboxesCheckbox.render(window);
    hpNumbersCheckbox.render(window);
    velocityVectorsCheckbox.render(window);
    blizzardZonesCheckbox.render(window);

    int enemyCount = 0;
    for (auto entity : ctx.registry.view<EnemyTag>()) {
        (void)entity;
        ++enemyCount;
    }

    const auto& playerPos = ctx.registry.get<Position>(ctx.player);
    const float fps = ctx.lastDt > 0.0001f ? 1.0f / ctx.lastDt : 0.0f;
    const std::size_t waveBank = ctx.world.getCurrentBiome().getEnemies().size();

    std::string info;
    info += "FPS: " + std::to_string(static_cast<int>(fps)) + "\n";
    info += "Entity Count: " + std::to_string(ctx.registry.storage<entt::entity>().size()) + "\n";
    info += "Enemy Count: " + std::to_string(enemyCount) + "\n";
    info += "Player Position: " + std::to_string(static_cast<int>(playerPos.x)) + ", "
        + std::to_string(static_cast<int>(playerPos.y)) + "\n";
    info += "Current Biome: " + biomeName(ctx.world.getCurrentBiome().getType()) + "\n";
    info += "Wave Bank: " + std::to_string(waveBank);

    infoText.setString(info);
    window.draw(infoText);
}

} // namespace tc

#endif // TC_DEBUG
