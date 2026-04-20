#include "commands.hpp"

#include "xpscenery/core_types/lat_lon.hpp"
#include "xpscenery/core_types/tile_coord.hpp"

#include <CLI/CLI.hpp>

#include <cstdio>
#include <print>

namespace xps::app_cli::detail
{

    void register_tile(CLI::App &root)
    {
        auto *cmd = root.add_subcommand(
            "tile",
            "Resolve a WGS84 lat/lon into its X-Plane 1°×1° DSF tile");

        static double lat = 0.0, lon = 0.0;
        static bool json = false;
        cmd->add_option("--lat", lat, "Latitude  [deg]")->required();
        cmd->add_option("--lon", lon, "Longitude [deg]")->required();
        cmd->add_flag("--json", json, "Emit machine-readable JSON");

        cmd->callback([]
                      {
        using xps::core_types::LatLon;
        using xps::core_types::TileCoord;

        auto p  = LatLon::from_lat_lon(lat, lon);
        auto tc = TileCoord::from_point(p);
        if (!tc) {
            std::fprintf(stderr, "tile: %s\n", tc.error().c_str());
            std::exit(1);
        }
        if (json) {
            std::println(
                R"({{"point":{{"lat":{},"lon":{}}},"tile":"{}","sw":"{}","ne":"{}","supertile":"{}"}})",
                lat, lon, tc->canonical_name(),
                tc->sw_corner().to_string(),
                tc->ne_corner().to_string(),
                tc->supertile_name());
        } else {
            std::println("point     : {},{}", lat, lon);
            std::println("tile      : {}", tc->canonical_name());
            std::println("sw / ne   : {}  ->  {}",
                tc->sw_corner().to_string(),
                tc->ne_corner().to_string());
            std::println("supertile : {}", tc->supertile_name());
        } });
    }

} // namespace xps::app_cli::detail
