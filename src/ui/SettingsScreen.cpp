#include "ui/SettingsScreen.hpp"

#include "core/TextUtils.hpp"

namespace tc {

namespace {

constexpr float PANEL_W = 480.0f;
constexpr float PANEL_H = 340.0f;
constexpr float PANEL_X = (1280.0f - PANEL_W) / 2.0f;
constexpr float PANEL_Y = (720.0f  - PANEL_H) / 2.0f;

constexpr float BTN_W  = 100.0f;
constexpr float BTN_H  = 36.0f;
constexpr float STEP_W = 40.0f;

} // namespace

SettingsScreen::SettingsScreen()
{
    overlay.setSize({1280.0f, 720.0f});
    overlay.setFillColor(sf::Color(0, 0, 0, 160));

    panel.setSize({PANEL_W, PANEL_H});
    panel.setPosition(PANEL_X, PANEL_Y);
    panel.setFillColor(sf::Color(12, 12, 24, 240));
    panel.setOutlineThickness(2.0f);
    panel.setOutlineColor(sf::Color(90, 90, 140));

    auto initBtn = [](sf::RectangleShape& b, float x, float y, float w, float h) {
        b.setSize({w, h});
        b.setPosition(x, y);
        b.setFillColor(sf::Color(50, 50, 80));
        b.setOutlineThickness(1.5f);
        b.setOutlineColor(sf::Color(120, 120, 180));
    };

    const float col = PANEL_X + PANEL_W - BTN_W - 20.0f;

    initBtn(langRuBtn,    PANEL_X + 20.0f,  PANEL_Y + 60.0f,  BTN_W, BTN_H);
    initBtn(langEnBtn,    PANEL_X + 140.0f, PANEL_Y + 60.0f,  BTN_W, BTN_H);

    initBtn(musicMinusBtn, PANEL_X + 20.0f,  PANEL_Y + 150.0f, STEP_W, BTN_H);
    initBtn(musicPlusBtn,  PANEL_X + 200.0f, PANEL_Y + 150.0f, STEP_W, BTN_H);

    initBtn(sfxMinusBtn,   PANEL_X + 20.0f,  PANEL_Y + 220.0f, STEP_W, BTN_H);
    initBtn(sfxPlusBtn,    PANEL_X + 200.0f, PANEL_Y + 220.0f, STEP_W, BTN_H);

    initBtn(backBtn,       PANEL_X + 20.0f,  PANEL_Y + PANEL_H - BTN_H - 20.0f, BTN_W, BTN_H);
}

void SettingsScreen::render(sf::RenderWindow& window, const Localization& loc,
                            const FontManager& fontManager, const AudioManager& audio)
{
    window.draw(overlay);
    window.draw(panel);
    window.draw(langRuBtn);
    window.draw(langEnBtn);
    window.draw(musicMinusBtn);
    window.draw(musicPlusBtn);
    window.draw(sfxMinusBtn);
    window.draw(sfxPlusBtn);
    window.draw(backBtn);

    if (!fontManager.isLoaded()) return;
    const sf::Font& font = fontManager.getFont(loc.getCurrentLanguage());

    auto label = [&](const std::string& str, float x, float y, unsigned sz, sf::Color col) {
        sf::Text t;
        t.setFont(font);
        t.setCharacterSize(sz);
        t.setFillColor(col);
        t.setString(toSfString(str));
        t.setPosition(x, y);
        window.draw(t);
    };

    auto btnLabel = [&](const sf::RectangleShape& btn, const std::string& str, unsigned sz = 18) {
        sf::Text t;
        t.setFont(font);
        t.setCharacterSize(sz);
        t.setFillColor(sf::Color::White);
        t.setString(toSfString(str));
        const auto bounds = t.getLocalBounds();
        t.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
        const auto& pos = btn.getPosition();
        const auto& sz2 = btn.getSize();
        t.setPosition(pos.x + sz2.x / 2.0f, pos.y + sz2.y / 2.0f);
        window.draw(t);
    };

    label(loc.get("settings.title"), PANEL_X + 20.0f, PANEL_Y + 16.0f, 22, sf::Color(220, 220, 255));

    label(loc.get("settings.language"), PANEL_X + 20.0f, PANEL_Y + 36.0f, 16, sf::Color(180, 180, 180));
    const bool isRu = loc.getCurrentLanguage() == "ru";
    langRuBtn.setFillColor(isRu ? sf::Color(70, 70, 120) : sf::Color(40, 40, 60));
    langEnBtn.setFillColor(!isRu ? sf::Color(70, 70, 120) : sf::Color(40, 40, 60));
    btnLabel(langRuBtn, "RU");
    btnLabel(langEnBtn, "EN");

    label(loc.get("settings.music"), PANEL_X + 20.0f, PANEL_Y + 126.0f, 16, sf::Color(180, 180, 180));
    btnLabel(musicMinusBtn, "-", 22);
    const int mvol = static_cast<int>(audio.getMusicVolume());
    label(std::to_string(mvol) + "%", PANEL_X + 70.0f, PANEL_Y + 154.0f, 18, sf::Color::White);
    btnLabel(musicPlusBtn, "+", 22);

    label(loc.get("settings.sfx"), PANEL_X + 20.0f, PANEL_Y + 196.0f, 16, sf::Color(180, 180, 180));
    btnLabel(sfxMinusBtn, "-", 22);
    const int svol = static_cast<int>(audio.getSfxVolume());
    label(std::to_string(svol) + "%", PANEL_X + 70.0f, PANEL_Y + 224.0f, 18, sf::Color::White);
    btnLabel(sfxPlusBtn, "+", 22);

    btnLabel(backBtn, loc.get("settings.back"), 16);
}

SettingsScreen::Button SettingsScreen::getButtonAt(sf::Vector2f point) const
{
    if (backBtn.getGlobalBounds().contains(point))       return Button::BACK;
    if (langRuBtn.getGlobalBounds().contains(point))     return Button::LANG_RU;
    if (langEnBtn.getGlobalBounds().contains(point))     return Button::LANG_EN;
    if (musicMinusBtn.getGlobalBounds().contains(point)) return Button::MUSIC_MINUS;
    if (musicPlusBtn.getGlobalBounds().contains(point))  return Button::MUSIC_PLUS;
    if (sfxMinusBtn.getGlobalBounds().contains(point))   return Button::SFX_MINUS;
    if (sfxPlusBtn.getGlobalBounds().contains(point))    return Button::SFX_PLUS;
    return Button::NONE;
}

} // namespace tc
