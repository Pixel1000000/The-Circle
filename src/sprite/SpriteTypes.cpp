#include "sprite/SpriteTypes.hpp"

#include <stdexcept>
#include <unordered_map>

namespace tc::sprite {

Facing facingFromString(const std::string& s)
{
    static const std::unordered_map<std::string, Facing> table = {
        {"south", Facing::South},
        {"south-east", Facing::SouthEast},
        {"east", Facing::East},
        {"north-east", Facing::NorthEast},
        {"north", Facing::North},
        {"north-west", Facing::NorthWest},
        {"west", Facing::West},
        {"south-west", Facing::SouthWest},
    };

    const auto it = table.find(s);
    if (it == table.end()) {
        throw std::invalid_argument("[sprite] Unknown facing string: " + s);
    }
    return it->second;
}

} // namespace tc::sprite
