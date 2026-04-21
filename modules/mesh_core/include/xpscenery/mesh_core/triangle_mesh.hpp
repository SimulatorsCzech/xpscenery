#pragma once
// xpscenery — mesh_core/triangle_mesh.hpp
//
// Trojúhelníková síť (TIN) nad N body v rovině (Point2) s optionální
// Z-souřadnicí (elevace). Drží pouze konektivitu + pole bodů; nečte
// ani nezapisuje soubory — I/O je v `mesh_io`.
//
// Reprezentace:
//   vertices_  : lineární pole Point3 (z = 0 pokud 2D), indexováno VertexId.
//   triangles_ : pole Triangle = 3 × VertexId v CCW (counter-clockwise)
//                orientaci pro standardní pravotočivý systém.
//
// Design choices:
//   - žádný half-edge graf (Fáze 3A) — pro základní Delaunay/export stačí
//     pole trojúhelníků. Half-edge přijde, jakmile budou potřeba
//     neighbor queries pro flip-based constrained Delaunay.
//   - žádné allocation pool — std::vector s reserve() stačí.

#include "point.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace xps::mesh_core
{

    using VertexId   = std::uint32_t;
    using TriangleId = std::uint32_t;

    inline constexpr VertexId kInvalidVertex = 0xFFFFFFFFu;

    struct Triangle
    {
        VertexId v0 = kInvalidVertex;
        VertexId v1 = kInvalidVertex;
        VertexId v2 = kInvalidVertex;

        [[nodiscard]] constexpr bool is_valid() const noexcept
        {
            return v0 != kInvalidVertex && v1 != kInvalidVertex
                && v2 != kInvalidVertex && v0 != v1 && v1 != v2 && v0 != v2;
        }
    };

    class TriangleMesh
    {
    public:
        TriangleMesh() = default;

        void clear() noexcept { vertices_.clear(); triangles_.clear(); }
        void reserve(std::size_t verts, std::size_t tris)
        {
            vertices_.reserve(verts);
            triangles_.reserve(tris);
        }

        VertexId add_vertex(const Point3 &p)
        {
            vertices_.push_back(p);
            return static_cast<VertexId>(vertices_.size() - 1);
        }
        VertexId add_vertex_xy(double x, double y, double z = 0.0)
        {
            vertices_.push_back({x, y, z});
            return static_cast<VertexId>(vertices_.size() - 1);
        }

        TriangleId add_triangle(VertexId a, VertexId b, VertexId c)
        {
            triangles_.push_back({a, b, c});
            return static_cast<TriangleId>(triangles_.size() - 1);
        }

        [[nodiscard]] std::size_t vertex_count()   const noexcept { return vertices_.size(); }
        [[nodiscard]] std::size_t triangle_count() const noexcept { return triangles_.size(); }

        [[nodiscard]] const std::vector<Point3>   &vertices()  const noexcept { return vertices_; }
        [[nodiscard]] const std::vector<Triangle> &triangles() const noexcept { return triangles_; }

        [[nodiscard]] std::vector<Point3>   &vertices_mut()  noexcept { return vertices_; }
        [[nodiscard]] std::vector<Triangle> &triangles_mut() noexcept { return triangles_; }

    private:
        std::vector<Point3>   vertices_;
        std::vector<Triangle> triangles_;
    };

} // namespace xps::mesh_core
