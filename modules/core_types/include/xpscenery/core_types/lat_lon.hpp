#pragma once
// xpscenery — core_types/lat_lon.hpp
//
// Strong-typed geographic coordinate. WGS84 degrees, double precision.
// Unlike the legacy xptools code (which used raw `double lat, lon;`
// everywhere and frequently mixed (lon,lat) vs (lat,lon) order), this
// type is ordering-safe: construction is explicit, accessors are named.

#include <compare>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>

namespace xps::core_types {

/// WGS84 latitude/longitude in decimal degrees.
/// Latitude is expected in [-90, 90], longitude *not* wrapped —
/// pass values already in [-180, 180] or call `wrapped()`.
class LatLon {
public:
    constexpr LatLon() noexcept = default;

    /// Construct from explicitly named values (safer than positional).
    static constexpr LatLon from_lat_lon(double lat_deg, double lon_deg) noexcept {
        return LatLon{lat_deg, lon_deg};
    }

    [[nodiscard]] constexpr double lat() const noexcept { return lat_; }
    [[nodiscard]] constexpr double lon() const noexcept { return lon_; }

    [[nodiscard]] bool is_finite() const noexcept {
        return std::isfinite(lat_) && std::isfinite(lon_);
    }

    [[nodiscard]] bool is_valid() const noexcept {
        return is_finite() && lat_ >= -90.0 && lat_ <= 90.0
            && lon_ >= -180.0 && lon_ <= 180.0;
    }

    /// Returns a copy with longitude wrapped into [-180, 180].
    [[nodiscard]] LatLon wrapped() const noexcept;

    /// Great-circle distance to another point, in meters (Haversine on WGS84 mean radius).
    /// Not a geodesic distance — for sub-meter accuracy use `geodesic_distance_m()`
    /// (TBD, will live in modules/geodesy with GeographicLib).
    [[nodiscard]] double haversine_distance_m(const LatLon& other) const noexcept;

    /// Format as "lat,lon" with 7 decimals (≈1 cm precision at the equator).
    [[nodiscard]] std::string to_string() const;

    friend constexpr auto operator<=>(const LatLon&, const LatLon&) = default;

private:
    constexpr LatLon(double lat, double lon) noexcept : lat_{lat}, lon_{lon} {}

    double lat_ = 0.0;
    double lon_ = 0.0;
};

}  // namespace xps::core_types
