#include "xpscenery/core_types/bounding_box.hpp"

#include <algorithm>
#include <format>

namespace xps::core_types {

BoundingBox& BoundingBox::expand_to_include(const LatLon& p) noexcept {
    if (is_empty()) {
        sw_ = p;
        ne_ = p;
        return *this;
    }
    sw_ = LatLon::from_lat_lon(std::min(sw_.lat(), p.lat()),
                               std::min(sw_.lon(), p.lon()));
    ne_ = LatLon::from_lat_lon(std::max(ne_.lat(), p.lat()),
                               std::max(ne_.lon(), p.lon()));
    return *this;
}

double BoundingBox::width_deg() const noexcept {
    return is_empty() ? 0.0 : ne_.lon() - sw_.lon();
}

double BoundingBox::height_deg() const noexcept {
    return is_empty() ? 0.0 : ne_.lat() - sw_.lat();
}

std::string BoundingBox::to_string() const {
    if (is_empty()) return "<empty>";
    return std::format("[sw={} ne={}]", sw_.to_string(), ne_.to_string());
}

}  // namespace xps::core_types
