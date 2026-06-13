#pragma once

#include <SFML/Graphics.hpp>
#include <string>

namespace tc {

// Loads the Latin and Cyrillic UI fonts and picks the right one for the
// active language, since a single font does not cover both alphabets.
class FontManager {
public:
    void load();

    const sf::Font& getFont(const std::string& languageCode) const;
    bool isLoaded() const;

private:
    sf::Font latinFont;
    sf::Font cyrillicFont;
    bool latinLoaded = false;
    bool cyrillicLoaded = false;
};

} // namespace tc
