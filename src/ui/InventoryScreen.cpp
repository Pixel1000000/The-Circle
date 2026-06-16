#include "ui/InventoryScreen.hpp"

#include <cmath>
#include <string>

#include "config/ConfigLoader.hpp"
#include "config/EquipmentConfig.hpp"
#include "core/TextUtils.hpp"
#include "ecs/Components.hpp"

namespace tc {

namespace {

constexpr float SCREEN_W = 1280.0f;
constexpr float SCREEN_H = 720.0f;

constexpr float PANEL_X  = 120.0f;
constexpr float PANEL_Y  = 55.0f;
constexpr float PANEL_W  = 1040.0f;
constexpr float PANEL_H  = 610.0f;

constexpr float NAME_COL = 145.0f;   // slot name + tier
constexpr float STAT_COL = 460.0f;   // stat lines

constexpr float ROW_Y[4] = { 130.0f, 255.0f, 380.0f, 505.0f };

constexpr float HELP_BTN_X = PANEL_X + PANEL_W - 46.0f;
constexpr float HELP_BTN_Y = PANEL_Y + 10.0f;
constexpr float HELP_BTN_SZ = 36.0f;

// Help panel
constexpr float HP_X = 200.0f;
constexpr float HP_Y = 80.0f;
constexpr float HP_W = 880.0f;
constexpr float HP_H = 560.0f;

// ── helpers ──────────────────────────────────────────────────────────────────

sf::Color elementColor(Element e)
{
    switch (e) {
    case Element::NATURE: return sf::Color(90,  220,  90);
    case Element::FIRE:   return sf::Color(255, 130,  40);
    case Element::ICE:    return sf::Color(80,  200, 255);
    case Element::DECAY:  return sf::Color(190, 190, 190);
    default:              return sf::Color(160, 160, 160);
    }
}

std::string elementStr(const Localization& loc, Element e, float pct)
{
    if (e == Element::NONE) return "";
    std::string key;
    switch (e) {
    case Element::NATURE: key = "element.nature"; break;
    case Element::FIRE:   key = "element.fire";   break;
    case Element::ICE:    key = "element.ice";     break;
    case Element::DECAY:  key = "element.decay";   break;
    default: return "";
    }
    return loc.get(key) + " +" + std::to_string(static_cast<int>(std::round(pct * 100.0f))) + "%";
}

// Returns "bonus.<slotType>.<element>" localization key.
// slot: 0=helmet, 1=chest → "armor"; 2=leggings → "leggings"; 3=weapon → "weapon"
std::string bonusKey(int slot, Element e)
{
    if (e == Element::NONE) return "";
    std::string slotType;
    if      (slot == 3) slotType = "weapon";
    else if (slot == 2) slotType = "leggings";
    else                slotType = "armor";

    std::string elemSuffix;
    switch (e) {
    case Element::NATURE: elemSuffix = "nature"; break;
    case Element::FIRE:   elemSuffix = "fire";   break;
    case Element::ICE:    elemSuffix = "ice";     break;
    case Element::DECAY:  elemSuffix = "decay";   break;
    default: return "";
    }
    return "bonus." + slotType + "." + elemSuffix;
}

int armorForSlot(const EquipmentConfig& cfg, int slot, int tier)
{
    if (tier < 1 || tier > TIER_COUNT) return 0;
    const auto idx = static_cast<std::size_t>(tier - 1);
    switch (slot) {
    case 0: return cfg.helmet[idx].armor;
    case 1: return cfg.chest[idx].armor;
    case 2: return cfg.leggings[idx].armor;
    default: return 0;
    }
}

// ── help panel ───────────────────────────────────────────────────────────────
// Vertical list layout: 3 sections (weapon / armor / leggings), each listing
// all 4 elements on separate lines. Fits within screen bounds at any language.

void drawHelpPanel(sf::RenderWindow& window, const Localization& loc, const sf::Font& font)
{
    // Overlay the whole inventory panel area
    sf::RectangleShape bg({PANEL_W, PANEL_H});
    bg.setPosition(PANEL_X, PANEL_Y);
    bg.setFillColor(sf::Color(8, 8, 18, 248));
    bg.setOutlineThickness(2.0f);
    bg.setOutlineColor(sf::Color(100, 100, 160));
    window.draw(bg);

    const float PAD = 28.0f;
    const float ELEM_COL = PANEL_X + PAD + 220.0f; // where element name starts
    const float DESC_COL = ELEM_COL + 160.0f;       // where description starts
    float curY = PANEL_Y + 16.0f;

    auto text = [&](const std::string& str, float x, float y, unsigned int sz, sf::Color col) {
        sf::Text t;
        t.setFont(font);
        t.setCharacterSize(sz);
        t.setFillColor(col);
        t.setString(toSfString(str));
        t.setPosition(x, y);
        window.draw(t);
    };

    text(loc.get("help.title"), PANEL_X + PAD, curY, 26, sf::Color::White);
    curY += 42.0f;

    struct Section {
        const char* headerKey;
        const char* bonusKeys[4];
    };
    const Section sections[3] = {
        { "help.weapon",
          { "bonus.weapon.nature", "bonus.weapon.fire", "bonus.weapon.ice", "bonus.weapon.decay" } },
        { "help.armor",
          { "bonus.armor.nature",  "bonus.armor.fire",  "bonus.armor.ice",  "bonus.armor.decay"  } },
        { "help.leggings",
          { "bonus.leggings.nature", "bonus.leggings.fire", "bonus.leggings.ice", "bonus.leggings.decay" } },
    };

    const Element elems[4]    = { Element::NATURE, Element::FIRE, Element::ICE, Element::DECAY };
    const char*  elemKeys[4]  = { "element.nature", "element.fire", "element.ice", "element.decay" };

    for (const auto& sec : sections) {
        // Section header
        sf::RectangleShape hdrLine({PANEL_W - PAD * 2.0f, 1.0f});
        hdrLine.setPosition(PANEL_X + PAD, curY - 4.0f);
        hdrLine.setFillColor(sf::Color(70, 70, 100));
        window.draw(hdrLine);

        text(loc.get(sec.headerKey), PANEL_X + PAD, curY, 20, sf::Color(220, 220, 130));
        curY += 28.0f;

        for (int i = 0; i < 4; ++i) {
            text(loc.get(elemKeys[i]), ELEM_COL, curY, 17, elementColor(elems[i]));
            text(loc.get(sec.bonusKeys[i]), DESC_COL, curY, 17, sf::Color(210, 210, 210));
            curY += 24.0f;
        }
        curY += 16.0f; // gap between sections
    }
}

} // namespace

// ── InventoryScreen ───────────────────────────────────────────────────────────

InventoryScreen::InventoryScreen()
{
    panel.setSize({PANEL_W, PANEL_H});
    panel.setPosition(PANEL_X, PANEL_Y);
    panel.setFillColor(sf::Color(10, 10, 20, 230));
    panel.setOutlineThickness(2.0f);
    panel.setOutlineColor(sf::Color(80, 80, 120));

    helpBtn.setSize({HELP_BTN_SZ, HELP_BTN_SZ});
    helpBtn.setPosition(HELP_BTN_X, HELP_BTN_Y);
    helpBtn.setFillColor(sf::Color(60, 60, 100));
    helpBtn.setOutlineThickness(2.0f);
    helpBtn.setOutlineColor(sf::Color(140, 140, 200));
}

void InventoryScreen::render(sf::RenderWindow& window, const Localization& loc,
    const FontManager& fontManager, entt::registry& registry, entt::entity player)
{
    window.draw(panel);
    window.draw(helpBtn);

    if (!fontManager.isLoaded()) return;

    const sf::Font& font = fontManager.getFont(loc.getCurrentLanguage());
    const auto& eq  = registry.get<Equipment>(player);
    const int curDmg = registry.get<Damage>(player).value;
    const auto& equipCfg = ConfigLoader::get().getEquipmentConfig();

    // Title
    sf::Text titleTxt;
    titleTxt.setFont(font);
    titleTxt.setCharacterSize(28);
    titleTxt.setFillColor(sf::Color::White);
    titleTxt.setString(toSfString(loc.get("inventory.title")));
    titleTxt.setPosition(NAME_COL, PANEL_Y + 14.0f);
    window.draw(titleTxt);

    // ? label inside help button
    sf::Text helpLbl;
    helpLbl.setFont(font);
    helpLbl.setCharacterSize(22);
    helpLbl.setFillColor(sf::Color(200, 200, 255));
    helpLbl.setString("?");
    const auto hb = helpLbl.getLocalBounds();
    helpLbl.setOrigin(hb.left + hb.width / 2.0f, hb.top + hb.height / 2.0f);
    helpLbl.setPosition(HELP_BTN_X + HELP_BTN_SZ / 2.0f, HELP_BTN_Y + HELP_BTN_SZ / 2.0f);
    window.draw(helpLbl);

    // ── Per-slot rows ──────────────────────────────────────────────────────
    struct SlotInfo {
        std::string nameKey;
        int tier;
        Element element;
        float percent;
        std::string primaryStatLabel;
        int primaryStatValue;
    };

    const std::string weaponKey = eq.weaponType == Equipment::BOW ? "equipment.bow" : "equipment.sword";
    const SlotInfo slots[4] = {
        { "equipment.helmet", eq.helmetTier,   eq.helmetElement,   eq.helmetElementPercent,
          loc.get("equipment.armor"),  armorForSlot(equipCfg, 0, eq.helmetTier)  },
        { "equipment.chest",  eq.chestTier,    eq.chestElement,    eq.chestElementPercent,
          loc.get("equipment.armor"),  armorForSlot(equipCfg, 1, eq.chestTier)   },
        { "equipment.legs",   eq.leggingsTier, eq.leggingsElement, eq.leggingsElementPercent,
          loc.get("equipment.armor"),  armorForSlot(equipCfg, 2, eq.leggingsTier) },
        { weaponKey,          eq.weaponTier,   eq.weaponElement,   eq.weaponElementPercent,
          loc.get("equipment.damage"), curDmg                                     },
    };

    for (int s = 0; s < 4; ++s) {
        const auto& si = slots[s];
        const float ry = ROW_Y[s];
        const bool empty = si.tier == 0;
        const sf::Color nameColor = empty ? sf::Color(110, 110, 110) : sf::Color::White;

        sf::Text nameTxt;
        nameTxt.setFont(font);
        nameTxt.setCharacterSize(24);
        nameTxt.setFillColor(nameColor);
        nameTxt.setString(toSfString(
            loc.get(si.nameKey) + (empty ? "  —" : "  T" + std::to_string(si.tier))));
        nameTxt.setPosition(NAME_COL, ry);
        window.draw(nameTxt);

        if (empty) continue;

        // Primary stat
        sf::Text stat1;
        stat1.setFont(font);
        stat1.setCharacterSize(20);
        stat1.setFillColor(sf::Color(200, 200, 200));
        stat1.setString(toSfString(si.primaryStatLabel + ": " + std::to_string(si.primaryStatValue)));
        stat1.setPosition(STAT_COL, ry);
        window.draw(stat1);

        // Element + percent
        const std::string elem = elementStr(loc, si.element, si.percent);
        if (!elem.empty()) {
            sf::Text elemTxt;
            elemTxt.setFont(font);
            elemTxt.setCharacterSize(20);
            elemTxt.setFillColor(elementColor(si.element));
            elemTxt.setString(toSfString(elem));
            elemTxt.setPosition(STAT_COL, ry + 28.0f);
            window.draw(elemTxt);

            // Bonus description
            const std::string bk = bonusKey(s, si.element);
            if (!bk.empty()) {
                sf::Text bonusTxt;
                bonusTxt.setFont(font);
                bonusTxt.setCharacterSize(16);
                bonusTxt.setFillColor(sf::Color(170, 170, 170));
                bonusTxt.setString(toSfString(loc.get(bk)));
                bonusTxt.setPosition(STAT_COL, ry + 56.0f);
                window.draw(bonusTxt);
            }
        }

        // Horizontal separator line (except after last slot)
        if (s < 3) {
            sf::RectangleShape sep({PANEL_W - 50.0f, 1.0f});
            sep.setPosition(PANEL_X + 25.0f, ry + 95.0f);
            sep.setFillColor(sf::Color(60, 60, 80));
            window.draw(sep);
        }
    }

    if (helpVisible) {
        drawHelpPanel(window, loc, font);
    }
}

InventoryScreen::Button InventoryScreen::getButtonAt(sf::Vector2f point) const
{
    if (helpBtn.getGlobalBounds().contains(point)) return Button::HELP;
    return Button::NONE;
}

void InventoryScreen::toggleHelp() { helpVisible = !helpVisible; }
void InventoryScreen::resetHelp()  { helpVisible = false; }

} // namespace tc
