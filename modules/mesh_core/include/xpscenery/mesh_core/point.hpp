#pragma once
// xpscenery — mesh_core/point.hpp
//
// 2D / 3D body s double-precision souřadnicemi. Drženo jako POD pro
// snadnou binární serializaci a rychlé iterace ve vnitřních smyčkách
// Delaunay algoritmu. Aritmetické operátory minimálně potřebné pro
// geometrické predikáty — žádné heavy-weight funkce.

#include <cstddef>
#include <cstdint>

namespace xps::mesh_core
{

    struct Point2
    {
        double x = 0.0;
        double y = 0.0;

        [[nodiscard]] constexpr Point2 operator+(const Point2 &o) const noexcept
        {
            return {x + o.x, y + o.y};
        }
        [[nodiscard]] constexpr Point2 operator-(const Point2 &o) const noexcept
        {
            return {x - o.x, y - o.y};
        }
        [[nodiscard]] constexpr Point2 operator*(double s) const noexcept
        {
            return {x * s, y * s};
        }
        [[nodiscard]] constexpr bool operator==(const Point2 &o) const noexcept
        {
            return x == o.x && y == o.y;
        }
    };

    struct Point3
    {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;

        [[nodiscard]] constexpr Point3 operator+(const Point3 &o) const noexcept
        {
            return {x + o.x, y + o.y, z + o.z};
        }
        [[nodiscard]] constexpr Point3 operator-(const Point3 &o) const noexcept
        {
            return {x - o.x, y - o.y, z - o.z};
        }
    };

    /// Axis-aligned bounding box in 2D.
    struct Bbox2
    {
        double x_min = 0, y_min = 0, x_max = 0, y_max = 0;

        [[nodiscard]] constexpr double width()  const noexcept { return x_max - x_min; }
        [[nodiscard]] constexpr double height() const noexcept { return y_max - y_min; }
        [[nodiscard]] constexpr bool   empty()  const noexcept { return x_max <= x_min || y_max <= y_min; }

        [[nodiscard]] constexpr bool contains(const Point2 &p) const noexcept
        {
            return p.x >= x_min && p.x <= x_max && p.y >= y_min && p.y <= y_max;
        }
    };

    /// Spočítá Bbox2 z N bodů. Vrací empty pro count==0.
    [[nodiscard]] Bbox2 bbox_of(const Point2 *pts, std::size_t count) noexcept;

} // namespace xps::mesh_core
