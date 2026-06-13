#include "ui/HUD.hpp"

#include <string>

#include "core/TextUtils.hpp"
#include "ecs/Components.hpp"

namespace tc {

namespace {
constexpr float BAR_WIDTH = 300.0f;
constexpr float BAR_HEIGHT = 24.0f;
constexpr float MARGIN = 20.0f;
}

void HUD::render(sf::RenderWindow& window, entt::registry& registry, entt::entity player,
    const Localization& localization, const FontManager& fontManager, int keyFragmentsRequired)
{
    const auto& health = registry.get<Health>(player);
    const auto& fragments = registry.get<KeyFragmentHolder>(player);
    const auto& potion = registry.get<Potion>(player);

    const float healthRatio = health.max > 0
        ? static_cast<float>(health.current) / static_cast<float>(health.max)
        : 0.0f;

    sf::RectangleShape healthBack({BAR_WIDTH, BAR_HEIGHT});
    healthBack.setPosition(MARGIN, MARGIN);
    healthBack.setFillColor(sf::Color(40, 40, 40));
    window.draw(healthBack);

    sf::RectangleShape healthFill({BAR_WIDTH * healthRatio, BAR_HEIGHT});
    healthFill.setPosition(MARGIN, MARGIN);
    healthFill.setFillColor(sf::Color(200, 60, 60));
    window.draw(healthFill);

    if (!fontManager.isLoaded()) {
        return;
    }

    const sf::Font& font = fontManager.getFont(localization.getCurrentLanguage());

    sf::Text healthText;
    healthText.setFont(font);
    healthText.setCharacterSize(16);
    healthText.setFillColor(sf::Color::White);
    healthText.setString(toSfString(localization.get("hud.health") + ": "
        + std::to_string(health.current) + "/" + std::to_string(health.max)));
    healthText.setPosition(MARGIN + 6.0f, MARGIN + 2.0f);
    window.draw(healthText);

    sf::Text fragmentsText;
    fragmentsText.setFont(font);
    fragmentsText.setCharacterSize(16);
    fragmentsText.setFillColor(sf::Color::White);
    fragmentsText.setString(toSfString(localization.get("hud.fragments") + ": "
        + std::to_string(fragments.count) + "/" + std::to_string(keyFragmentsRequired)));
    fragmentsText.setPosition(MARGIN, MARGIN + BAR_HEIGHT + 8.0f);
    window.draw(fragmentsText);

    sf::Text potionText;
    potionText.setFont(font);
    potionText.setCharacterSize(16);
    potionText.setFillColor(sf::Color::White);
    potionText.setString(toSfString(localization.get("hud.potion") + ": "
        + std::to_string(potion.charges) + "/" + std::to_string(potion.maxCharges)));
    potionText.setPosition(MARGIN, MARGIN + BAR_HEIGHT + 32.0f);
    window.draw(potionText);
}

} // namespace tc
