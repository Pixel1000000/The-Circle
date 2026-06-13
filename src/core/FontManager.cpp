#include "core/FontManager.hpp"

#include <iostream>

namespace tc {

void FontManager::load()
{
    latinLoaded = latinFont.loadFromFile("assets/fonts/latin.ttf");
    if (!latinLoaded) {
        std::cerr << "[FontManager] Failed to load assets/fonts/latin.ttf\n";
    }

    cyrillicLoaded = cyrillicFont.loadFromFile("assets/fonts/cyrillic.ttf");
    if (!cyrillicLoaded) {
        std::cerr << "[FontManager] Failed to load assets/fonts/cyrillic.ttf\n";
    }
}

const sf::Font& FontManager::getFont(const std::string& languageCode) const
{
    if (languageCode == "ru" && cyrillicLoaded) {
        return cyrillicFont;
    }
    if (latinLoaded) {
        return latinFont;
    }
    return cyrillicFont;
}

bool FontManager::isLoaded() const
{
    return latinLoaded || cyrillicLoaded;
}

} // namespace tc
