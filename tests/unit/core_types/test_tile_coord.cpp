#include <catch2/catch_test_macros.hpp>

#include "xpscenery/core_types/lat_lon.hpp"
#include "xpscenery/core_types/tile_coord.hpp"

using xps::core_types::LatLon;
using xps::core_types::TileCoord;

TEST_CASE("TileCoord from_lat_lon accepts valid ints", "[core_types][tile]") {
    auto t = TileCoord::from_lat_lon(50, 15);
    REQUIRE(t.has_value());
    REQUIRE(t->lat() == 50);
    REQUIRE(t->lon() == 15);
}

TEST_CASE("TileCoord from_lat_lon rejects out-of-range", "[core_types][tile]") {
    REQUIRE_FALSE(TileCoord::from_lat_lon(90, 0).has_value());
    REQUIRE_FALSE(TileCoord::from_lat_lon(-90, 0).has_value());
    REQUIRE_FALSE(TileCoord::from_lat_lon(0, 180).has_value());
    REQUIRE_FALSE(TileCoord::from_lat_lon(0, -181).has_value());
}

TEST_CASE("TileCoord::canonical_name matches X-Plane pattern",
          "[core_types][tile]") {
    auto a = TileCoord::from_lat_lon(50, 15).value();
    REQUIRE(a.canonical_name() == "+50+015");

    auto b = TileCoord::from_lat_lon(-1, -70).value();
    REQUIRE(b.canonical_name() == "-01-070");

    auto c = TileCoord::from_lat_lon(0, 0).value();
    REQUIRE(c.canonical_name() == "+00+000");

    auto d = TileCoord::from_lat_lon(-89, 179).value();
    REQUIRE(d.canonical_name() == "-89+179");
}

TEST_CASE("TileCoord::supertile_name groups 10°", "[core_types][tile]") {
    REQUIRE(TileCoord::from_lat_lon( 50,  15).value().supertile_name() == "+50+010");
    REQUIRE(TileCoord::from_lat_lon( 59,  19).value().supertile_name() == "+50+010");
    REQUIRE(TileCoord::from_lat_lon(  0,   0).value().supertile_name() == "+00+000");
    REQUIRE(TileCoord::from_lat_lon( -1, -70).value().supertile_name() == "-10-070");
    REQUIRE(TileCoord::from_lat_lon( -9,  -1).value().supertile_name() == "-10-010");
}

TEST_CASE("TileCoord::parse round-trips canonical names",
          "[core_types][tile][parse]") {
    for (auto name : {"+50+015", "-01-070", "+00+000", "-89+179"}) {
        auto t = TileCoord::parse(name);
        REQUIRE(t.has_value());
        REQUIRE(t->canonical_name() == name);
    }
}

TEST_CASE("TileCoord::parse rejects bad input", "[core_types][tile][parse]") {
    REQUIRE_FALSE(TileCoord::parse("50+015").has_value());   // missing sign
    REQUIRE_FALSE(TileCoord::parse("+50+15").has_value());   // short lon
    REQUIRE_FALSE(TileCoord::parse("+50+0015").has_value()); // too long
    REQUIRE_FALSE(TileCoord::parse("+aa+015").has_value());  // non-digit
    REQUIRE_FALSE(TileCoord::parse("").has_value());
}

TEST_CASE("TileCoord corners", "[core_types][tile][corners]") {
    auto t = TileCoord::from_lat_lon(50, 15).value();
    REQUIRE(t.sw_corner() == LatLon::from_lat_lon(50.0, 15.0));
    REQUIRE(t.ne_corner() == LatLon::from_lat_lon(51.0, 16.0));
}

TEST_CASE("TileCoord::from_point snaps to SW corner",
          "[core_types][tile][from_point]") {
    auto t = TileCoord::from_point(LatLon::from_lat_lon(50.5, 15.9));
    REQUIRE(t.has_value());
    REQUIRE(t->lat() == 50);
    REQUIRE(t->lon() == 15);
}
