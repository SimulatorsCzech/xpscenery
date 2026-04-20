#include "xpscenery/io_config/tile_config.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <nlohmann/json.hpp>

#include <format>

namespace xps::io_config {

namespace fs = std::filesystem;
using nlohmann::json;

namespace {

template <typename T>
std::optional<T> try_get(const json& j, std::string_view key) {
    auto it = j.find(key);
    if (it == j.end() || it->is_null()) return std::nullopt;
    return it->get<T>();
}

std::expected<xps::core_types::TileCoord, std::string>
parse_tile(const json& j) {
    if (auto name = try_get<std::string>(j, "tile_name")) {
        auto parsed = xps::core_types::TileCoord::parse(*name);
        if (!parsed) return std::unexpected(std::move(parsed.error()));
        return *parsed;
    }
    auto lat = try_get<int>(j, "tile_lat");
    auto lon = try_get<int>(j, "tile_lon");
    if (!lat || !lon) {
        return std::unexpected(
            "tile config must have either 'tile_name' (e.g. '+50+015') "
            "or both 'tile_lat' and 'tile_lon' integer fields");
    }
    return xps::core_types::TileCoord::from_lat_lon(*lat, *lon);
}

}  // namespace

std::expected<TileConfig, std::string>
load(const fs::path& json_path) {
    auto text = xps::io_filesystem::read_text_utf8(json_path);
    if (!text) return std::unexpected(std::move(text.error()));

    json root;
    try {
        root = json::parse(*text, /*cb*/ nullptr,
                           /*throw*/ true, /*ignore_comments*/ true);
    } catch (const std::exception& e) {
        return std::unexpected(std::format(
            "JSON parse error in {}: {}", json_path.string(), e.what()));
    }

    TileConfig cfg;
    if (auto v = try_get<int>(root, "schema_version")) {
        cfg.schema_version = *v;
        if (*v != 1) {
            return std::unexpected(std::format(
                "unsupported schema_version {} (expected 1) in {}",
                *v, json_path.string()));
        }
    }

    auto tile = parse_tile(root);
    if (!tile) return std::unexpected(std::move(tile.error()));
    cfg.tile = *tile;

    if (auto aoi_j = root.find("aoi"); aoi_j != root.end() && aoi_j->is_object()) {
        const auto& a = *aoi_j;
        auto sw_lat = try_get<double>(a, "sw_lat");
        auto sw_lon = try_get<double>(a, "sw_lon");
        auto ne_lat = try_get<double>(a, "ne_lat");
        auto ne_lon = try_get<double>(a, "ne_lon");
        if (sw_lat && sw_lon && ne_lat && ne_lon) {
            cfg.aoi = xps::core_types::BoundingBox::from_corners(
                xps::core_types::LatLon::from_lat_lon(*sw_lat, *sw_lon),
                xps::core_types::LatLon::from_lat_lon(*ne_lat, *ne_lon));
        } else {
            return std::unexpected(
                "aoi must have numeric sw_lat, sw_lon, ne_lat, ne_lon");
        }
    }

    if (auto layers_j = root.find("layers"); layers_j != root.end()) {
        if (!layers_j->is_array()) {
            return std::unexpected("'layers' must be a JSON array");
        }
        for (const auto& lj : *layers_j) {
            LayerSource ls;
            auto id   = try_get<std::string>(lj, "id");
            auto kind = try_get<std::string>(lj, "kind");
            auto path = try_get<std::string>(lj, "path");
            if (!id || !kind || !path) {
                return std::unexpected(
                    "each layer needs 'id', 'kind', and 'path' string fields");
            }
            ls.id   = *id;
            ls.kind = *kind;
            ls.path = *path;
            ls.srs      = try_get<std::string>(lj, "srs");
            ls.priority = try_get<int>(lj, "priority").value_or(0);
            ls.enabled  = try_get<bool>(lj, "enabled").value_or(true);
            cfg.layers.push_back(std::move(ls));
        }
    }

    if (auto out = try_get<std::string>(root, "output_dsf")) {
        cfg.output_dsf = *out;
    }

    if (auto mj = root.find("meshing"); mj != root.end() && mj->is_object()) {
        if (auto v = try_get<double>(*mj, "max_edge_m"))           cfg.meshing.max_edge_m = *v;
        if (auto v = try_get<double>(*mj, "min_triangle_area_m2")) cfg.meshing.min_triangle_area_m2 = *v;
        if (auto v = try_get<double>(*mj, "smoothing_factor"))     cfg.meshing.smoothing_factor = *v;
    }

    if (auto ej = root.find("export"); ej != root.end() && ej->is_object()) {
        if (auto v = try_get<bool>(*ej, "bit_identical_baseline")) cfg.xp_export.bit_identical_baseline = *v;
        if (auto v = try_get<bool>(*ej, "generate_overlay"))       cfg.xp_export.generate_overlay = *v;
        if (auto v = try_get<bool>(*ej, "compress"))               cfg.xp_export.compress = *v;
    }

    return cfg;
}

std::string dump(const TileConfig& cfg) {
    json root;
    root["schema_version"] = cfg.schema_version;
    root["tile_name"]      = cfg.tile.canonical_name();
    root["tile_lat"]       = cfg.tile.lat();
    root["tile_lon"]       = cfg.tile.lon();
    if (cfg.aoi && !cfg.aoi->is_empty()) {
        root["aoi"] = {
            {"sw_lat", cfg.aoi->sw().lat()},
            {"sw_lon", cfg.aoi->sw().lon()},
            {"ne_lat", cfg.aoi->ne().lat()},
            {"ne_lon", cfg.aoi->ne().lon()},
        };
    }
    auto& layers_j = root["layers"] = json::array();
    for (const auto& l : cfg.layers) {
        json lj;
        lj["id"]       = l.id;
        lj["kind"]     = l.kind;
        lj["path"]     = l.path.string();
        if (l.srs)     lj["srs"] = *l.srs;
        lj["priority"] = l.priority;
        lj["enabled"]  = l.enabled;
        layers_j.push_back(std::move(lj));
    }
    if (!cfg.output_dsf.empty()) {
        root["output_dsf"] = cfg.output_dsf.string();
    }
    root["meshing"] = {
        {"max_edge_m",           cfg.meshing.max_edge_m},
        {"min_triangle_area_m2", cfg.meshing.min_triangle_area_m2},
        {"smoothing_factor",     cfg.meshing.smoothing_factor},
    };
    root["export"] = {
        {"bit_identical_baseline", cfg.xp_export.bit_identical_baseline},
        {"generate_overlay",       cfg.xp_export.generate_overlay},
        {"compress",               cfg.xp_export.compress},
    };
    return root.dump(2);
}

}  // namespace xps::io_config
