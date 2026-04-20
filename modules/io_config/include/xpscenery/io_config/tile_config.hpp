#pragma once
// xpscenery — io_config/tile_config.hpp
//
// Typed tile build configuration. Replaces the legacy RenderFarm
// `+50+015.json` ad-hoc schema with an explicit, versioned, validated
// structure. Unknown keys are *preserved* so future fields don't get
// silently dropped when a file is round-tripped.

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "xpscenery/core_types/bounding_box.hpp"
#include "xpscenery/core_types/tile_coord.hpp"

namespace xps::io_config {

struct LayerSource {
    std::string               id;        ///< user-facing name
    std::string               kind;      ///< "shapefile"|"geotiff"|"osm_pbf"|"dsf"
    std::filesystem::path     path;      ///< absolute or tile-root-relative
    std::optional<std::string> srs;      ///< EPSG code like "EPSG:4326"
    int                       priority = 0; ///< higher wins
    bool                      enabled  = true;
};

struct TileConfig {
    /// Schema version. We start at 1; bump on breaking changes.
    int                              schema_version = 1;
    xps::core_types::TileCoord        tile{};
    std::optional<xps::core_types::BoundingBox> aoi; ///< optional sub-bbox
    std::vector<LayerSource>         layers;
    std::filesystem::path            output_dsf;     ///< where to write result

    // Build knobs — sane defaults, override per tile.
    struct Meshing {
        double max_edge_m           = 2'500.0;
        double min_triangle_area_m2 = 100.0;
        double smoothing_factor     = 0.5;   ///< 0..1, 0 = sharp
    } meshing;

    struct Export {
        bool    bit_identical_baseline = false; ///< mimic legacy output byte-for-byte
        bool    generate_overlay       = true;
        bool    compress               = true;
    } xp_export;
};

/// Parse a tile configuration from a JSON file. Missing optional fields
/// become defaults; required fields trigger a descriptive error.
[[nodiscard]] std::expected<TileConfig, std::string>
load(const std::filesystem::path& json_path);

/// Serialise a config back to JSON text (pretty-printed).
[[nodiscard]] std::string dump(const TileConfig& cfg);

}  // namespace xps::io_config
