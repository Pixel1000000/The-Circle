#include "ui/HUD.hpp"

#include <cmath>
#include <string>

#include "config/ConfigLoader.hpp"
#include "config/EquipmentConfig.hpp"
#include "core/TextUtils.hpp"
#include "ecs/Components.hpp"

namespace tc {

namespace {
constexpr float BAR_WIDTH = 300.0f;
constexpr float BAR_HEIGHT = 24.0f;
constexpr float MARGIN = 20.0f;
constexpr float WORLD_WIDTH = 1280.0f;
constexpr float DROP_NOTICE_DURATION = 2.5f;
constexpr float STAT_LINE_OFFSET = 20.0f;

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

// Armor bonus granted by a single equipped piece at the given tier (0 = none equipped).
int armorBonusFor(const std::array<ArmorPieceStats, TIER_COUNT>& tiers, int tier)
{
    if (tier < 1 || tier > TIER_COUNT) return 0;
    return tiers[static_cast<std::size_t>(tier - 1)].armor;
}

// Returns "ElementName +X%" string for a slot, or "" if element is NONE.
std::string elementStatStr(const Localization& localization, Element element, float percent)
{
    std::string key;
    switch (element) {
    case Element::NATURE: key = "element.nature"; break;
    case Element::FIRE:   key = "element.fire";   break;
    case Element::ICE:    key = "element.ice";     break;
    case Element::DECAY:  key = "element.decay";   break;
    default: return "";
    }
    return localization.get(key) + " +" + std::to_string(static_cast<int>(std::round(percent * 100.0f))) + "%";
}
}

void HUD::render(sf::RenderWindow& window, entt::registry& registry, entt::entity player,
    const Localization& localization, const FontManager& fontManager, int keyFragmentsRequired,
    bool showNextBiomeHint, float dt)
{
    const auto& health = registry.get<Health>(player);
    const auto& fragments = registry.get<KeyFragmentHolder>(player);
    const auto& potion = registry.get<Potion>(player);
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

        equipmentStatText.setFont(font);
        equipmentStatText.setCharacterSize(13);
        equipmentStatText.setFillColor(sf::Color::White);

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

    const std::string weaponLabel = equipment.weaponType == Equipment::BOW
        ? localization.get("equipment.bow")
        : localization.get("equipment.sword");

    const auto& equipmentConfig = ConfigLoader::get().getEquipmentConfig();
    const int currentDamage = registry.get<Damage>(player).value;

    const sf::Color equippedColor(255, 255, 255);
    const sf::Color emptyColor(120, 120, 120);

    auto withElem = [&](std::string base, Element elem, float pct) -> std::string {
        const std::string e = elementStatStr(localization, elem, pct);
        return e.empty() ? base : base + " | " + e;
    };

    float x = MARGIN;
    const float y = MARGIN + BAR_HEIGHT + 56.0f;

    auto drawEquipmentSegment = [&](const std::string& str, int tier, const std::string& statStr) {
        const float segmentX = x;
        const sf::Color color = tier == 0 ? emptyColor : equippedColor;

        equipmentText.setString(toSfString(str));
        equipmentText.setFillColor(color);
        equipmentText.setPosition(segmentX, y);
        window.draw(equipmentText);
        x = equipmentText.findCharacterPos(equipmentText.getString().getSize()).x;

        equipmentStatText.setString(toSfString(statStr));
        equipmentStatText.setFillColor(color);
        equipmentStatText.setPosition(segmentX, y + STAT_LINE_OFFSET);
        window.draw(equipmentStatText);
    };
    auto drawSeparator = [&]() {
        equipmentText.setString(toSfString(" | "));
        equipmentText.setFillColor(equippedColor);
        equipmentText.setPosition(x, y);
        window.draw(equipmentText);
        x = equipmentText.findCharacterPos(equipmentText.getString().getSize()).x;
    };

    drawEquipmentSegment(weaponLabel + " T" + std::to_string(equipment.weaponTier), equipment.weaponTier,
        withElem(localization.get("equipment.damage") + " " + std::to_string(currentDamage),
            equipment.weaponElement, equipment.weaponElementPercent));
    drawSeparator();
    drawEquipmentSegment(localization.get("equipment.helmet") + " T" + std::to_string(equipment.helmetTier), equipment.helmetTier,
        withElem(localization.get("equipment.armor") + " " + std::to_string(armorBonusFor(equipmentConfig.helmet, equipment.helmetTier)),
            equipment.helmetElement, equipment.helmetElementPercent));
    drawSeparator();
    drawEquipmentSegment(localization.get("equipment.chest") + " T" + std::to_string(equipment.chestTier), equipment.chestTier,
        withElem(localization.get("equipment.armor") + " " + std::to_string(armorBonusFor(equipmentConfig.chest, equipment.chestTier)),
            equipment.chestElement, equipment.chestElementPercent));
    drawSeparator();
    drawEquipmentSegment(localization.get("equipment.legs") + " T" + std::to_string(equipment.leggingsTier), equipment.leggingsTier,
        withElem(localization.get("equipment.armor") + " " + std::to_string(armorBonusFor(equipmentConfig.leggings, equipment.leggingsTier)),
            equipment.leggingsElement, equipment.leggingsElementPercent));

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
