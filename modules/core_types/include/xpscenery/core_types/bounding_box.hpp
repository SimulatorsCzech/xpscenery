#pragma once
// xpscenery — core_types/bounding_box.hpp
//
// Axis-aligned bounding box in WGS84 degrees. Does NOT cross the
// antimeridian; a region crossing it must be represented as two boxes
// (callers must split upstream). Empty box is represented by min > max.

#include <algorithm>
#include <compare>
#include <optional>
#include <string>

#include "xpscenery/core_types/lat_lon.hpp"

namespace xps::core_types {

class BoundingBox {
public:
    constexpr BoundingBox() noexcept = default;

    static constexpr BoundingBox from_corners(const LatLon& sw, const LatLon& ne) noexcept {
        return BoundingBox{sw, ne};
    }

    /// Build from a single point (min == max).
    static constexpr BoundingBox from_point(const LatLon& p) noexcept {
        return BoundingBox{p, p};
    }

    [[nodiscard]] constexpr const LatLon& sw() const noexcept { return sw_; }
    [[nodiscard]] constexpr const LatLon& ne() const noexcept { return ne_; }

    [[nodiscard]] constexpr bool is_empty() const noexcept {
        return sw_.lat() > ne_.lat() || sw_.lon() > ne_.lon();
    }

    [[nodiscard]] bool contains(const LatLon& p) const noexcept {
        return p.lat() >= sw_.lat() && p.lat() <= ne_.lat()
            && p.lon() >= sw_.lon() && p.lon() <= ne_.lon();
    }

    [[nodiscard]] bool intersects(const BoundingBox& other) const noexcept {
        return !(is_empty() || other.is_empty()
                 || ne_.lat() < other.sw_.lat()
                 || sw_.lat() > other.ne_.lat()
                 || ne_.lon() < other.sw_.lon()
                 || sw_.lon() > other.ne_.lon());
    }

    /// Expand the box to include a point (no-op if already inside).
    BoundingBox& expand_to_include(const LatLon& p) noexcept;

    /// Width/height in degrees (always non-negative; 0 if empty).
    [[nodiscard]] double width_deg()  const noexcept;
    [[nodiscard]] double height_deg() const noexcept;

    [[nodiscard]] std::string to_string() const;

    friend constexpr auto operator<=>(const BoundingBox&, const BoundingBox&) = default;

private:
    constexpr BoundingBox(const LatLon& sw, const LatLon& ne) noexcept
        : sw_{sw}, ne_{ne} {}

    LatLon sw_{};
    LatLon ne_{LatLon::from_lat_lon(-1.0, -1.0)};  // empty by default
};

}  // namespace xps::core_types
