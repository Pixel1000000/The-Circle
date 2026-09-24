#include <doctest/doctest.h>

#include <fstream>
#include <stdexcept>

#include "sprite/SpriteSheetLoader.hpp"

using namespace tc::sprite;

namespace {
const char* kPng = "assets/characters/dead_swordsman/dead_swordsman_Idle.png";
const char* kJson = "assets/characters/dead_swordsman/dead_swordsman_Idle.json";
} // namespace

TEST_CASE("SpriteSheetLoader parses idle rotations from real PixelLab JSON")
{
    SpriteSheetLoader::clearCache();
    auto asset = SpriteSheetLoader::load(kPng, kJson);
    REQUIRE(asset != nullptr);

    // Row 0 lists directions south, south-east, east, north-east, north,
    // north-west, west, south-west in that column order - each 64x64 at y=0.
    const Facing expectedOrder[8] = {Facing::South, Facing::SouthEast, Facing::East, Facing::NorthEast,
        Facing::North, Facing::NorthWest, Facing::West, Facing::SouthWest};

    for (int col = 0; col < 8; ++col) {
        const sf::IntRect rect = asset->idleRotationFrames[static_cast<std::size_t>(expectedOrder[col])];
        CHECK(rect.left == col * 64);
        CHECK(rect.top == 0);
        CHECK(rect.width == 64);
        CHECK(rect.height == 64);
    }
}

TEST_CASE("SpriteSheetLoader parses the walk animation for every direction")
{
    SpriteSheetLoader::clearCache();
    auto asset = SpriteSheetLoader::load(kPng, kJson);
    REQUIRE(asset != nullptr);

    const auto walkIt = asset->animations.find("walk");
    REQUIRE(walkIt != asset->animations.end());
    CHECK(walkIt->second.size() == 8);

    // Rows 1..8 map to south, south-east, east, north-east, north,
    // north-west, west, south-west respectively, per the JSON fixture.
    const Facing rowDirection[8] = {Facing::South, Facing::SouthEast, Facing::East, Facing::NorthEast,
        Facing::North, Facing::NorthWest, Facing::West, Facing::SouthWest};

    for (int i = 0; i < 8; ++i) {
        const int row = i + 1;
        const auto dirIt = walkIt->second.find(rowDirection[i]);
        REQUIRE(dirIt != walkIt->second.end());
        const AnimationFrameData& data = dirIt->second;
        REQUIRE(data.frames.size() == 6);
        for (int col = 0; col < 6; ++col) {
            CHECK(data.frames[static_cast<std::size_t>(col)].left == col * 64);
            CHECK(data.frames[static_cast<std::size_t>(col)].top == row * 64);
        }
    }
}

TEST_CASE("SpriteSheetLoader caches by (pngPath, jsonPath)")
{
    SpriteSheetLoader::clearCache();
    auto first = SpriteSheetLoader::load(kPng, kJson);
    auto second = SpriteSheetLoader::load(kPng, kJson);
    CHECK(first == second);
}

TEST_CASE("SpriteSheetLoader throws a clear error on malformed JSON")
{
    const char* badJsonPath = "tests_tmp_missing_spritesheet.json";
    {
        std::ofstream f(badJsonPath);
        f << "{ \"not_spritesheet\": true }";
    }

    SpriteSheetLoader::clearCache();
    CHECK_THROWS_AS(SpriteSheetLoader::load(kPng, badJsonPath), std::runtime_error);

    std::remove(badJsonPath);
}

TEST_CASE("SpriteSheetLoader throws when the PNG is missing")
{
    SpriteSheetLoader::clearCache();
    CHECK_THROWS_AS(SpriteSheetLoader::load("assets/does_not_exist.png", kJson), std::runtime_error);
}

TEST_CASE("facingFromString parses every PixelLab direction and rejects unknown ones")
{
    CHECK(facingFromString("south") == Facing::South);
    CHECK(facingFromString("south-east") == Facing::SouthEast);
    CHECK(facingFromString("east") == Facing::East);
    CHECK(facingFromString("north-east") == Facing::NorthEast);
    CHECK(facingFromString("north") == Facing::North);
    CHECK(facingFromString("north-west") == Facing::NorthWest);
    CHECK(facingFromString("west") == Facing::West);
    CHECK(facingFromString("south-west") == Facing::SouthWest);
    CHECK_THROWS_AS(facingFromString("up"), std::invalid_argument);
}
