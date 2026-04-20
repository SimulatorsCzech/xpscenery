#include <catch2/catch_test_macros.hpp>
#include "xpscenery/xpscenery.h"

TEST_CASE("xps_version returns valid major/minor/patch", "[version]") {
    const xps_version_t* v = xps_version();
    REQUIRE(v != nullptr);
    REQUIRE(v->major >= 0);
    REQUIRE(v->minor >= 0);
    REQUIRE(v->patch >= 0);
}

TEST_CASE("xps_build_info returns non-empty string", "[version]") {
    const char* info = xps_build_info();
    REQUIRE(info != nullptr);
    REQUIRE(info[0] != '\0');
}
