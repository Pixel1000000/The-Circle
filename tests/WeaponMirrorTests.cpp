#include <doctest/doctest.h>

#include "sprite/WeaponTypes.hpp"

using namespace tc::sprite;

TEST_CASE("resolveMirror leaves the 5 authored directions untouched")
{
    for (Facing f : {Facing::South, Facing::North, Facing::East, Facing::SouthEast, Facing::NorthEast}) {
        const FacingMirror m = resolveMirror(f);
        CHECK(m.source == f);
        CHECK_FALSE(m.mirrored);
    }
}

TEST_CASE("resolveMirror maps the 3 west-side directions onto their east-side source")
{
    const FacingMirror west = resolveMirror(Facing::West);
    CHECK(west.source == Facing::East);
    CHECK(west.mirrored);

    const FacingMirror southWest = resolveMirror(Facing::SouthWest);
    CHECK(southWest.source == Facing::SouthEast);
    CHECK(southWest.mirrored);

    const FacingMirror northWest = resolveMirror(Facing::NorthWest);
    CHECK(northWest.source == Facing::NorthEast);
    CHECK(northWest.mirrored);
}

TEST_CASE("curveForDirection returns the matching authored keyframe list")
{
    WeaponAttackCurve curve;
    curve.south = {{0, {1.f, 2.f}, 10.f, true}};
    curve.east = {{0, {3.f, 4.f}, 20.f, false}, {1, {5.f, 6.f}, 30.f, true}};

    CHECK(curveForDirection(curve, Facing::South).size() == 1);
    CHECK(curveForDirection(curve, Facing::South)[0].rotationDeg == 10.f);

    CHECK(curveForDirection(curve, Facing::East).size() == 2);
    CHECK(curveForDirection(curve, Facing::East)[1].offset.x == 5.f);
}

TEST_CASE("curveForDirection rejects west-side directions that were never authored")
{
    WeaponAttackCurve curve;
    CHECK_THROWS_AS(curveForDirection(curve, Facing::West), std::invalid_argument);
    CHECK_THROWS_AS(curveForDirection(curve, Facing::SouthWest), std::invalid_argument);
    CHECK_THROWS_AS(curveForDirection(curve, Facing::NorthWest), std::invalid_argument);
}

TEST_CASE("A full west-facing keyframe resolves by mirroring its east-side source")
{
    WeaponAttackCurve curve;
    curve.east = {{3, {10.f, -4.f}, 45.f, true}};

    const FacingMirror mirror = resolveMirror(Facing::West);
    const auto& keyframes = curveForDirection(curve, mirror.source);
    REQUIRE(keyframes.size() == 1);

    SocketKeyframe kf = keyframes[0];
    if (mirror.mirrored) {
        kf.offset.x = -kf.offset.x;
        kf.rotationDeg = -kf.rotationDeg;
    }

    CHECK(kf.offset.x == doctest::Approx(-10.f));
    CHECK(kf.offset.y == doctest::Approx(-4.f));
    CHECK(kf.rotationDeg == doctest::Approx(-45.f));
}
