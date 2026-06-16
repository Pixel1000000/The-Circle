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

void drawHelpPanel(sf::RenderWindow& window, const Localization& loc, const sf::Font& font)
{
    sf::RectangleShape bg({HP_W, HP_H});
    bg.setPosition(HP_X, HP_Y);
    bg.setFillColor(sf::Color(15, 15, 25, 245));
    bg.setOutlineThickness(2.0f);
    bg.setOutlineColor(sf::Color(100, 100, 140));
    window.draw(bg);

    auto drawLine = [&](const std::string& text, float x, float y, unsigned int size,
                        sf::Color color = sf::Color::White) {
        sf::Text t;
        t.setFont(font);
        t.setCharacterSize(size);
        t.setFillColor(color);
        t.setString(toSfString(text));
        t.setPosition(x, y);
        window.draw(t);
    };

    drawLine(loc.get("help.title"), HP_X + 20.0f, HP_Y + 14.0f, 26);

    // Column headers (elements)
    const float colX[4] = { HP_X + 260.0f, HP_X + 440.0f, HP_X + 610.0f, HP_X + 760.0f };
    drawLine(loc.get("element.nature"), colX[0], HP_Y + 56.0f, 18, elementColor(Element::NATURE));
    drawLine(loc.get("element.fire"),   colX[1], HP_Y + 56.0f, 18, elementColor(Element::FIRE));
    drawLine(loc.get("element.ice"),    colX[2], HP_Y + 56.0f, 18, elementColor(Element::ICE));
    drawLine(loc.get("element.decay"),  colX[3], HP_Y + 56.0f, 18, elementColor(Element::DECAY));

    // Row headers and descriptions
    struct Row { const char* headerKey; const char* bKeys[4]; };
    const Row rows[3] = {
        { "help.weapon",   { "bonus.weapon.nature",   "bonus.weapon.fire",   "bonus.weapon.ice",   "bonus.weapon.decay"   } },
        { "help.armor",    { "bonus.armor.nature",    "bonus.armor.fire",    "bonus.armor.ice",    "bonus.armor.decay"    } },
        { "help.leggings", { "bonus.leggings.nature", "bonus.leggings.fire", "bonus.leggings.ice", "bonus.leggings.decay" } },
    };

    const float rowYs[3] = { HP_Y + 100.0f, HP_Y + 250.0f, HP_Y + 390.0f };

    for (int r = 0; r < 3; ++r) {
        drawLine(loc.get(rows[r].headerKey), HP_X + 20.0f, rowYs[r], 20, sf::Color(220, 220, 140));

        for (int c = 0; c < 4; ++c) {
            // Word-wrap: split at 22 chars naively by inserting newlines in the text
            const std::string desc = loc.get(rows[r].bKeys[c]);
            // Draw at most 2 lines (split at space near char 22)
            std::string line1 = desc, line2;
            if (desc.size() > 22) {
                const std::size_t split = desc.rfind(' ', 22);
                if (split != std::string::npos) {
                    line1 = desc.substr(0, split);
                    line2 = desc.substr(split + 1);
                }
            }
            drawLine(line1, colX[c], rowYs[r], 16, sf::Color(210, 210, 210));
            if (!line2.empty()) {
                drawLine(line2, colX[c], rowYs[r] + 20.0f, 16, sf::Color(210, 210, 210));
            }
        }
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
