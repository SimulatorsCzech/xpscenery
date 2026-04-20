#include <catch2/catch_test_macros.hpp>
#include "xpscenery/xpscenery.h"

TEST_CASE("xps_init + xps_shutdown roundtrip", "[smoke]") {
    REQUIRE(xps_init() == XPS_OK);
    xps_shutdown();
}

TEST_CASE("xps_last_error returns empty on success", "[smoke]") {
    REQUIRE(xps_init() == XPS_OK);
    REQUIRE(xps_last_error() != nullptr);
    xps_shutdown();
}
