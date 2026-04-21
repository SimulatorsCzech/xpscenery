// xpscenery — mesh_core/delaunay.cpp  (Phase 3A, hand-rolled Bowyer-Watson)
//
// Důležité implementační poznámky:
//   - Edge se ukládá jako pár (min(a,b), max(a,b)) pro snadné porovnání.
//   - „Shared edge" mezi dvěma bad triangles → není polygon boundary
//     (z díry ven neukazuje). Detekujeme scanem: edge se v seznamu bad
//     trojúhelníků objeví jen jednou = boundary.
//   - Super-triangle je tak velký, aby žádný vstupní bod nebyl v jeho
//     circumcircle. Používáme bbox vstupu, zvětšený 20×.

#include "xpscenery/mesh_core/delaunay.hpp"
#include "xpscenery/mesh_core/predicates.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <unordered_set>
#include <vector>

namespace xps::mesh_core
{

    Bbox2 bbox_of(const Point2 *pts, std::size_t count) noexcept
    {
        if (count == 0) return {};
        Bbox2 b{pts[0].x, pts[0].y, pts[0].x, pts[0].y};
        for (std::size_t i = 1; i < count; ++i)
        {
            if (pts[i].x < b.x_min) b.x_min = pts[i].x;
            if (pts[i].x > b.x_max) b.x_max = pts[i].x;
            if (pts[i].y < b.y_min) b.y_min = pts[i].y;
            if (pts[i].y > b.y_max) b.y_max = pts[i].y;
        }
        return b;
    }

    namespace
    {
        struct Edge
        {
            VertexId a, b;
            [[nodiscard]] bool operator==(const Edge &o) const noexcept
            {
                return a == o.a && b == o.b;
            }
        };
        struct EdgeHash
        {
            std::size_t operator()(const Edge &e) const noexcept
            {
                return (static_cast<std::size_t>(e.a) << 32)
                     ^  static_cast<std::size_t>(e.b);
            }
        };

        Edge make_edge(VertexId a, VertexId b) noexcept
        {
            return (a < b) ? Edge{a, b} : Edge{b, a};
        }

        // Make sure ABC is CCW; swap v1/v2 otherwise.
        void ensure_ccw(const std::vector<Point2> &pts, Triangle &t) noexcept
        {
            if (orient_2d(pts[t.v0], pts[t.v1], pts[t.v2]) < 0.0)
                std::swap(t.v1, t.v2);
        }

