#pragma once

#include <string>

#include <SFML/System/String.hpp>

namespace tc {

// Localization strings are loaded from JSON as UTF-8. sf::String's
// std::string constructor assumes the locale's ANSI encoding, which mangles
// Cyrillic and other non-Latin text, so this converts explicitly.
inline sf::String toSfString(const std::string& utf8)
{
    return sf::String::fromUtf8(utf8.begin(), utf8.end());
}

} // namespace tc
