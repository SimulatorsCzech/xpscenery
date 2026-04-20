#include <catch2/catch_test_macros.hpp>

#include "xpscenery/core_types/bounding_box.hpp"

using xps::core_types::BoundingBox;
using xps::core_types::LatLon;

TEST_CASE("Default BoundingBox is empty", "[core_types][bbox]") {
    BoundingBox b;
    REQUIRE(b.is_empty());
    REQUIRE(b.width_deg()  == 0.0);
    REQUIRE(b.height_deg() == 0.0);
}

TEST_CASE("BoundingBox::from_corners basic", "[core_types][bbox]") {
    auto b = BoundingBox::from_corners(
        LatLon::from_lat_lon(50.0, 15.0),
        LatLon::from_lat_lon(51.0, 16.0));
    REQUIRE_FALSE(b.is_empty());
    REQUIRE(b.width_deg()  == 1.0);
    REQUIRE(b.height_deg() == 1.0);
    REQUIRE(b.contains(LatLon::from_lat_lon(50.5, 15.5)));
    REQUIRE_FALSE(b.contains(LatLon::from_lat_lon(52.0, 15.5)));
}

TEST_CASE("BoundingBox::expand_to_include grows", "[core_types][bbox]") {
    BoundingBox b;
    b.expand_to_include(LatLon::from_lat_lon(50.0, 15.0));
    REQUIRE_FALSE(b.is_empty());
    REQUIRE(b.width_deg() == 0.0);

    b.expand_to_include(LatLon::from_lat_lon(51.0, 16.5));
    REQUIRE(b.width_deg()  == 1.5);
    REQUIRE(b.height_deg() == 1.0);

    b.expand_to_include(LatLon::from_lat_lon(49.5, 14.5));
    REQUIRE(b.width_deg()  == 2.0);
    REQUIRE(b.height_deg() == 1.5);
    REQUIRE(b.sw() == LatLon::from_lat_lon(49.5, 14.5));
    REQUIRE(b.ne() == LatLon::from_lat_lon(51.0, 16.5));
}

TEST_CASE("BoundingBox intersection", "[core_types][bbox]") {
    auto a = BoundingBox::from_corners(
        LatLon::from_lat_lon(50, 15), LatLon::from_lat_lon(52, 17));
    auto b = BoundingBox::from_corners(
        LatLon::from_lat_lon(51, 16), LatLon::from_lat_lon(53, 18));
    auto c = BoundingBox::from_corners(
        LatLon::from_lat_lon(60, 30), LatLon::from_lat_lon(61, 31));
    REQUIRE(a.intersects(b));
    REQUIRE_FALSE(a.intersects(c));
}
