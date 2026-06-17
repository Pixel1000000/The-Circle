#include "debug/DebugPlayerPanel.hpp"

#ifdef TC_DEBUG

#include <cmath>
#include <string>

#include "ecs/Components.hpp"

namespace tc {

namespace {
constexpr int ONE_SHOT_DAMAGE = 99999;
}

void DebugPlayerPanel::init(const sf::Font& font, sf::Vector2f origin)
{
    constexpr float WIDGET_W = 260.0f;

    hpText.setFont(font);
    hpText.setCharacterSize(16);
    hpText.setFillColor(sf::Color::White);
    hpText.setPosition(origin.x, origin.y);

    fillHpButton.setup(font, "Fill HP", origin + sf::Vector2f(0.0f, 30.0f), {120.0f, 28.0f});
    godModeCheckbox.setup(font, "God Mode", origin + sf::Vector2f(150.0f, 34.0f));

    oneShotCheckbox.setup(font, "One Shot", origin + sf::Vector2f(0.0f, 80.0f));

    speedSlider.setup(font, "Speed", origin + sf::Vector2f(0.0f, 130.0f), {WIDGET_W, 16.0f}, 50.0f, 600.0f, 200.0f);
    armorSlider.setup(font, "Armor", origin + sf::Vector2f(0.0f, 180.0f), {WIDGET_W, 16.0f}, 0.0f, 100.0f, 0.0f, true);
    damageSlider.setup(font, "Base Damage", origin + sf::Vector2f(0.0f, 230.0f), {WIDGET_W, 16.0f}, 1.0f, 200.0f, 10.0f, true);

    fillPotionButton.setup(font, "Fill Potion", origin + sf::Vector2f(0.0f, 280.0f), {140.0f, 28.0f});
}

void DebugPlayerPanel::onOpen(DebugContext& ctx)
{
    godModeCheckbox.checked = ctx.registry.all_of<Invulnerable>(ctx.player);
    oneShotCheckbox.checked = oneShotActive;

    speedSlider.setValue(ctx.registry.get<Speed>(ctx.player).value);
    armorSlider.setValue(static_cast<float>(ctx.registry.get<Armor>(ctx.player).value));
    damageSlider.setValue(static_cast<float>(
        oneShotActive ? savedDamageValue : ctx.registry.get<Damage>(ctx.player).value));
}

void DebugPlayerPanel::handleMousePressed(sf::Vector2f point, DebugContext& ctx)
{
    speedSlider.handleMousePressed(point);
    armorSlider.handleMousePressed(point);
    damageSlider.handleMousePressed(point);

    if (fillHpButton.isPressed(point)) {
        auto& health = ctx.registry.get<Health>(ctx.player);
        health.current = health.max;
    } else if (fillPotionButton.isPressed(point)) {
        auto& potion = ctx.registry.get<Potion>(ctx.player);
        potion.charges = potion.maxCharges;
    } else if (godModeCheckbox.isClicked(point)) {
        godModeCheckbox.toggle();
        if (godModeCheckbox.checked) {
            ctx.registry.emplace_or_replace<Invulnerable>(ctx.player);
        } else {
            ctx.registry.remove<Invulnerable>(ctx.player);
        }
    } else if (oneShotCheckbox.isClicked(point)) {
        oneShotCheckbox.toggle();
        oneShotActive = oneShotCheckbox.checked;
        auto& damage = ctx.registry.get<Damage>(ctx.player);
        if (oneShotActive) {
            savedDamageValue = damage.value;
            damage.value = ONE_SHOT_DAMAGE;
        } else {
            damage.value = savedDamageValue;
            damageSlider.setValue(static_cast<float>(savedDamageValue));
        }
    }

    applySlidersToEntity(ctx);
}

void DebugPlayerPanel::handleMouseMoved(sf::Vector2f point, DebugContext& ctx)
{
    speedSlider.handleMouseMoved(point);
    armorSlider.handleMouseMoved(point);
    damageSlider.handleMouseMoved(point);

    applySlidersToEntity(ctx);
}

void DebugPlayerPanel::applySlidersToEntity(DebugContext& ctx)
{
    ctx.registry.get<Speed>(ctx.player).value = speedSlider.getValue();
    ctx.registry.get<Armor>(ctx.player).value = static_cast<int>(std::round(armorSlider.getValue()));

    if (oneShotActive) {
        savedDamageValue = static_cast<int>(std::round(damageSlider.getValue()));
    } else {
        ctx.registry.get<Damage>(ctx.player).value = static_cast<int>(std::round(damageSlider.getValue()));
    }
}

void DebugPlayerPanel::handleMouseReleased()
{
    speedSlider.handleMouseReleased();
    armorSlider.handleMouseReleased();
    damageSlider.handleMouseReleased();
}

void DebugPlayerPanel::render(sf::RenderWindow& window, DebugContext& ctx)
{
    const auto& health = ctx.registry.get<Health>(ctx.player);
    hpText.setString("HP: " + std::to_string(health.current) + " / " + std::to_string(health.max));
    window.draw(hpText);

    fillHpButton.render(window);
    godModeCheckbox.render(window);
    oneShotCheckbox.render(window);

    speedSlider.render(window);
    armorSlider.render(window);
    damageSlider.render(window);

    fillPotionButton.render(window);
}

} // namespace tc

#endif // TC_DEBUG
