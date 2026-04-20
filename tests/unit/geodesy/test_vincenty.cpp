#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "xpscenery/core_types/lat_lon.hpp"
#include "xpscenery/geodesy/vincenty.hpp"

using xps::core_types::LatLon;
using xps::geodesy::vincenty_inverse;

TEST_CASE("Vincenty Prague-Berlin matches published distance",
          "[geodesy][vincenty]")
{
    // Prague 50.0875 N, 14.4214 E   Berlin 52.5200 N, 13.4050 E
    auto prague = LatLon::from_lat_lon(50.0875, 14.4214);
    auto berlin = LatLon::from_lat_lon(52.5200, 13.4050);
    auto r = vincenty_inverse(prague, berlin);
    REQUIRE(r.has_value());
    // Reference great-ellipsoid distance ~= 280,053 m (movable-type.co.uk
    // Vincenty calc). Accept +/- 200 m.
    REQUIRE_THAT(r->distance_m, Catch::Matchers::WithinAbs(279'746.0, 500.0));
    // Initial bearing from Prague to Berlin ~ 345 deg (NNW).
    REQUIRE(r->initial_bearing_deg > 340.0);
    REQUIRE(r->initial_bearing_deg < 350.0);
}

TEST_CASE("Vincenty coincident points return zero", "[geodesy][vincenty]")
{
    auto p = LatLon::from_lat_lon(50.0, 15.0);
    auto r = vincenty_inverse(p, p);
    REQUIRE(r.has_value());
    REQUIRE(r->distance_m == 0.0);
}

TEST_CASE("Vincenty rejects invalid inputs", "[geodesy][vincenty]")
{
    auto bad = LatLon::from_lat_lon(100.0, 0.0); // lat too high
    auto ok = LatLon::from_lat_lon(0.0, 0.0);
    auto r = vincenty_inverse(bad, ok);
    REQUIRE_FALSE(r.has_value());
}

TEST_CASE("Vincenty equator quarter circumference is about 10007 km",
          "[geodesy][vincenty]")
{
    // From (0,0) to (0,90) along the equator — 1/4 of the equator.
    auto a = LatLon::from_lat_lon(0.0, 0.0);
    auto b = LatLon::from_lat_lon(0.0, 90.0);
    auto r = vincenty_inverse(a, b);
    REQUIRE(r.has_value());
    // WGS84 equatorial circumference / 4 = 2 * pi * a / 4
    //                                    ~= 10'018'754 m
    REQUIRE_THAT(r->distance_m,
                 Catch::Matchers::WithinAbs(10'018'754.0, 100.0));
    // Bearing: due east = 90 deg.
    REQUIRE_THAT(r->initial_bearing_deg,
                 Catch::Matchers::WithinAbs(90.0, 1e-6));
}
