#pragma once

#include <unordered_map>
#include <vector>

#include <SFML/Graphics.hpp>

#include "sprite/SpriteTypes.hpp"

namespace tc::sprite {

// One keyframe of a weapon's attack curve: where the blade sits relative to
// the hand socket on a given body frame, and whether it draws in front of
// or behind the body on that frame.
struct SocketKeyframe {
    int frameIndex = 0;
    sf::Vector2f offset;
    float rotationDeg = 0.f;
    bool drawInFrontOfBody = true;
};

// Attack curve authored for 5 directions; west/south-west/north-west are
// derived by mirroring east/south-east/north-east (see resolveMirror).
struct WeaponAttackCurve {
    std::vector<SocketKeyframe> south;
    std::vector<SocketKeyframe> north;
    std::vector<SocketKeyframe> east;
    std::vector<SocketKeyframe> southEast;
    std::vector<SocketKeyframe> northEast;
};

struct WeaponItem {
    std::shared_ptr<WeaponAttackCurve> curve;
    sf::Texture bladeTexture;
};

// Per-frame hand attach point, authored for the same 5 directions as
// WeaponAttackCurve (the mirrored west-side directions reuse these too).
struct HandSocketData {
    std::unordered_map<Facing, std::vector<sf::Vector2f>> socketPerFrame;
};

// Attached to an entity that is currently wielding a weapon.
struct WeaponAppearance {
    std::shared_ptr<WeaponItem> weapon;
    std::shared_ptr<HandSocketData> handSockets;
};

// Which authored (non-mirrored) direction a given Facing resolves to, and
// whether the curve/socket/sprite need to be mirrored to get there. The body
// itself is never mirrored (every direction is separate PixelLab art) - only
// the weapon's socket curve is, since it wasn't authored for the west side.
struct FacingMirror {
    Facing source;
    bool mirrored;
};

FacingMirror resolveMirror(Facing f);

// Returns the curve's keyframe list for the curve's own authored direction
// (South/North/East/SouthEast/NorthEast only - west-side directions must be
// resolved via resolveMirror first).
const std::vector<SocketKeyframe>& curveForDirection(const WeaponAttackCurve& curve, Facing authoredDirection);

} // namespace tc::sprite
