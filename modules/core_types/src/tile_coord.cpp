#include "xpscenery/core_types/tile_coord.hpp"
#include "xpscenery/core_types/lat_lon.hpp"

#include <cctype>
#include <charconv>
#include <cmath>
#include <format>

namespace xps::core_types {

namespace {

constexpr bool lat_in_range(int v) noexcept { return v >= -89 && v <= 89; }
constexpr bool lon_in_range(int v) noexcept { return v >= -180 && v <= 179; }

// Parse "+LL" / "-LL" (2 digit lat) or "+LLL" / "-LLL" (3 digit lon).
// Returns value and consumed count, or 0-consumed on error.
struct ParsedField { int value = 0; std::size_t consumed = 0; };

ParsedField parse_signed(std::string_view s, std::size_t digits) noexcept {
    if (s.size() < digits + 1) return {};
    const char sign = s[0];
    if (sign != '+' && sign != '-') return {};
    int v = 0;
    auto [ptr, ec] = std::from_chars(s.data() + 1, s.data() + 1 + digits, v);
    if (ec != std::errc{} || ptr != s.data() + 1 + digits) return {};
    return {(sign == '-') ? -v : v, digits + 1};
}

}  // namespace

std::expected<TileCoord, std::string>
TileCoord::from_lat_lon(int lat_deg, int lon_deg) noexcept {
    if (!lat_in_range(lat_deg)) {
        return std::unexpected(std::format(
            "tile latitude out of range [-89,89]: {}", lat_deg));
    }
    if (!lon_in_range(lon_deg)) {
        return std::unexpected(std::format(
            "tile longitude out of range [-180,179]: {}", lon_deg));
    }
    return TileCoord{lat_deg, lon_deg};
}

std::expected<TileCoord, std::string>
TileCoord::from_point(const LatLon& p) noexcept {
    if (!p.is_valid()) {
        return std::unexpected(std::format(
            "point not inside WGS84 range: {}", p.to_string()));
    }
    const int lat = static_cast<int>(std::floor(p.lat()));
    const int lon = static_cast<int>(std::floor(p.lon()));
    return from_lat_lon(lat, lon);
}

std::expected<TileCoord, std::string>
TileCoord::parse(std::string_view name) noexcept {
    if (name.size() != 7) {
        return std::unexpected(std::format(
            "tile name must be 7 chars (like +50+015), got '{}' ({} chars)",
            name, name.size()));
    }
    auto lat_p = parse_signed(name.substr(0, 3), 2);
    auto lon_p = parse_signed(name.substr(3, 4), 3);
    if (lat_p.consumed == 0 || lon_p.consumed == 0) {
        return std::unexpected(std::format(
            "tile name not in '+LL+LLL' / '-LL-LLL' shape: '{}'", name));
    }
    return from_lat_lon(lat_p.value, lon_p.value);
}

std::string TileCoord::canonical_name() const {
    return std::format("{:+03d}{:+04d}", lat_, lon_);
}

std::string TileCoord::supertile_name() const {
    const int super_lat = (lat_ >= 0) ? (lat_ / 10) * 10 : ((lat_ - 9) / 10) * 10;
    const int super_lon = (lon_ >= 0) ? (lon_ / 10) * 10 : ((lon_ - 9) / 10) * 10;
    return std::format("{:+03d}{:+04d}", super_lat, super_lon);
}

LatLon TileCoord::sw_corner() const noexcept {
    return LatLon::from_lat_lon(static_cast<double>(lat_), static_cast<double>(lon_));
}

LatLon TileCoord::ne_corner() const noexcept {
    return LatLon::from_lat_lon(static_cast<double>(lat_ + 1), static_cast<double>(lon_ + 1));
}

}  // namespace xps::core_types
