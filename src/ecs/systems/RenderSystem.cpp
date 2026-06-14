#include "ecs/systems/RenderSystem.hpp"

#include <algorithm>
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
