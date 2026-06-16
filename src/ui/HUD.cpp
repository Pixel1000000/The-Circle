#include "ui/HUD.hpp"

#include <cmath>
#include <string>

#include "core/TextUtils.hpp"
#include "ecs/Components.hpp"

namespace tc {

namespace {
constexpr float BAR_WIDTH  = 300.0f;
constexpr float BAR_HEIGHT = 24.0f;
constexpr float MARGIN     = 20.0f;
constexpr float WORLD_WIDTH = 1280.0f;
constexpr float DROP_NOTICE_DURATION = 2.5f;

// Status-effect icons sit to the right of the HP bar.
constexpr float ICON_SIZE = BAR_HEIGHT + 4.0f;   // slightly taller than HP bar
constexpr float ICON_GAP  = 4.0f;
constexpr float ICONS_X   = MARGIN + BAR_WIDTH + 8.0f;

std::string equipmentSlotName(const Localization& localization, const Equipment& equipment, int slot)
{
    switch (slot) {
    case 0: return localization.get("equipment.helmet");
    case 1: return localization.get("equipment.chest");
    case 2: return localization.get("equipment.legs");
    default:
        return equipment.weaponType == Equipment::BOW
            ? localization.get("equipment.bow")
            : localization.get("equipment.sword");
    }
}
} // namespace

void HUD::render(sf::RenderWindow& window, entt::registry& registry, entt::entity player,
    const Localization& localization, const FontManager& fontManager, int keyFragmentsRequired,
    bool showNextBiomeHint, float dt)
{
    const auto& health    = registry.get<Health>(player);
    const auto& fragments = registry.get<KeyFragmentHolder>(player);
    const auto& potion    = registry.get<Potion>(player);
    const auto& equipment = registry.get<Equipment>(player);

    if (dropNoticeTimer > 0.0f) {
        dropNoticeTimer -= dt;
    }

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

    if (!textInitialized) {
        healthText.setFont(font);
        healthText.setCharacterSize(16);
        healthText.setFillColor(sf::Color::White);
        healthText.setPosition(MARGIN + 6.0f, MARGIN + 2.0f);

        fragmentsText.setFont(font);
        fragmentsText.setCharacterSize(16);
        fragmentsText.setFillColor(sf::Color::White);
        fragmentsText.setPosition(MARGIN, MARGIN + BAR_HEIGHT + 8.0f);

        potionText.setFont(font);
        potionText.setCharacterSize(16);
        potionText.setFillColor(sf::Color::White);
        potionText.setPosition(MARGIN, MARGIN + BAR_HEIGHT + 32.0f);

        equipmentText.setFont(font);
        equipmentText.setCharacterSize(16);
        equipmentText.setFillColor(sf::Color::White);

        nextBiomeText.setFont(font);
        nextBiomeText.setCharacterSize(20);
        nextBiomeText.setFillColor(sf::Color::Yellow);

        dropNoticeText.setFont(font);
        dropNoticeText.setCharacterSize(20);
        dropNoticeText.setFillColor(sf::Color(255, 215, 0));

        textInitialized = true;
    }

    healthText.setString(toSfString(localization.get("hud.health") + ": "
        + std::to_string(health.current) + "/" + std::to_string(health.max)));
    window.draw(healthText);

    fragmentsText.setString(toSfString(localization.get("hud.fragments") + ": "
        + std::to_string(fragments.count) + "/" + std::to_string(keyFragmentsRequired)));
    window.draw(fragmentsText);

    potionText.setString(toSfString(localization.get("hud.potion") + ": "
        + std::to_string(potion.charges) + "/" + std::to_string(potion.maxCharges)));
    window.draw(potionText);

    // ── Equipment bar: only name + tier (stats are in the inventory screen) ──
    const std::string weaponLabel = equipment.weaponType == Equipment::BOW
        ? localization.get("equipment.bow")
        : localization.get("equipment.sword");

    const sf::Color equippedColor(255, 255, 255);
    const sf::Color emptyColor(120, 120, 120);

    float x = MARGIN;
    const float y = MARGIN + BAR_HEIGHT + 56.0f;

    auto drawSegment = [&](const std::string& str, int tier) {
        equipmentText.setString(toSfString(str));
        equipmentText.setFillColor(tier == 0 ? emptyColor : equippedColor);
        equipmentText.setPosition(x, y);
        window.draw(equipmentText);
        x = equipmentText.findCharacterPos(equipmentText.getString().getSize()).x;
    };
    auto drawSep = [&]() {
        equipmentText.setString(toSfString(" | "));
        equipmentText.setFillColor(equippedColor);
        equipmentText.setPosition(x, y);
        window.draw(equipmentText);
        x = equipmentText.findCharacterPos(equipmentText.getString().getSize()).x;
    };

    drawSegment(weaponLabel + " T" + std::to_string(equipment.weaponTier),   equipment.weaponTier);
    drawSep();
    drawSegment(localization.get("equipment.helmet") + " T" + std::to_string(equipment.helmetTier),   equipment.helmetTier);
    drawSep();
    drawSegment(localization.get("equipment.chest")  + " T" + std::to_string(equipment.chestTier),    equipment.chestTier);
    drawSep();
    drawSegment(localization.get("equipment.legs")   + " T" + std::to_string(equipment.leggingsTier), equipment.leggingsTier);

    // ── Status-effect icons (right of HP bar) ────────────────────────────────
    float iconX = ICONS_X;

    auto drawStatusIcon = [&](sf::Color bgColor, const std::string& sym, float timer) {
        // Background box
        sf::RectangleShape box({ICON_SIZE, ICON_SIZE});
        box.setPosition(iconX, MARGIN);
        box.setFillColor(bgColor);
        window.draw(box);

        // Symbol (top portion of box)
        sf::Text symTxt;
        symTxt.setFont(font);
        symTxt.setCharacterSize(13);
        symTxt.setFillColor(sf::Color::White);
        symTxt.setString(sym);
        const auto sb = symTxt.getLocalBounds();
        symTxt.setOrigin(sb.left + sb.width / 2.0f, 0.0f);
        symTxt.setPosition(iconX + ICON_SIZE / 2.0f, MARGIN + 2.0f);
        window.draw(symTxt);

        // Timer (bottom portion of box)
        const int secs = std::max(1, static_cast<int>(std::ceil(timer)));
        sf::Text timerTxt;
        timerTxt.setFont(font);
        timerTxt.setCharacterSize(11);
        timerTxt.setFillColor(sf::Color::White);
        timerTxt.setString(std::to_string(secs) + "s");
        const auto tb = timerTxt.getLocalBounds();
        timerTxt.setOrigin(tb.left + tb.width / 2.0f, 0.0f);
        timerTxt.setPosition(iconX + ICON_SIZE / 2.0f, MARGIN + ICON_SIZE - 14.0f);
        window.draw(timerTxt);

        iconX += ICON_SIZE + ICON_GAP;
    };

    const auto* se = registry.try_get<StatusEffect>(player);
    if (se) {
        if (se->type == StatusEffect::POISON)
            drawStatusIcon(sf::Color(50, 160, 50, 220),   "P", se->timer);
        else if (se->type == StatusEffect::SLOW)
            drawStatusIcon(sf::Color(0, 160, 210, 220),   "S", se->timer);
    }
    if (const auto* ns = registry.try_get<NatureStack>(player))
        drawStatusIcon(sf::Color(50, 200, 50, 220),       "N", ns->timer);
    if (const auto* fb = registry.try_get<FireBurn>(player))
        drawStatusIcon(sf::Color(220, 90, 20, 220),       "F", fb->timer);
    if (const auto* de = registry.try_get<DecayEffect>(player))
        drawStatusIcon(sf::Color(120, 120, 120, 220),     "D", de->timer);

    // ── Misc HUD ─────────────────────────────────────────────────────────────
    if (showNextBiomeHint) {
        nextBiomeText.setString(toSfString(localization.get("hud.nextbiome")));
        const auto bounds = nextBiomeText.getLocalBounds();
        nextBiomeText.setPosition((WORLD_WIDTH - bounds.width) / 2.0f - bounds.left, MARGIN);
        window.draw(nextBiomeText);
    }

    if (dropNoticeTimer > 0.0f) {
        const std::string slotName = equipmentSlotName(localization, equipment, dropNoticeSlot);
        dropNoticeText.setString(toSfString(localization.get("hud.found") + ": "
            + slotName + " T" + std::to_string(dropNoticeTier)));
        const auto bounds = dropNoticeText.getLocalBounds();
        dropNoticeText.setPosition((WORLD_WIDTH - bounds.width) / 2.0f - bounds.left, MARGIN + 30.0f);
        window.draw(dropNoticeText);
    }
}

void HUD::showEquipmentDrop(int slot, int tier)
{
    dropNoticeSlot = slot;
    dropNoticeTier = tier;
    dropNoticeTimer = DROP_NOTICE_DURATION;
}

} // namespace tc
