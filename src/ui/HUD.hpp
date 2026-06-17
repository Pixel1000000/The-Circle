#pragma once

#include <entt/entt.hpp>
#include <SFML/Graphics.hpp>

#include "core/FontManager.hpp"
#include "core/Localization.hpp"

namespace tc {

// In-game overlay: player health bar, key fragment progress, equipment and
// potion charges.
class HUD {
public:
    void render(sf::RenderWindow& window, entt::registry& registry, entt::entity player,
        const Localization& localization, const FontManager& fontManager, int keyFragmentsRequired,
        bool showNextBiomeHint, float dt);

    // Shows a "found item" notice for a few seconds. slot: 0 = helmet,
    // 1 = chest, 2 = leggings, 3 = weapon.
    void showEquipmentDrop(int slot, int tier);

    // Visibility flags — toggled by the developer panel.
    bool showHealth      = true;
    bool showFragments   = true;
    bool showPotions     = true;
    bool showStatusIcons = true;

private:
    sf::Text healthText;
    sf::Text fragmentsText;
    sf::Text potionText;
    sf::Text equipmentText;
    sf::Text equipmentStatText;
    sf::Text nextBiomeText;
    sf::Text dropNoticeText;
    bool textInitialized = false;

    float dropNoticeTimer = 0.0f;
    int dropNoticeSlot = 0;
    int dropNoticeTier = 0;
};

} // namespace tc