        std::expected<TriangleMesh, std::string>
        triangulate_impl(std::vector<Point2> pts_xy,
                         const std::vector<double> &zs,
                         const DelaunayOptions &opts)
        {
            const std::size_t n = pts_xy.size();
            if (n < 3)
                return std::unexpected(std::format(
                    "delaunay: potřeba ≥ 3 bodů, dostáno {}", n));

            // Optional deduplicate (stable keep-first).
            std::vector<std::size_t> orig_index(n);
            for (std::size_t i = 0; i < n; ++i) orig_index[i] = i;
            if (opts.deduplicate_points)
            {
                constexpr double eps = 1e-12;
                std::vector<Point2> unique_pts;
                std::vector<std::size_t> unique_orig;
                unique_pts.reserve(n);
                unique_orig.reserve(n);
                for (std::size_t i = 0; i < n; ++i)
                {
                    bool dup = false;
                    for (const auto &q : unique_pts)
                    {
                        if (std::fabs(q.x - pts_xy[i].x) < eps
                         && std::fabs(q.y - pts_xy[i].y) < eps)
                        { dup = true; break; }
                    }
                    if (!dup)
                    {
                        unique_pts.push_back(pts_xy[i]);
                        unique_orig.push_back(i);
                    }
                }
                pts_xy    = std::move(unique_pts);
                orig_index = std::move(unique_orig);
            }

            const std::size_t m = pts_xy.size();
            if (m < 3)
                return std::unexpected(std::format(
                    "delaunay: po odstranění duplicit zbyly < 3 body ({})", m));

            const Bbox2 bb = bbox_of(pts_xy.data(), pts_xy.size());
            const double dx = bb.width(), dy = bb.height();
            const double d  = std::max(dx, dy);
            if (!(d > 0.0))
                return std::unexpected("delaunay: všechny body kolineární (nulový bbox)");

            const double cx = (bb.x_min + bb.x_max) * 0.5;
            const double cy = (bb.y_min + bb.y_max) * 0.5;
            const double big = d * 20.0;

            // Three virtual super-vertices at indices m, m+1, m+2.
            std::vector<Point2> work = pts_xy;
            work.push_back({cx - 2.0 * big, cy - big});         // s0
            work.push_back({cx + 2.0 * big, cy - big});         // s1
            work.push_back({cx,              cy + 2.0 * big});  // s2
            const VertexId s0 = static_cast<VertexId>(m);
            const VertexId s1 = static_cast<VertexId>(m + 1);
            const VertexId s2 = static_cast<VertexId>(m + 2);

            std::vector<Triangle> tris;
            tris.reserve(4 * m);
            tris.push_back({s0, s1, s2});
            ensure_ccw(work, tris.back());

            // Incremental insertion.
            std::vector<std::uint8_t> is_bad;
            std::vector<Edge> poly_edges;
            poly_edges.reserve(64);

            for (VertexId pi = 0; pi < static_cast<VertexId>(m); ++pi)
            {
                const Point2 &p = work[pi];
                is_bad.assign(tris.size(), 0);

                // 1. Mark bad triangles.
                for (std::size_t ti = 0; ti < tris.size(); ++ti)
                {
                    const Triangle &t = tris[ti];
                    if (in_circle_2d(work[t.v0], work[t.v1], work[t.v2], p) > 0.0)
                        is_bad[ti] = 1;
                }

                // 2. Collect boundary polygon = edges belonging to exactly
                //    one bad triangle.
                poly_edges.clear();
                std::unordered_set<Edge, EdgeHash> seen;
                std::unordered_set<Edge, EdgeHash> shared;
                for (std::size_t ti = 0; ti < tris.size(); ++ti)
                {
                    if (!is_bad[ti]) continue;
                    const Triangle &t = tris[ti];
                    const Edge e01 = make_edge(t.v0, t.v1);
                    const Edge e12 = make_edge(t.v1, t.v2);
                    const Edge e20 = make_edge(t.v2, t.v0);
                    for (const Edge &e : {e01, e12, e20})
                    {
                        if (!seen.insert(e).second) shared.insert(e);
                    }
                }
                for (std::size_t ti = 0; ti < tris.size(); ++ti)
                {
                    if (!is_bad[ti]) continue;
                    const Triangle &t = tris[ti];
                    const Edge es[3] = {
                        make_edge(t.v0, t.v1),
                        make_edge(t.v1, t.v2),
                        make_edge(t.v2, t.v0),
                    };
                    for (const Edge &e : es)
                        if (shared.find(e) == shared.end())
                            poly_edges.push_back(e);
                }

                // 3. Remove bad triangles.
                std::size_t w = 0;
                for (std::size_t ti = 0; ti < tris.size(); ++ti)
                    if (!is_bad[ti]) tris[w++] = tris[ti];
                tris.resize(w);

                // 4. Connect p to each polygon edge.
                tris.reserve(tris.size() + poly_edges.size());
                for (const Edge &e : poly_edges)
                {
                    Triangle t{e.a, e.b, pi};
                    ensure_ccw(work, t);
                    tris.push_back(t);
                }
            }

            // 5. Strip triangles that reference super-vertices.
            std::vector<Triangle> final_tris;
            final_tris.reserve(tris.size());
            for (const Triangle &t : tris)
            {
                if (t.v0 >= static_cast<VertexId>(m)
                 || t.v1 >= static_cast<VertexId>(m)
                 || t.v2 >= static_cast<VertexId>(m)) continue;
                final_tris.push_back(t);
            }

            TriangleMesh mesh;
            mesh.reserve(m, final_tris.size());
            for (std::size_t i = 0; i < m; ++i)
            {
                const double z = (i < zs.size()) ? zs[orig_index[i]] : 0.0;
                mesh.add_vertex({pts_xy[i].x, pts_xy[i].y, z});
            }
            for (const Triangle &t : final_tris)
                mesh.add_triangle(t.v0, t.v1, t.v2);
            return mesh;
        }
    } // namespace

    std::expected<TriangleMesh, std::string>
    delaunay_triangulate_2d(std::span<const Point3> points,
                            const DelaunayOptions &opts)
    {
        std::vector<Point2> xy;
        xy.reserve(points.size());
        std::vector<double> zs;
        zs.reserve(points.size());
        for (const auto &p : points)
        {
            xy.push_back({p.x, p.y});
            zs.push_back(p.z);
        }
        return triangulate_impl(std::move(xy), zs, opts);
    }

    std::expected<TriangleMesh, std::string>
    delaunay_triangulate_2d(std::span<const Point2> points,
                            const DelaunayOptions &opts)
    {
        std::vector<Point2> xy(points.begin(), points.end());
        return triangulate_impl(std::move(xy), {}, opts);
    }

} // namespace xps::mesh_core
