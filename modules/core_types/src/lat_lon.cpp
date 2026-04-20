#include "xpscenery/core_types/lat_lon.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>

namespace xps::core_types {

namespace {

inline constexpr double kEarthRadiusMean = 6'371'008.8;  // meters, IUGG mean

inline constexpr double deg_to_rad(double deg) noexcept {
    return deg * std::numbers::pi / 180.0;
}

}  // namespace

LatLon LatLon::wrapped() const noexcept {
    double lon = std::fmod(lon_ + 180.0, 360.0);
    if (lon < 0.0) lon += 360.0;
    lon -= 180.0;
    // Clamp lat into [-90, 90] — anything else is an error upstream.
    double lat = std::clamp(lat_, -90.0, 90.0);
    return LatLon::from_lat_lon(lat, lon);
}

double LatLon::haversine_distance_m(const LatLon& other) const noexcept {
    const double phi1 = deg_to_rad(lat_);
    const double phi2 = deg_to_rad(other.lat_);
    const double dphi = deg_to_rad(other.lat_ - lat_);
    const double dlam = deg_to_rad(other.lon_ - lon_);

    const double s_dphi = std::sin(dphi * 0.5);
    const double s_dlam = std::sin(dlam * 0.5);
    const double a = s_dphi * s_dphi + std::cos(phi1) * std::cos(phi2) * s_dlam * s_dlam;
    const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return kEarthRadiusMean * c;
}

std::string LatLon::to_string() const {
    return std::format("{:.7f},{:.7f}", lat_, lon_);
}

}  // namespace xps::core_types
