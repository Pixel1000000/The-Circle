#pragma once

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <SFML/Graphics.hpp>

namespace tc::sprite {

// 8-way facing for character sprites. Distinct from tc::Facing (which only
// tracks a raw movement direction vector for melee aiming) - this one
// indexes directly into the spritesheet rows/frames baked by PixelLab, where
// every direction is a fully separate hand-drawn pose (no auto-mirroring of
// the body).
enum class Facing {
    South,
    SouthEast,
    East,
    NorthEast,
    North,
    NorthWest,
    West,
    SouthWest
};

// Parses a PixelLab direction string (e.g. "south-east") into a Facing.
// Throws std::invalid_argument on an unrecognized value.
Facing facingFromString(const std::string& s);

// Frame data for one (animation, direction) pair.
struct AnimationFrameData {
    std::vector<sf::IntRect> frames;
    float frameDuration = 0.1f;
    bool loop = true;
};

// animation name -> direction -> frame data
using AnimationTable = std::unordered_map<std::string, std::unordered_map<Facing, AnimationFrameData>>;

// A single loaded spritesheet (PNG texture + parsed animation/rotation
// layout). Shared between every entity that uses the same asset.
struct SpriteSheetAsset {
    sf::Texture texture;
    AnimationTable animations;
    // Static idle pose per direction, indexed by Facing.
    std::array<sf::IntRect, 8> idleRotationFrames{};
};

// Body appearance shared by every entity using the same base asset.
struct BaseAppearance {
    std::shared_ptr<SpriteSheetAsset> bodySheet;
};

enum class EquipSlot {
    Legs,
    Torso,
    Head,
    COUNT
};

// Paperdoll equipment layers drawn on top of the body, in slot order.
// nullptr = slot empty, skipped at render time.
struct EquipmentAppearance {
    std::array<std::shared_ptr<SpriteSheetAsset>, static_cast<std::size_t>(EquipSlot::COUNT)> layers{};
};

// Per-entity animation playback state.
struct AnimationState {
    std::string currentAnimation = "idle";
    Facing facing = Facing::South;
    int currentFrame = 0;
    float elapsedInFrame = 0.f;
    bool finished = false;
};

} // namespace tc::sprite
