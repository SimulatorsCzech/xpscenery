#include <catch2/catch_test_macros.hpp>

#include "xpscenery/core_types/tile_coord.hpp"
#include "xpscenery/io_filesystem/paths.hpp"

#include <filesystem>

using xps::core_types::TileCoord;

TEST_CASE("dsf_path_for_tile assembles the X-Plane canonical layout",
          "[io_filesystem][paths]") {
    const std::filesystem::path root = "C:/X-Plane 12/Custom Scenery/MyScenery";
    auto tile = TileCoord::from_lat_lon(50, 15).value();
    auto p    = xps::io_filesystem::dsf_path_for_tile(root, tile);
    // Normalise separators for cross-check via generic_string().
    auto s = p.generic_string();
    REQUIRE(s.find("Earth nav data") != std::string::npos);
    REQUIRE(s.find("+50+010") != std::string::npos);
    REQUIRE(s.ends_with("+50+015.dsf"));
}

TEST_CASE("require_existing_* rejects nonexistent paths",
          "[io_filesystem][paths]") {
    auto f = xps::io_filesystem::require_existing_file("::definitely/no/such/file");
    REQUIRE_FALSE(f.has_value());
    auto d = xps::io_filesystem::require_existing_dir("::definitely/no/such/dir");
    REQUIRE_FALSE(d.has_value());
}
