#include "sprite/WeaponTypes.hpp"

#include <stdexcept>

namespace tc::sprite {

FacingMirror resolveMirror(Facing f)
{
    switch (f) {
        case Facing::West:
            return {Facing::East, true};
        case Facing::SouthWest:
            return {Facing::SouthEast, true};
        case Facing::NorthWest:
            return {Facing::NorthEast, true};
        default:
            return {f, false};
    }
}

const std::vector<SocketKeyframe>& curveForDirection(const WeaponAttackCurve& curve, Facing authoredDirection)
{
    switch (authoredDirection) {
        case Facing::South:
            return curve.south;
        case Facing::North:
            return curve.north;
        case Facing::East:
            return curve.east;
        case Facing::SouthEast:
            return curve.southEast;
        case Facing::NorthEast:
            return curve.northEast;
        default:
            throw std::invalid_argument("[sprite] WeaponAttackCurve has no authored data for this direction");
    }
}

} // namespace tc::sprite
