#include <doctest/doctest.h>

#include "sprite/PaletteValidator.hpp"

using namespace tc::sprite;

TEST_CASE("findOffPaletteColors returns only distinct colors absent from the palette")
{
    sf::Image image;
    image.create(2, 2, sf::Color::Black);

    const sf::Color inPalette(10, 20, 30);
    const sf::Color offPalette(200, 50, 5);
    const sf::Color transparentJunk(1, 2, 3, 0);

    image.setPixel(0, 0, inPalette);
    image.setPixel(1, 0, offPalette);
    image.setPixel(0, 1, offPalette); // duplicate off-palette pixel
    image.setPixel(1, 1, transparentJunk);

    const std::vector<sf::Color> palette = {inPalette, sf::Color::White};

    const auto result = findOffPaletteColors(image, palette);

    REQUIRE(result.size() == 1);
    CHECK(result[0] == offPalette);
}

TEST_CASE("findOffPaletteColors returns empty when every opaque pixel is in the palette")
{
    sf::Image image;
    image.create(2, 2, sf::Color::White);

    const std::vector<sf::Color> palette = {sf::Color::White};

    CHECK(findOffPaletteColors(image, palette).empty());
}
