#include "commands.hpp"

#include "xpscenery/core_types/bounding_box.hpp"
#include "xpscenery/core_types/lat_lon.hpp"
#include "xpscenery/core_types/tile_coord.hpp"
#include "xpscenery/geodesy/vincenty.hpp"

#include <CLI/CLI.hpp>

#include <cstdio>
#include <print>
#include <string>

namespace xps::app_cli::detail
{

    void register_bbox(CLI::App &root)
    {
        auto *cmd = root.add_subcommand(
            "bbox",
            "Report the geodesic dimensions of a DSF tile or an explicit bbox");

        static std::string tile_name;
        static bool json = false;
        cmd->add_option("--tile", tile_name,
                        "Tile name like '+50+014'")
            ->required();
        cmd->add_flag("--json", json, "Emit machine-readable JSON");

        cmd->callback([]
                      {
        using xps::core_types::LatLon;
        using xps::core_types::TileCoord;
        using xps::geodesy::vincenty_inverse;

        auto tc = TileCoord::parse(tile_name);
        if (!tc) {
            std::fprintf(stderr, "bbox: %s\n", tc.error().c_str());
            std::exit(1);
        }

        const auto sw = tc->sw_corner();
        const auto ne = tc->ne_corner();
        const auto nw = LatLon::from_lat_lon(ne.lat(), sw.lon());
        const auto se = LatLon::from_lat_lon(sw.lat(), ne.lon());

        auto d_south = vincenty_inverse(sw, se);
        auto d_north = vincenty_inverse(nw, ne);
        auto d_west  = vincenty_inverse(sw, nw);
        auto d_east  = vincenty_inverse(se, ne);
        auto d_diag  = vincenty_inverse(sw, ne);

        if (!d_south || !d_north || !d_west || !d_east || !d_diag) {
            std::fprintf(stderr, "bbox: vincenty failed\n");
            std::exit(2);
        }

        // Trapezoid-ish area: average top/bottom width times height.
        const double avg_width  = 0.5 * (d_south->distance_m + d_north->distance_m);
        const double avg_height = 0.5 * (d_west->distance_m  + d_east->distance_m);
        const double area_m2    = avg_width * avg_height;

        if (json) {
            std::println(
                R"({{"tile":"{}","south_m":{:.3f},"north_m":{:.3f},"west_m":{:.3f},"east_m":{:.3f},"diagonal_m":{:.3f},"approx_area_km2":{:.3f}}})",
                tc->canonical_name(),
                d_south->distance_m, d_north->distance_m,
                d_west->distance_m,  d_east->distance_m,
                d_diag->distance_m,  area_m2 / 1'000'000.0);
        } else {
            std::println("tile         : {}", tc->canonical_name());
            std::println("sw / ne      : {}  ->  {}",
                sw.to_string(), ne.to_string());
            std::println("south edge   : {:>12.3f} m", d_south->distance_m);
            std::println("north edge   : {:>12.3f} m", d_north->distance_m);
            std::println("west  edge   : {:>12.3f} m", d_west->distance_m);
            std::println("east  edge   : {:>12.3f} m", d_east->distance_m);
            std::println("diagonal     : {:>12.3f} m", d_diag->distance_m);
            std::println("approx area  : {:>12.3f} km^2", area_m2 / 1'000'000.0);
        } });
    }

} // namespace xps::app_cli::detail
