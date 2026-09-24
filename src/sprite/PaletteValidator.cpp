#include "sprite/PaletteValidator.hpp"

#include <algorithm>

namespace tc::sprite {

std::vector<sf::Color> findOffPaletteColors(const sf::Image& image, const std::vector<sf::Color>& palette)
{
    std::vector<sf::Color> offPalette;

    const sf::Vector2u size = image.getSize();
    for (unsigned int y = 0; y < size.y; ++y) {
        for (unsigned int x = 0; x < size.x; ++x) {
            const sf::Color pixel = image.getPixel(x, y);
            if (pixel.a == 0) {
                continue; // transparent padding, not a real color
            }
            if (std::find(palette.begin(), palette.end(), pixel) != palette.end()) {
                continue;
            }
            if (std::find(offPalette.begin(), offPalette.end(), pixel) == offPalette.end()) {
                offPalette.push_back(pixel);
            }
        }
    }

    return offPalette;
}

} // namespace tc::sprite
