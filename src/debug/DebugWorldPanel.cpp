#include "debug/DebugWorldPanel.hpp"

#ifdef TC_DEBUG

#include <cmath>
#include <random>

#include "config/ConfigLoader.hpp"
#include "ecs/EntityFactory.hpp"
#include "ecs/ItemUpgrader.hpp"
#include "Game.hpp"
#include "states/PlayState.hpp"
#include "world/Biome.hpp"
#include "world/World.hpp"

namespace tc {

namespace {
constexpr float WIDGET_W = 220.0f;

void applyMetaStats(DebugContext& ctx, const MetaStats& metaStats)
{
    EntityFactory::applyEquipmentStats(ctx.registry, ctx.player, ConfigLoader::get().getPlayerConfig(),
        ConfigLoader::get().getEquipmentConfig(), metaStats);
}

// Spawns one drifting large blizzard zone plus the usual batch of small
// static ones, mirroring AISystem's BOSS_ELEMENTAL phase so the mechanic can
// be tested without waiting for the elemental boss to trigger it.
void spawnBlizzardZones(entt::registry& registry)
{
    for (auto zone : registry.view<BlizzardZoneTag>()) {
        registry.destroy(zone);
    }

    const auto& bcfg = ConfigLoader::get().getElementalConfig().blizzard;
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> xd(80.0f, static_cast<float>(Game::LOGICAL_WIDTH) - 80.0f);
    std::uniform_real_distribution<float> yd(80.0f, static_cast<float>(Game::LOGICAL_HEIGHT) - 80.0f);
    std::uniform_int_distribution<int> smallCount(bcfg.smallCountMin, bcfg.smallCountMax);

    {
        auto e = registry.create();
        registry.emplace<Position>(e, xd(rng), yd(rng));
        registry.emplace<Renderable>(e, sf::Color(80, 180, 255, 70), bcfg.bigSize);
        registry.emplace<BlizzardZoneTag>(e);
        registry.emplace<BlizzardZone>(e, true, sf::Vector2f{0.f, 0.f}, 0.f);
    }

    const int sc = smallCount(rng);
    for (int i = 0; i < sc; ++i) {
        auto e = registry.create();
        registry.emplace<Position>(e, xd(rng), yd(rng));
        registry.emplace<Renderable>(e, sf::Color(80, 180, 255, 70), bcfg.smallSize);
        registry.emplace<BlizzardZoneTag>(e);
        registry.emplace<BlizzardZone>(e, false, sf::Vector2f{0.f, 0.f}, 0.f);
    }
}
} // namespace

void DebugWorldPanel::init(const sf::Font& font, sf::Vector2f origin)
{
    addFragmentButton.setup(font, "+1 Fragment", origin + sf::Vector2f(0.0f, 0.0f), {120.0f, 28.0f});
    maxFragmentsButton.setup(font, "Max Fragments", origin + sf::Vector2f(140.0f, 0.0f), {140.0f, 28.0f});

    slotDropdown.setup(font, origin + sf::Vector2f(0.0f, 50.0f), {WIDGET_W, 24.0f});
    slotDropdown.setItems({"Helmet", "Chest", "Leggings", "Weapon"});

    tierDropdown.setup(font, origin + sf::Vector2f(0.0f, 84.0f), {WIDGET_W, 24.0f});
    tierDropdown.setItems({"1", "2", "3", "4"});

    elementDropdown.setup(font, origin + sf::Vector2f(0.0f, 118.0f), {WIDGET_W, 24.0f});
    elementDropdown.setItems({"None", "Nature", "Fire", "Ice", "Decay"});

    effectPercentSlider.setup(font, "Effect %", origin + sf::Vector2f(0.0f, 168.0f), {WIDGET_W, 16.0f},
        5.0f, 50.0f, 5.0f, true);

    giveEquipmentButton.setup(font, "Give Equipment", origin + sf::Vector2f(0.0f, 200.0f), {180.0f, 28.0f});

    strengthSlider.setup(font, "Strength", origin + sf::Vector2f(0.0f, 250.0f), {WIDGET_W, 16.0f}, 0.0f, 20.0f, 0.0f, true);
    enduranceSlider.setup(font, "Endurance", origin + sf::Vector2f(0.0f, 300.0f), {WIDGET_W, 16.0f}, 0.0f, 20.0f, 0.0f, true);
    healthSlider.setup(font, "Health", origin + sf::Vector2f(0.0f, 350.0f), {WIDGET_W, 16.0f}, 0.0f, 20.0f, 0.0f, true);
    applyMetaButton.setup(font, "Apply Meta", origin + sf::Vector2f(0.0f, 380.0f), {140.0f, 28.0f});

    nextBiomeButton.setup(font, "Next Biome", origin + sf::Vector2f(280.0f, 0.0f), {140.0f, 28.0f},
        sf::Color(70, 110, 90));
    enterBossRoomButton.setup(font, "Enter Boss Room", origin + sf::Vector2f(280.0f, 40.0f), {160.0f, 28.0f},
        sf::Color(110, 70, 70));
    respawnObstaclesButton.setup(font, "Respawn Obstacles", origin + sf::Vector2f(280.0f, 80.0f), {180.0f, 28.0f},
        sf::Color(90, 90, 70));
    spawnBlizzardButton.setup(font, "Spawn Blizzard", origin + sf::Vector2f(280.0f, 120.0f), {180.0f, 28.0f},
        sf::Color(70, 90, 110));
    clearBlizzardButton.setup(font, "Clear Blizzard", origin + sf::Vector2f(280.0f, 160.0f), {180.0f, 28.0f},
        sf::Color(110, 60, 60));
}

void DebugWorldPanel::onOpen(DebugContext& ctx)
{
    debugMetaStats = ctx.game.getMetaProgression().getStats();
    strengthSlider.setValue(static_cast<float>(debugMetaStats.strength));
    enduranceSlider.setValue(static_cast<float>(debugMetaStats.endurance));
    healthSlider.setValue(static_cast<float>(debugMetaStats.health));
}

void DebugWorldPanel::handleMousePressed(sf::Vector2f point, DebugContext& ctx)
{
    effectPercentSlider.handleMousePressed(point);
    strengthSlider.handleMousePressed(point);
    enduranceSlider.handleMousePressed(point);
    healthSlider.handleMousePressed(point);

    if (addFragmentButton.isPressed(point)) {
        ctx.registry.get<KeyFragmentHolder>(ctx.player).count++;
    } else if (maxFragmentsButton.isPressed(point)) {
        ctx.registry.get<KeyFragmentHolder>(ctx.player).count = Biome::KEY_FRAGMENTS_REQUIRED;
    } else if (giveEquipmentButton.isPressed(point)) {
        const int slot = slotDropdown.getSelectedIndex();
        const int tier = tierDropdown.getSelectedIndex() + 1;
        const Element element = static_cast<Element>(elementDropdown.getSelectedIndex());
        const float percent = effectPercentSlider.getValue() / 100.0f;

        if (slot >= 0) {
            auto& equipment = ctx.registry.get<Equipment>(ctx.player);
            ItemUpgrader::takeNew(equipment, slot, tier, element, percent);
            applyMetaStats(ctx, debugMetaStats);
        }
    } else if (applyMetaButton.isPressed(point)) {
        applyMetaStats(ctx, debugMetaStats);
    } else if (nextBiomeButton.isPressed(point)) {
        ctx.playState.debugAdvanceToNextBiome();
    } else if (enterBossRoomButton.isPressed(point)) {
        ctx.playState.debugEnterBossRoom();
    } else if (respawnObstaclesButton.isPressed(point)) {
        ctx.playState.debugRespawnObstacles();
    } else if (spawnBlizzardButton.isPressed(point)) {
        spawnBlizzardZones(ctx.registry);
    } else if (clearBlizzardButton.isPressed(point)) {
        for (auto zone : ctx.registry.view<BlizzardZoneTag>()) {
            ctx.registry.destroy(zone);
        }
    }

    debugMetaStats.strength = static_cast<int>(std::round(strengthSlider.getValue()));
    debugMetaStats.endurance = static_cast<int>(std::round(enduranceSlider.getValue()));
    debugMetaStats.health = static_cast<int>(std::round(healthSlider.getValue()));
}

void DebugWorldPanel::handleMouseMoved(sf::Vector2f point)
{
    effectPercentSlider.handleMouseMoved(point);
    strengthSlider.handleMouseMoved(point);
    enduranceSlider.handleMouseMoved(point);
    healthSlider.handleMouseMoved(point);

    debugMetaStats.strength = static_cast<int>(std::round(strengthSlider.getValue()));
    debugMetaStats.endurance = static_cast<int>(std::round(enduranceSlider.getValue()));
    debugMetaStats.health = static_cast<int>(std::round(healthSlider.getValue()));
}

void DebugWorldPanel::handleMouseReleased()
{
    effectPercentSlider.handleMouseReleased();
    strengthSlider.handleMouseReleased();
    enduranceSlider.handleMouseReleased();
    healthSlider.handleMouseReleased();
}

bool DebugWorldPanel::handleDropdownClick(sf::Vector2f point)
{
    if (slotDropdown.handleClick(point)) return true;
    if (tierDropdown.handleClick(point)) return true;
    if (elementDropdown.handleClick(point)) return true;
    return false;
}

bool DebugWorldPanel::handleScroll(sf::Vector2f point, float delta)
{
    if (slotDropdown.handleScroll(point, delta)) return true;
    if (tierDropdown.handleScroll(point, delta)) return true;
    if (elementDropdown.handleScroll(point, delta)) return true;
    return false;
}

void DebugWorldPanel::render(sf::RenderWindow& window) const
{
    addFragmentButton.render(window);
    maxFragmentsButton.render(window);

    slotDropdown.renderHeader(window);
    tierDropdown.renderHeader(window);
    elementDropdown.renderHeader(window);
    effectPercentSlider.render(window);
    giveEquipmentButton.render(window);

    strengthSlider.render(window);
    enduranceSlider.render(window);
    healthSlider.render(window);
    applyMetaButton.render(window);

    nextBiomeButton.render(window);
    enterBossRoomButton.render(window);
    respawnObstaclesButton.render(window);
    spawnBlizzardButton.render(window);
    clearBlizzardButton.render(window);
}

void DebugWorldPanel::renderDropdownOverlays(sf::RenderWindow& window) const
{
    slotDropdown.renderExpandedList(window);
    tierDropdown.renderExpandedList(window);
    elementDropdown.renderExpandedList(window);
}

} // namespace tc

#endif // TC_DEBUG
