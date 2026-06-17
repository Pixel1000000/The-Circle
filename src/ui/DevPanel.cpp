#include "ui/DevPanel.hpp"

#include <algorithm>
#include <string>

#include "core/TextUtils.hpp"

namespace tc {

namespace {

constexpr float PANEL_X     = 960.0f;
constexpr float PANEL_W     = 320.0f;
constexpr float PANEL_H     = 720.0f;
constexpr float TAB_Y       = 10.0f;
constexpr float TAB_H       = 36.0f;
constexpr float TAB_W       = PANEL_W / 3.0f;
constexpr float CONTENT_Y   = 56.0f;
constexpr float SPAWN_ROW_H = 28.0f;
constexpr float STAT_ROW_H  = 50.0f;
constexpr float HUD_ROW_H   = 44.0f;
constexpr float BTN_W       = 32.0f;
constexpr float BTN_H       = 26.0f;
constexpr int   VISIBLE_SPAWN_ROWS = static_cast<int>((PANEL_H - CONTENT_Y) / SPAWN_ROW_H);

bool hits(sf::Vector2f p, float x, float y, float w, float h)
{
    return p.x >= x && p.x < x + w && p.y >= y && p.y < y + h;
}

void drawBtn(sf::RenderWindow& window, float x, float y, float w, float h,
             sf::Color bg, const std::string& label, const sf::Font& font, unsigned sz = 15)
{
    sf::RectangleShape rect({w, h});
    rect.setPosition(x, y);
    rect.setFillColor(bg);
    window.draw(rect);

    sf::Text txt;
    txt.setFont(font);
    txt.setCharacterSize(sz);
    txt.setFillColor(sf::Color::White);
    txt.setString(toSfString(label));
    const auto b = txt.getLocalBounds();
    txt.setOrigin(b.left + b.width / 2.0f, b.top + b.height / 2.0f);
    txt.setPosition(x + w / 2.0f, y + h / 2.0f);
    window.draw(txt);
}

void drawTxt(sf::RenderWindow& window, const std::string& str,
             float x, float y, const sf::Font& font,
             unsigned sz = 14, sf::Color color = sf::Color::White)
{
    sf::Text txt;
    txt.setFont(font);
    txt.setCharacterSize(sz);
    txt.setFillColor(color);
    txt.setString(toSfString(str));
    txt.setPosition(x, y);
    window.draw(txt);
}

} // namespace

// ── Input ─────────────────────────────────────────────────────────────────────

bool DevPanel::handleClick(sf::Vector2f pos)
{
    if (pos.x < PANEL_X) return false;

    // Tab row
    if (hits(pos, PANEL_X,              TAB_Y, TAB_W, TAB_H)) { activeTab = Tab::SPAWN;  scrollOffset = 0; return true; }
    if (hits(pos, PANEL_X + TAB_W,      TAB_Y, TAB_W, TAB_H)) { activeTab = Tab::PLAYER; return true; }
    if (hits(pos, PANEL_X + TAB_W * 2,  TAB_Y, TAB_W, TAB_H)) { activeTab = Tab::HUD;    return true; }

    if (pos.y < CONTENT_Y) return true;

    if (activeTab == Tab::SPAWN) {
        const int row = static_cast<int>((pos.y - CONTENT_Y) / SPAWN_ROW_H);
        if (row >= 0 && row < VISIBLE_SPAWN_ROWS) {
            const int idx = row + scrollOffset;
            if (idx < cachedEnemyCount) {
                spawnPending = true;
                spawnIsBoss  = false;
                spawnIndex   = idx;
            } else if (idx > cachedEnemyCount) {
                const int bossIdx = idx - cachedEnemyCount - 1;
                if (bossIdx >= 0 && bossIdx < cachedBossCount) {
                    spawnPending = true;
                    spawnIsBoss  = true;
                    spawnIndex   = bossIdx;
                }
            }
            // idx == cachedEnemyCount is the separator — no action
        }
        return true;
    }

    if (activeTab == Tab::PLAYER) {
        // Rows 0-4: stat sliders
        for (int i = 0; i < 5; ++i) {
            const float rowY = CONTENT_Y + i * STAT_ROW_H;
            const float btnY = rowY + (STAT_ROW_H - BTN_H) / 2.0f;
            if (pos.y < rowY || pos.y >= rowY + STAT_ROW_H) continue;
            if (hits(pos, PANEL_X + 5,                    btnY, BTN_W, BTN_H)) { applyStatDelta(i, -1); return true; }
            if (hits(pos, PANEL_X + PANEL_W - 5 - BTN_W,  btnY, BTN_W, BTN_H)) { applyStatDelta(i, +1); return true; }
            return true;
        }
        // Row 5: invincibility toggle
        const float invY = CONTENT_Y + 5 * STAT_ROW_H;
        if (pos.y >= invY && pos.y < invY + STAT_ROW_H)
            playerInvincible = !playerInvincible;
        return true;
    }

    if (activeTab == Tab::HUD) {
        bool* flags[] = {&showHudHealth, &showHudFragments, &showHudPotions, &showHudStatusIcons};
        for (int i = 0; i < 4; ++i) {
            const float rowY = CONTENT_Y + i * HUD_ROW_H;
            if (pos.y >= rowY && pos.y < rowY + HUD_ROW_H) {
                *flags[i] = !*flags[i];
                return true;
            }
        }
    }

    return true;
}

void DevPanel::handleScroll(float delta)
{
    if (activeTab != Tab::SPAWN) return;
    scrollOffset -= static_cast<int>(delta);
    const int total  = cachedEnemyCount + 1 + cachedBossCount;
    const int maxOff = std::max(0, total - VISIBLE_SPAWN_ROWS);
    scrollOffset = std::clamp(scrollOffset, 0, maxOff);
}

void DevPanel::applyStatDelta(int statIdx, int dir)
{
    switch (statIdx) {
    case 0: healthDelta    += dir;          break;
    case 1: maxHealthDelta += dir;          break;
    case 2: damageDelta    += dir;          break;
    case 3: armorDelta     += dir;          break;
    case 4: speedDelta     += dir * 10.0f; break;
    default: break;
    }
}

// ── Render ────────────────────────────────────────────────────────────────────

void DevPanel::render(sf::RenderWindow& window, const FontManager& fontManager,
                      const Localization& localization, const EnemyConfig& enemyConfig,
                      const PlayerStats& stats)
{
    cachedEnemyCount = static_cast<int>(enemyConfig.enemies.size());
    cachedBossCount  = static_cast<int>(enemyConfig.bosses.size());

    // Background
    sf::RectangleShape bg({PANEL_W, PANEL_H});
    bg.setPosition(PANEL_X, 0.0f);
    bg.setFillColor(sf::Color(20, 20, 30, 220));
    window.draw(bg);

    if (!fontManager.isLoaded()) return;
    const sf::Font& font = fontManager.getFont(localization.getCurrentLanguage());

    // ── Tabs ─────────────────────────────────────────────────────────────────
    const char* tabLabels[] = {"SPAWN", "PLAYER", "HUD"};
    for (int i = 0; i < 3; ++i) {
        const bool active = (activeTab == static_cast<Tab>(i));
        drawBtn(window, PANEL_X + i * TAB_W, TAB_Y, TAB_W - 2, TAB_H,
                active ? sf::Color(70, 70, 150) : sf::Color(40, 40, 70),
                tabLabels[i], font, 14);
    }

    // ── SPAWN tab ─────────────────────────────────────────────────────────────
    if (activeTab == Tab::SPAWN) {
        const int total = cachedEnemyCount + 1 + cachedBossCount;
        for (int i = scrollOffset; i < scrollOffset + VISIBLE_SPAWN_ROWS && i < total; ++i) {
            const float rowY       = CONTENT_Y + (i - scrollOffset) * SPAWN_ROW_H;
            const bool isSeparator = (i == cachedEnemyCount);
            const bool isBoss      = (i > cachedEnemyCount);

            if (isSeparator) {
                drawTxt(window, "--- BOSSES ---", PANEL_X + 8, rowY + 6, font, 12,
                        sf::Color(180, 150, 255));
                continue;
            }

            const std::string& id = isBoss
                ? enemyConfig.bosses[i - cachedEnemyCount - 1].id
                : enemyConfig.enemies[i].id;

            sf::RectangleShape row({PANEL_W - 4, SPAWN_ROW_H - 2});
            row.setPosition(PANEL_X + 2, rowY);
            row.setFillColor(isBoss ? sf::Color(50, 30, 70, 180) : sf::Color(25, 50, 25, 180));
            window.draw(row);

            drawTxt(window, id, PANEL_X + 8, rowY + 5, font, 13);
        }
        drawTxt(window, "scroll to browse", PANEL_X + 8, PANEL_H - 20, font, 11,
                sf::Color(110, 110, 110));
    }

    // ── PLAYER tab ────────────────────────────────────────────────────────────
    else if (activeTab == Tab::PLAYER) {
        struct Row { const char* label; int value; };
        const Row rows[] = {
            {"HP",      stats.health},
            {"Max HP",  stats.maxHealth},
            {"Damage",  stats.damage},
            {"Armor",   stats.armor},
            {"Speed",   static_cast<int>(stats.speed)},
        };

        for (int i = 0; i < 5; ++i) {
            const float rowY = CONTENT_Y + i * STAT_ROW_H;
            const float btnY = rowY + (STAT_ROW_H - BTN_H) / 2.0f;

            drawTxt(window, rows[i].label,
                    PANEL_X + BTN_W + 10, rowY + (STAT_ROW_H - 16) / 2.0f, font, 14);
            drawTxt(window, std::to_string(rows[i].value),
                    PANEL_X + PANEL_W / 2.0f - 15, rowY + (STAT_ROW_H - 16) / 2.0f, font, 15);

            drawBtn(window, PANEL_X + 5,                   btnY, BTN_W, BTN_H,
                    sf::Color(130, 50, 50),  "-", font, 18);
            drawBtn(window, PANEL_X + PANEL_W - 5 - BTN_W, btnY, BTN_W, BTN_H,
                    sf::Color(50, 130, 50), "+", font, 18);
        }

        // Invincibility toggle
        const float invY = CONTENT_Y + 5 * STAT_ROW_H;
        drawBtn(window, PANEL_X + 5, invY + 8, PANEL_W - 10, BTN_H + 4,
                playerInvincible ? sf::Color(180, 140, 0) : sf::Color(60, 60, 85),
                playerInvincible ? "INVINCIBLE: ON" : "INVINCIBLE: OFF", font, 14);
    }

    // ── HUD tab ───────────────────────────────────────────────────────────────
    else if (activeTab == Tab::HUD) {
        struct HudRow { const char* label; bool on; };
        const HudRow rows[] = {
            {"Health bar",    showHudHealth},
            {"Key Fragments", showHudFragments},
            {"Potions",       showHudPotions},
            {"Status Icons",  showHudStatusIcons},
        };
        for (int i = 0; i < 4; ++i) {
            const float rowY = CONTENT_Y + i * HUD_ROW_H;

            sf::RectangleShape row({PANEL_W - 4, HUD_ROW_H - 4});
            row.setPosition(PANEL_X + 2, rowY + 2);
            row.setFillColor(rows[i].on ? sf::Color(35, 90, 35, 200) : sf::Color(80, 35, 35, 200));
            window.draw(row);

            drawTxt(window, rows[i].label, PANEL_X + 12, rowY + 12, font, 15);
            drawTxt(window, rows[i].on ? "ON" : "OFF",
                    PANEL_X + PANEL_W - 48, rowY + 12, font, 15,
                    rows[i].on ? sf::Color(100, 255, 100) : sf::Color(255, 100, 100));
        }
    }
}

} // namespace tc
