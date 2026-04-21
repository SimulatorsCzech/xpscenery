// xpscenery/tests/unit/mesh_core/test_delaunay.cpp
#include "xpscenery/mesh_core/delaunay.hpp"
#include "xpscenery/mesh_core/predicates.hpp"

#include <catch2/catch_test_macros.hpp>

#include <set>
#include <span>
#include <vector>

using namespace xps::mesh_core;

namespace
{
    std::set<std::tuple<VertexId, VertexId, VertexId>>
    triangle_vertex_sets(const TriangleMesh &mesh)
    {
        std::set<std::tuple<VertexId, VertexId, VertexId>> s;
        for (const auto &t : mesh.triangles())
        {
            VertexId a = t.v0, b = t.v1, c = t.v2;
            if (a > b) std::swap(a, b);
            if (b > c) std::swap(b, c);
            if (a > b) std::swap(a, b);
            s.insert({a, b, c});
        }
        return s;
    }
}

TEST_CASE("orient_2d: CCW/CW/collinear", "[mesh_core][predicates]")
{
    REQUIRE(orient_2d({0, 0}, {1, 0}, {0, 1}) > 0.0);   // CCW
    REQUIRE(orient_2d({0, 0}, {0, 1}, {1, 0}) < 0.0);   // CW
    REQUIRE(orient_2d({0, 0}, {1, 1}, {2, 2}) == 0.0);  // collinear
}

TEST_CASE("in_circle_2d: inside/outside", "[mesh_core][predicates]")
{
    // Unit circle triangle: (1,0), (0,1), (-1,0) CCW.
    Point2 a{1, 0}, b{0, 1}, c{-1, 0};
    REQUIRE(in_circle_2d(a, b, c, {0.0, 0.0}) > 0.0);   // inside
    REQUIRE(in_circle_2d(a, b, c, {2.0, 2.0}) < 0.0);   // outside
}

TEST_CASE("delaunay: unit square + centre", "[mesh_core][delaunay]")
{
    std::vector<Point3> pts{
        {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0.5, 0.5, 0}};
    auto r = delaunay_triangulate_2d(std::span<const Point3>{pts});
    REQUIRE(r);
    const auto &m = *r;
    REQUIRE(m.vertex_count() == 5);
    // Čtverec s bodem uprostřed má přesně 4 Delaunay trojúhelníky.
    REQUIRE(m.triangle_count() == 4);
    // Každý trojúhelník musí obsahovat středový vrchol (index 4).
    for (const auto &t : m.triangles())
        REQUIRE((t.v0 == 4 || t.v1 == 4 || t.v2 == 4));
}

TEST_CASE("delaunay: regular triangle = 1 triangle", "[mesh_core][delaunay]")
{
    std::vector<Point2> pts{{0, 0}, {1, 0}, {0.5, 1.0}};
    auto r = delaunay_triangulate_2d(std::span<const Point2>{pts});
    REQUIRE(r);
    REQUIRE(r->triangle_count() == 1);
}

TEST_CASE("delaunay: < 3 points fails", "[mesh_core][delaunay]")
{
    std::vector<Point2> pts{{0, 0}, {1, 1}};
    auto r = delaunay_triangulate_2d(std::span<const Point2>{pts});
    REQUIRE_FALSE(r);
}

TEST_CASE("delaunay: collinear points fail", "[mesh_core][delaunay]")
{
    std::vector<Point2> pts{{0, 0}, {1, 0}, {2, 0}, {3, 0}};
    auto r = delaunay_triangulate_2d(std::span<const Point2>{pts});
    // Kolineární body: buď selže, nebo vrátí prázdnou sadu trojúhelníků.
    if (r)
        REQUIRE(r->triangle_count() == 0);
}

TEST_CASE("delaunay: deduplicate handles equal points", "[mesh_core][delaunay]")
{
    std::vector<Point3> pts{
        {0, 0, 0}, {0, 0, 0}, {1, 0, 0}, {0.5, 1, 0}};
    auto r = delaunay_triangulate_2d(std::span<const Point3>{pts});
    REQUIRE(r);
    // Po deduplikaci zbyly 3 body → 1 trojúhelník.
    REQUIRE(r->triangle_count() == 1);
}

TEST_CASE("delaunay: empty triangulation is valid CCW", "[mesh_core][delaunay]")
{
    // 10 bodů na mřížce — výstupní trojúhelníky musí být všechny CCW.
    std::vector<Point3> pts;
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 3; ++x)
            pts.push_back({double(x), double(y), 0});
    auto r = delaunay_triangulate_2d(std::span<const Point3>{pts});
    REQUIRE(r);
    for (const auto &t : r->triangles())
    {
        const auto &v0 = r->vertices()[t.v0];
        const auto &v1 = r->vertices()[t.v1];
        const auto &v2 = r->vertices()[t.v2];
        REQUIRE(orient_2d({v0.x, v0.y}, {v1.x, v1.y}, {v2.x, v2.y}) > 0.0);
    }
}
