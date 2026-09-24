#pragma once

#include <vector>

#include <SFML/Graphics.hpp>

namespace tc::sprite {

// Dev-time asset check: returns every distinct pixel color in `image` that
// is not in `palette` (e.g. DB32), so an off-palette import can be caught
// before it reaches the build. Ignores fully transparent pixels (alpha ==
// 0), since PixelLab exports use transparent padding around the sprite.
// Not called at runtime - see the CLI tool / test that runs it over
// assets/.
std::vector<sf::Color> findOffPaletteColors(const sf::Image& image, const std::vector<sf::Color>& palette);

} // namespace tc::sprite
