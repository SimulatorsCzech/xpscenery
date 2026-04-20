#pragma once
// xpscenery — core_types/tile_coord.hpp
//
// X-Plane DSF tile coordinate. A tile is the 1°×1° cell identified by
// the integer (lat, lon) of its *south-west* corner. Examples:
//   +50+015 = Central Europe tile 50°N,15°E → covers lat [50,51], lon [15,16]
//   -01-070 = Amazon tile 1°S,70°W       → covers lat [-1,0], lon [-70,-69]
//
// Legacy xptools represented this as loose ints and a hand-rolled string
// formatter scattered across DSFTool/RenderFarm. We centralize here.

#include <compare>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace xps::core_types {

class LatLon;

/// Integer tile coordinate (south-west corner latitude/longitude).
class TileCoord {
public:
    constexpr TileCoord() noexcept = default;

    /// Construct from integer lat/lon degrees. `lat` must be in [-89, 89],
    /// `lon` in [-180, 179]. Returns nullopt-like via std::expected otherwise.
    static std::expected<TileCoord, std::string>
    from_lat_lon(int lat_deg, int lon_deg) noexcept;

    /// Construct from a point (truncates toward the SW corner). The point
    /// must be inside the global tile grid.
    static std::expected<TileCoord, std::string>
    from_point(const LatLon& p) noexcept;

    /// Parse canonical DSF name like "+50+015", "-01-070". Accepts either
    /// "+50+015" or "50+015" (sign optional on non-negative values).
    static std::expected<TileCoord, std::string>
    parse(std::string_view name) noexcept;

    [[nodiscard]] constexpr int lat() const noexcept { return lat_; }
    [[nodiscard]] constexpr int lon() const noexcept { return lon_; }

    /// Canonical "+LL±LLL" name used by X-Plane DSF file paths.
    [[nodiscard]] std::string canonical_name() const;

    /// 10° super-tile group name ("+50+010" covers tiles 50..59N,10..19E in path).
    [[nodiscard]] std::string supertile_name() const;

    /// South-west corner as LatLon.
    [[nodiscard]] LatLon sw_corner() const noexcept;

    /// North-east corner as LatLon.
    [[nodiscard]] LatLon ne_corner() const noexcept;

    friend constexpr auto operator<=>(const TileCoord&, const TileCoord&) = default;

private:
    constexpr TileCoord(int lat, int lon) noexcept : lat_{lat}, lon_{lon} {}

    std::int32_t lat_ = 0;
    std::int32_t lon_ = 0;
};

}  // namespace xps::core_types
