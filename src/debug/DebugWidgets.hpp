#pragma once

#ifdef TC_DEBUG

#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

namespace tc {

// Clickable rectangle with a centered label.
struct DebugButton {
    void setup(const sf::Font& font, const std::string& labelStr, sf::Vector2f pos, sf::Vector2f size,
        sf::Color fillColor = sf::Color(60, 70, 90), unsigned int charSize = 16);

    bool isPressed(sf::Vector2f point) const;
    void render(sf::RenderWindow& window) const;

    sf::RectangleShape shape;
    sf::Text label;
};

// Horizontal drag slider over a numeric range.
struct DebugSlider {
    void setup(const sf::Font& font, const std::string& labelStr, sf::Vector2f pos, sf::Vector2f size,
        float minV, float maxV, float startValue, bool integerStep = false);

    void handleMousePressed(sf::Vector2f point);
    void handleMouseMoved(sf::Vector2f point);
    void handleMouseReleased();

    void setValue(float v);
    float getValue() const;

    void render(sf::RenderWindow& window) const;

    sf::RectangleShape track;
    sf::RectangleShape thumb;
    sf::Text label;
    sf::Text valueText;

    sf::Vector2f position;
    sf::Vector2f size;
    float minVal = 0.0f;
    float maxVal = 1.0f;
    float value = 0.0f;
    bool integerStep = false;
    bool dragging = false;

private:
    void updateThumbFromValue();
    void updateValueFromMouseX(float mouseX);
    void refreshValueText();
};

// Tickbox with a label.
struct DebugCheckbox {
    void setup(const sf::Font& font, const std::string& labelStr, sf::Vector2f pos, bool initialChecked = false);

    bool isClicked(sf::Vector2f point) const;
    void toggle();
    void render(sf::RenderWindow& window) const;

    sf::RectangleShape box;
    sf::RectangleShape mark;
    sf::Text label;
    bool checked = false;
};

// Simple expandable list. Click the header to expand/collapse, click an item to select it.
// Rendering is split in two so callers can draw every dropdown's header inline
// with the rest of a panel, then draw all expanded lists in one final pass -
// that keeps an open list on top of any widget/dropdown header positioned
// below it, regardless of draw order within the panel itself.
struct DebugDropdown {
    void setup(const sf::Font& font, sf::Vector2f pos, sf::Vector2f dropdownSize, unsigned int textCharSize = 14,
        int maxVisibleItems = 6);
    void setItems(const std::vector<std::string>& newItems);

    // Returns true if the click was consumed by this dropdown.
    bool handleClick(sf::Vector2f point);
    // Scrolls the open list when the wheel is used over it. Returns true if consumed.
    bool handleScroll(sf::Vector2f point, float delta);

    void renderHeader(sf::RenderWindow& window) const;
    void renderExpandedList(sf::RenderWindow& window) const;
    void render(sf::RenderWindow& window) const;

    const std::string& getSelected() const;
    int getSelectedIndex() const;

    sf::RectangleShape headerBox;
    sf::Text headerText;
    sf::RectangleShape listBackground;
    std::vector<sf::Text> itemTexts;
    std::vector<std::string> items;

    sf::Vector2f position;
    sf::Vector2f size;
    int selectedIndex = -1;
    bool expanded = false;
    int scrollOffset = 0;
    int maxVisibleItems = 6;

private:
    const sf::Font* font = nullptr;
    unsigned int charSize = 14;

    void refreshHeaderText();
    void refreshItemPositions();
};

} // namespace tc

#endif // TC_DEBUG
