#include "debug/DebugWidgets.hpp"

#ifdef TC_DEBUG

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace tc {

namespace {
constexpr float THUMB_WIDTH = 10.0f;
constexpr float CHECKBOX_SIZE = 18.0f;
constexpr float DROPDOWN_ITEM_HEIGHT = 22.0f;

void centerInRect(sf::Text& text, sf::Vector2f rectPos, sf::Vector2f rectSize)
{
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
    text.setPosition(rectPos.x + rectSize.x / 2.0f, rectPos.y + rectSize.y / 2.0f);
}
} // namespace

// ---------------------------------------------------------------- Button --

void DebugButton::setup(const sf::Font& font, const std::string& labelStr, sf::Vector2f pos, sf::Vector2f size,
    sf::Color fillColor, unsigned int charSize)
{
    shape.setSize(size);
    shape.setPosition(pos);
    shape.setFillColor(fillColor);
    shape.setOutlineColor(sf::Color(120, 130, 150));
    shape.setOutlineThickness(1.0f);

    label.setFont(font);
    label.setCharacterSize(charSize);
    label.setFillColor(sf::Color::White);
    label.setString(labelStr);
    centerInRect(label, pos, size);
}

bool DebugButton::isPressed(sf::Vector2f point) const
{
    return shape.getGlobalBounds().contains(point);
}

void DebugButton::render(sf::RenderWindow& window) const
{
    window.draw(shape);
    window.draw(label);
}

// ---------------------------------------------------------------- Slider --

void DebugSlider::setup(const sf::Font& font, const std::string& labelStr, sf::Vector2f pos, sf::Vector2f sliderSize,
    float minV, float maxV, float startValue, bool isIntegerStep)
{
    position = pos;
    size = sliderSize;
    minVal = minV;
    maxVal = maxV;
    integerStep = isIntegerStep;
    value = std::clamp(startValue, minVal, maxVal);

    track.setSize(size);
    track.setPosition(position);
    track.setFillColor(sf::Color(50, 50, 60));
    track.setOutlineColor(sf::Color(120, 130, 150));
    track.setOutlineThickness(1.0f);

    thumb.setSize({THUMB_WIDTH, size.y});
    thumb.setFillColor(sf::Color::White);

    label.setFont(font);
    label.setCharacterSize(14);
    label.setFillColor(sf::Color::White);
    label.setString(labelStr);
    label.setPosition(position.x, position.y - 18.0f);

    valueText.setFont(font);
    valueText.setCharacterSize(14);
    valueText.setFillColor(sf::Color::White);

    updateThumbFromValue();
    refreshValueText();
}

void DebugSlider::updateThumbFromValue()
{
    const float ratio = (maxVal > minVal) ? (value - minVal) / (maxVal - minVal) : 0.0f;
    const float thumbX = position.x + ratio * (size.x - THUMB_WIDTH);
    thumb.setPosition(thumbX, position.y);
}

void DebugSlider::updateValueFromMouseX(float mouseX)
{
    const float ratio = std::clamp((mouseX - position.x) / size.x, 0.0f, 1.0f);
    float newValue = minVal + ratio * (maxVal - minVal);
    if (integerStep) {
        newValue = std::round(newValue);
    }
    setValue(newValue);
}

void DebugSlider::refreshValueText()
{
    char buffer[32];
    if (integerStep) {
        std::snprintf(buffer, sizeof(buffer), "%d", static_cast<int>(std::round(value)));
    } else {
        std::snprintf(buffer, sizeof(buffer), "%.2f", value);
    }
    valueText.setString(buffer);
    valueText.setPosition(position.x + size.x + 8.0f, position.y - 2.0f);
}

void DebugSlider::handleMousePressed(sf::Vector2f point)
{
    const sf::FloatRect hitArea(position.x, position.y, size.x, size.y);
    if (hitArea.contains(point)) {
        dragging = true;
        updateValueFromMouseX(point.x);
    }
}

void DebugSlider::handleMouseMoved(sf::Vector2f point)
{
    if (dragging) {
        updateValueFromMouseX(point.x);
    }
}

void DebugSlider::handleMouseReleased()
{
    dragging = false;
}

void DebugSlider::setValue(float v)
{
    value = std::clamp(v, minVal, maxVal);
    updateThumbFromValue();
    refreshValueText();
}

float DebugSlider::getValue() const
{
    return value;
}

void DebugSlider::render(sf::RenderWindow& window) const
{
    window.draw(label);
    window.draw(track);
    window.draw(thumb);
    window.draw(valueText);
}

// -------------------------------------------------------------- Checkbox --

void DebugCheckbox::setup(const sf::Font& font, const std::string& labelStr, sf::Vector2f pos, bool initialChecked)
{
    checked = initialChecked;

    box.setSize({CHECKBOX_SIZE, CHECKBOX_SIZE});
    box.setPosition(pos);
    box.setFillColor(sf::Color(50, 50, 60));
    box.setOutlineColor(sf::Color(120, 130, 150));
    box.setOutlineThickness(1.0f);

    mark.setSize({CHECKBOX_SIZE - 6.0f, CHECKBOX_SIZE - 6.0f});
    mark.setPosition(pos.x + 3.0f, pos.y + 3.0f);
    mark.setFillColor(sf::Color(90, 200, 110));

    label.setFont(font);
    label.setCharacterSize(14);
    label.setFillColor(sf::Color::White);
    label.setString(labelStr);
    label.setPosition(pos.x + CHECKBOX_SIZE + 8.0f, pos.y);
}

bool DebugCheckbox::isClicked(sf::Vector2f point) const
{
    const sf::FloatRect labelBounds = label.getGlobalBounds();
    return box.getGlobalBounds().contains(point) || labelBounds.contains(point);
}

void DebugCheckbox::toggle()
{
    checked = !checked;
}

void DebugCheckbox::render(sf::RenderWindow& window) const
{
    window.draw(box);
    if (checked) {
        window.draw(mark);
    }
    window.draw(label);
}

// -------------------------------------------------------------- Dropdown --

void DebugDropdown::setup(const sf::Font& fontRef, sf::Vector2f pos, sf::Vector2f dropdownSize, unsigned int textCharSize)
{
    font = &fontRef;
    position = pos;
    size = dropdownSize;
    charSize = textCharSize;

    headerBox.setSize(dropdownSize);
    headerBox.setPosition(position);
    headerBox.setFillColor(sf::Color(50, 50, 60));
    headerBox.setOutlineColor(sf::Color(120, 130, 150));
    headerBox.setOutlineThickness(1.0f);

    headerText.setFont(fontRef);
    headerText.setCharacterSize(charSize);
    headerText.setFillColor(sf::Color::White);

    listBackground.setFillColor(sf::Color(35, 35, 45));
    listBackground.setOutlineColor(sf::Color(120, 130, 150));
    listBackground.setOutlineThickness(1.0f);
}

void DebugDropdown::setItems(const std::vector<std::string>& newItems)
{
    items = newItems;
    selectedIndex = items.empty() ? -1 : 0;
    expanded = false;

    itemTexts.clear();
    if (!font) return;

    for (std::size_t i = 0; i < items.size(); ++i) {
        sf::Text text;
        text.setFont(*font);
        text.setCharacterSize(charSize);
        text.setFillColor(sf::Color::White);
        text.setString(items[i]);
        text.setPosition(position.x + 6.0f, position.y + size.y + static_cast<float>(i) * DROPDOWN_ITEM_HEIGHT + 3.0f);
        itemTexts.push_back(text);
    }

    listBackground.setSize({size.x, static_cast<float>(items.size()) * DROPDOWN_ITEM_HEIGHT});
    listBackground.setPosition(position.x, position.y + size.y);

    refreshHeaderText();
}

void DebugDropdown::refreshHeaderText()
{
    headerText.setString(getSelected());
    headerText.setPosition(position.x + 6.0f, position.y + 3.0f);
}

bool DebugDropdown::handleClick(sf::Vector2f point)
{
    if (headerBox.getGlobalBounds().contains(point)) {
        expanded = !expanded;
        return true;
    }

    if (expanded) {
        if (listBackground.getGlobalBounds().contains(point)) {
            const int index = static_cast<int>((point.y - (position.y + size.y)) / DROPDOWN_ITEM_HEIGHT);
            if (index >= 0 && index < static_cast<int>(items.size())) {
                selectedIndex = index;
                refreshHeaderText();
            }
            expanded = false;
            return true;
        }
        expanded = false;
        return true;
    }

    return false;
}

void DebugDropdown::render(sf::RenderWindow& window) const
{
    window.draw(headerBox);
    window.draw(headerText);

    if (expanded) {
        window.draw(listBackground);
        for (const auto& text : itemTexts) {
            window.draw(text);
        }
    }
}

const std::string& DebugDropdown::getSelected() const
{
    static const std::string empty;
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(items.size())) return empty;
    return items[static_cast<std::size_t>(selectedIndex)];
}

int DebugDropdown::getSelectedIndex() const
{
    return selectedIndex;
}

} // namespace tc

#endif // TC_DEBUG
