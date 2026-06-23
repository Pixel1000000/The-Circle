#include "ecs/systems/RenderSystem.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "core/TextUtils.hpp"
#include "ecs/Components.hpp"

namespace tc {

namespace {
constexpr float HP_BAR_HEIGHT = 5.0f;
constexpr float HP_BAR_GAP = 6.0f;
constexpr float HP_BAR_MIN_WIDTH = 28.0f;
constexpr unsigned int NAME_FONT_SIZE = 12;
}

void RenderSystem::update(entt::registry& registry, sf::RenderWindow& window, const Localization& localization,
    const FontManager& fontManager, float dt)
{
    std::vector<entt::entity> expiredFlashes;
    for (auto entity : registry.view<HitFlash>()) {
        auto& flash = registry.get<HitFlash>(entity);
        flash.timer -= dt;
        if (flash.timer <= 0.0f) {
            expiredFlashes.push_back(entity);
        }
    }
    for (auto entity : expiredFlashes) {
        registry.remove<HitFlash>(entity);
    }

    if (fontManager.isLoaded() && !textInitialized) {
        nameText.setOutlineColor(sf::Color::Black);
        nameText.setOutlineThickness(1.0f);
        nameText.setFillColor(sf::Color::White);
        nameText.setCharacterSize(NAME_FONT_SIZE);
        textInitialized = true;
    }
    if (fontManager.isLoaded()) {
        nameText.setFont(fontManager.getFont(localization.getCurrentLanguage()));
    }

#ifdef TC_DEBUG
    if (fontManager.isLoaded() && !debugTextInitialized) {
        hpNumberText.setOutlineColor(sf::Color::Black);
        hpNumberText.setOutlineThickness(1.0f);
        hpNumberText.setFillColor(sf::Color::Yellow);
        hpNumberText.setCharacterSize(NAME_FONT_SIZE);
        debugTextInitialized = true;
    }
    if (fontManager.isLoaded()) {
        hpNumberText.setFont(fontManager.getFont(localization.getCurrentLanguage()));
    }
#endif

    // Mummy death-bomb telegraph: a light-red danger zone showing the blast
    // radius, drawn under all units.
    for (auto entity : registry.view<MummyDeathBomb, Position>()) {
        const auto& bomb = registry.get<MummyDeathBomb>(entity);
        if (bomb.triggered || bomb.timer <= 0.0f) continue;

        const auto& pos = registry.get<Position>(entity);
        sf::CircleShape danger(bomb.radius);
        danger.setOrigin(bomb.radius, bomb.radius);
        danger.setPosition(pos.x, pos.y);
        danger.setFillColor(sf::Color(255, 80, 80, 70));
        window.draw(danger);
    }

    auto view = registry.view<Position, Renderable>();
    for (auto entity : view) {
        const auto& pos = view.get<Position>(entity);
        const auto& renderable = view.get<Renderable>(entity);

        sf::Color color = renderable.color;
        if (const auto* flash = registry.try_get<HitFlash>(entity)) {
            const float t = std::clamp(flash->timer / HitFlash::DURATION, 0.0f, 1.0f);
            color.r = static_cast<sf::Uint8>(color.r + (255 - color.r) * t);
            color.g = static_cast<sf::Uint8>(color.g * (1.0f - t));
            color.b = static_cast<sf::Uint8>(color.b * (1.0f - t));
        }

        sf::RectangleShape shape(renderable.size);
        shape.setOrigin(renderable.size.x * 0.5f, renderable.size.y * 0.5f);
        shape.setPosition(pos.x, pos.y);
        shape.setFillColor(color);

        window.draw(shape);

#ifdef TC_DEBUG
        if (showHitboxes) {
            sf::RectangleShape outline(renderable.size);
            outline.setOrigin(renderable.size.x * 0.5f, renderable.size.y * 0.5f);
            outline.setPosition(pos.x, pos.y);
            outline.setFillColor(sf::Color::Transparent);
            outline.setOutlineColor(sf::Color::Yellow);
            outline.setOutlineThickness(1.0f);
            window.draw(outline);
        }

        if (showVelocityVectors) {
            if (const auto* velocity = registry.try_get<Velocity>(entity)) {
                const bool isPlayer = registry.all_of<PlayerTag>(entity);
                const sf::Color lineColor = isPlayer ? sf::Color::Green : sf::Color::Red;
                constexpr float VELOCITY_VECTOR_SCALE = 30.0f;
                sf::Vertex line[2] = {
                    sf::Vertex({pos.x, pos.y}, lineColor),
                    sf::Vertex({pos.x + velocity->dx * VELOCITY_VECTOR_SCALE, pos.y + velocity->dy * VELOCITY_VECTOR_SCALE}, lineColor)
                };
                window.draw(line, 2, sf::Lines);
            }
        }
#endif

        // Elemental status overlays — semi-transparent tint on the sprite.
        {
            const auto* se = registry.try_get<StatusEffect>(entity);
            sf::Color tint(0, 0, 0, 0);
            if (registry.all_of<NatureStack>(entity) ||
                    (se && se->type == StatusEffect::POISON)) {
                tint = sf::Color(100, 255, 100, 90);
            } else if (registry.all_of<FireBurn>(entity)) {
                tint = sf::Color(255, 100, 30, 90);
            } else if (registry.all_of<IceChill>(entity) ||
                    (se && se->type == StatusEffect::SLOW)) {
                tint = sf::Color(0, 220, 255, 90);
            } else if (registry.all_of<DecayEffect>(entity)) {
                tint = sf::Color(140, 140, 140, 90);
            }
            if (tint.a > 0) {
                sf::RectangleShape tintShape(renderable.size);
                tintShape.setOrigin(renderable.size.x * 0.5f, renderable.size.y * 0.5f);
                tintShape.setPosition(pos.x, pos.y);
                tintShape.setFillColor(tint);
                window.draw(tintShape);
            }
        }

        if (registry.all_of<Invulnerable>(entity)) {
            sf::RectangleShape shield(renderable.size + sf::Vector2f(16.0f, 16.0f));
            shield.setOrigin(shield.getSize().x * 0.5f, shield.getSize().y * 0.5f);
            shield.setPosition(pos.x, pos.y);
            shield.setFillColor(sf::Color(180, 70, 255, 90));
            shield.setOutlineColor(sf::Color(220, 140, 255, 220));
            shield.setOutlineThickness(2.0f);
            window.draw(shield);
        }

        const auto* health = registry.try_get<Health>(entity);
        if (!health || !registry.all_of<EnemyTag>(entity)) continue;

        const float barWidth = std::max(renderable.size.x, HP_BAR_MIN_WIDTH);
        const float barX = pos.x - barWidth * 0.5f;
        const float barY = pos.y - renderable.size.y * 0.5f - HP_BAR_GAP - HP_BAR_HEIGHT;
        const float ratio = health->max > 0
            ? std::clamp(static_cast<float>(health->current) / static_cast<float>(health->max), 0.0f, 1.0f)
            : 0.0f;

        sf::RectangleShape barBack({barWidth, HP_BAR_HEIGHT});
        barBack.setPosition(barX, barY);
        barBack.setFillColor(sf::Color(40, 40, 40));
        window.draw(barBack);

        sf::RectangleShape barFill({barWidth * ratio, HP_BAR_HEIGHT});
        barFill.setPosition(barX, barY);
        barFill.setFillColor(sf::Color(200, 60, 60));
        window.draw(barFill);

#ifdef TC_DEBUG
        if (showHpNumbers && fontManager.isLoaded()) {
            hpNumberText.setString(std::to_string(health->current) + "/" + std::to_string(health->max));
            const auto hpBounds = hpNumberText.getLocalBounds();
            hpNumberText.setPosition(pos.x - hpBounds.width * 0.5f - hpBounds.left,
                pos.y - renderable.size.y * 0.5f - HP_BAR_GAP - HP_BAR_HEIGHT - NAME_FONT_SIZE - 4.0f);
            window.draw(hpNumberText);
        }
#endif

        if (!fontManager.isLoaded()) continue;

        const auto* name = registry.try_get<Name>(entity);
        if (!name) continue;

        nameText.setString(toSfString(localization.get("enemy." + name->id)));
        const auto bounds = nameText.getLocalBounds();
        nameText.setPosition(pos.x - bounds.width * 0.5f - bounds.left, barY + HP_BAR_HEIGHT + 2.0f);
        window.draw(nameText);
    }
}

} // namespace tc
