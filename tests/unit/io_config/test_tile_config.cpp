#include <catch2/catch_test_macros.hpp>

#include "xpscenery/io_config/tile_config.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <filesystem>

TEST_CASE("tile_config load from a minimal JSON", "[io_config][tile_config]") {
    namespace fs = std::filesystem;
    auto dir = fs::temp_directory_path() / "xpscenery_tests";
    fs::create_directories(dir);
    auto file = dir / "tile_min.json";
    fs::remove(file);

    const std::string json_text = R"({
        "schema_version": 1,
        "tile_name": "+50+015",
        "layers": [
            {"id":"shp1","kind":"shapefile","path":"landuse.shp"}
        ]
    })";
    REQUIRE(xps::io_filesystem::write_text_utf8_atomic(file, json_text).has_value());

    auto cfg = xps::io_config::load(file);
    REQUIRE(cfg.has_value());
    REQUIRE(cfg->schema_version == 1);
    REQUIRE(cfg->tile.canonical_name() == "+50+015");
    REQUIRE(cfg->layers.size() == 1);
    REQUIRE(cfg->layers[0].id   == "shp1");
    REQUIRE(cfg->layers[0].kind == "shapefile");
    REQUIRE(cfg->layers[0].enabled == true);
}

TEST_CASE("tile_config rejects missing tile fields",
          "[io_config][tile_config]") {
    namespace fs = std::filesystem;
    auto dir = fs::temp_directory_path() / "xpscenery_tests";
    fs::create_directories(dir);
    auto file = dir / "tile_bad.json";
    fs::remove(file);

    REQUIRE(xps::io_filesystem::write_text_utf8_atomic(file,
        R"({"schema_version":1})").has_value());

    auto cfg = xps::io_config::load(file);
    REQUIRE_FALSE(cfg.has_value());
}

TEST_CASE("tile_config dump is round-trippable",
          "[io_config][tile_config][roundtrip]") {
    namespace fs = std::filesystem;
    auto dir = fs::temp_directory_path() / "xpscenery_tests";
    fs::create_directories(dir);

    auto file1 = dir / "rt1.json";
    auto file2 = dir / "rt2.json";
    fs::remove(file1); fs::remove(file2);

    const std::string json_text = R"({
        "schema_version": 1,
        "tile_name": "+50+015",
        "aoi": {"sw_lat":50.1,"sw_lon":15.1,"ne_lat":50.9,"ne_lon":15.9},
        "layers": [
            {"id":"L","kind":"geotiff","path":"dem.tif","priority":3}
        ],
        "output_dsf": "+50+015.dsf"
    })";
    REQUIRE(xps::io_filesystem::write_text_utf8_atomic(file1, json_text).has_value());

    auto cfg1 = xps::io_config::load(file1);
    REQUIRE(cfg1.has_value());

    const auto dumped = xps::io_config::dump(*cfg1);
    REQUIRE(xps::io_filesystem::write_text_utf8_atomic(file2, dumped).has_value());

    auto cfg2 = xps::io_config::load(file2);
    REQUIRE(cfg2.has_value());

    REQUIRE(cfg2->schema_version      == cfg1->schema_version);
    REQUIRE(cfg2->tile                == cfg1->tile);
    REQUIRE(cfg2->layers.size()       == cfg1->layers.size());
    REQUIRE(cfg2->layers[0].priority  == 3);
    REQUIRE(cfg2->aoi.has_value());
    REQUIRE(cfg1->aoi.has_value());
    REQUIRE(cfg2->aoi->sw() == cfg1->aoi->sw());
    REQUIRE(cfg2->aoi->ne() == cfg1->aoi->ne());
}
