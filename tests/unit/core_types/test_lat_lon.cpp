#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "xpscenery/core_types/lat_lon.hpp"

using xps::core_types::LatLon;

TEST_CASE("LatLon basic construction", "[core_types][lat_lon]") {
    auto p = LatLon::from_lat_lon(50.0, 15.0);
    REQUIRE(p.lat() == 50.0);
    REQUIRE(p.lon() == 15.0);
    REQUIRE(p.is_valid());
    REQUIRE(p.is_finite());
}

TEST_CASE("LatLon validity limits", "[core_types][lat_lon]") {
    REQUIRE(!LatLon::from_lat_lon(91.0, 0.0).is_valid());
    REQUIRE(!LatLon::from_lat_lon(-91.0, 0.0).is_valid());
    REQUIRE(!LatLon::from_lat_lon(0.0, 181.0).is_valid());
    REQUIRE(!LatLon::from_lat_lon(0.0, -181.0).is_valid());
    REQUIRE( LatLon::from_lat_lon(-90.0, -180.0).is_valid());
    REQUIRE( LatLon::from_lat_lon( 90.0,  180.0).is_valid());
}

TEST_CASE("LatLon longitude wrapping", "[core_types][lat_lon]") {
    auto wrapped = LatLon::from_lat_lon(0.0, 181.0).wrapped();
    REQUIRE_THAT(wrapped.lon(), Catch::Matchers::WithinAbs(-179.0, 1e-9));

    auto w2 = LatLon::from_lat_lon(0.0, -200.0).wrapped();
    REQUIRE_THAT(w2.lon(), Catch::Matchers::WithinAbs(160.0, 1e-9));
}

TEST_CASE("LatLon haversine distance: Prague → Berlin ≈ 280 km",
          "[core_types][lat_lon][haversine]") {
    // Prague  50.0875° N, 14.4214° E
    // Berlin  52.5200° N, 13.4050° E
    auto prague = LatLon::from_lat_lon(50.0875, 14.4214);
    auto berlin = LatLon::from_lat_lon(52.5200, 13.4050);
    double d = prague.haversine_distance_m(berlin);
    // Published great-circle distance ≈ 280 km (±3 km depending on R).
    REQUIRE_THAT(d, Catch::Matchers::WithinAbs(280'000.0, 3'000.0));
}

TEST_CASE("LatLon to_string keeps 7 decimals", "[core_types][lat_lon]") {
    auto p = LatLon::from_lat_lon(50.123456789, 15.9876543);
    auto s = p.to_string();
    // String is "50.1234568,15.9876543" (7 decimals each).
    REQUIRE(s.starts_with("50.1234568"));
    REQUIRE(s.contains(','));
    REQUIRE(s.ends_with("15.9876543"));
}

TEST_CASE("LatLon comparison is deterministic", "[core_types][lat_lon]") {
    auto a = LatLon::from_lat_lon(50.0, 15.0);
    auto b = LatLon::from_lat_lon(51.0, 15.0);
    REQUIRE(a < b);
    REQUIRE(a != b);
    REQUIRE(a == LatLon::from_lat_lon(50.0, 15.0));
}
