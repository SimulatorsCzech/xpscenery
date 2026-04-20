#include "commands.hpp"

#include "xpscenery/core_types/lat_lon.hpp"
#include "xpscenery/geodesy/vincenty.hpp"

#include <CLI/CLI.hpp>

#include <cstdio>
#include <iostream>
#include <print>
#include <string>

namespace xps::app_cli::detail {

void register_distance(CLI::App& root) {
    auto* cmd = root.add_subcommand(
        "distance",
        "Compute WGS84 geodesic distance + bearings between two points (Vincenty inverse)");

    static double lat1 = 0.0, lon1 = 0.0, lat2 = 0.0, lon2 = 0.0;
    static bool json = false;

    cmd->add_option("--lat1", lat1, "First point latitude  [deg]")->required();
    cmd->add_option("--lon1", lon1, "First point longitude [deg]")->required();
    cmd->add_option("--lat2", lat2, "Second point latitude  [deg]")->required();
    cmd->add_option("--lon2", lon2, "Second point longitude [deg]")->required();
    cmd->add_flag("--json", json, "Emit machine-readable JSON");

    cmd->callback([] {
        using xps::core_types::LatLon;
        auto a = LatLon::from_lat_lon(lat1, lon1);
        auto b = LatLon::from_lat_lon(lat2, lon2);

        auto res = xps::geodesy::vincenty_inverse(a, b);
        if (!res) {
            std::cerr << "distance: " << res.error() << '\n';
            std::exit(1);
        }

        if (json) {
            std::println(
                R"({{"distance_m":{:.6f},"initial_bearing_deg":{:.6f},"final_bearing_deg":{:.6f},"iterations":{}}})",
                res->distance_m, res->initial_bearing_deg,
                res->final_bearing_deg, res->iterations);
        } else {
            std::println("distance          : {:.3f} m  ({:.3f} km)",
                res->distance_m, res->distance_m / 1000.0);
            std::println("initial bearing   : {:.4f} deg", res->initial_bearing_deg);
            std::println("final   bearing   : {:.4f} deg", res->final_bearing_deg);
            std::println("iterations        : {}", res->iterations);
        }
    });
}

}  // namespace xps::app_cli::detail
