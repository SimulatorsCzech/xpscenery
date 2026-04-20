#include <catch2/catch_test_macros.hpp>

#include "xpscenery/utils/version.hpp"

TEST_CASE("xps_utils scaffold is linkable", "[utils][scaffold]") {
    REQUIRE(xps::utils::module_name() != nullptr);
    REQUIRE(std::string_view(xps::utils::module_name()) == "xps_utils");
}

TEST_CASE("xps_utils revision starts at zero", "[utils][scaffold]") {
    // Placeholder scaffold; bumps as real code lands.
    REQUIRE(xps::utils::module_revision() == 0);
}
