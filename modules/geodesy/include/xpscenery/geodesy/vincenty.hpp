#pragma once
// xpscenery -- geodesy/vincenty.hpp
//
// Vincenty's inverse formula on the WGS84 ellipsoid. Returns distance
// in meters and initial / final bearings in degrees [0, 360). Accuracy
// is sub-millimeter for distances up to ~20,000 km; nearly-antipodal
// points may fail to converge, in which case we return an error.

#include <cstdint>
#include <expected>
#include <string>

#include "xpscenery/core_types/lat_lon.hpp"

namespace xps::geodesy {

struct InverseResult {
    double distance_m        = 0.0;
    double initial_bearing_deg = 0.0;  ///< azimuth from `a` towards `b`
    double final_bearing_deg   = 0.0;  ///< azimuth at `b` departing from `a`
    int    iterations         = 0;
};

/// Vincenty inverse problem. Input points must be valid WGS84 lat/lon.
/// Returns error if either point is invalid or iteration fails to
/// converge (antipodal / near-antipodal pair).
[[nodiscard]] std::expected<InverseResult, std::string>
vincenty_inverse(const xps::core_types::LatLon& a,
                 const xps::core_types::LatLon& b) noexcept;

}  // namespace xps::geodesy
