#pragma once
// xpscenery — mesh_core/delaunay.hpp
//
// Bowyer-Watson Delaunay triangulace nad množinou 2D bodů. Vstup: N
// bodů (Point2 nebo Point3 — z-složka se přenese do výstupního meshe).
// Výstup: TriangleMesh s N vrcholy a trojúhelníky v CCW orientaci.
//
// Algoritmus (klasický O(N log N) v praxi, O(N²) worst case):
//   1. Obal všechny body do super-trojúhelníku dostatečně velkého
//      na to, aby nikdy neležel v circumcircle žádného vstupního bodu.
//   2. Pro každý bod P:
//      - najdi všechny „bad" trojúhelníky (P je v jejich circumcircle),
//      - vyřízni je z meshe → dostaneš polygonální díru,
//      - re-triangulace: spoj P s každou hranou polygonu.
//   3. Smaž všechny trojúhelníky obsahující super-vertex.
//
// Fáze 3A: naivní scan přes všechny trojúhelníky v bodě 2. O(N²)
// celkem, ale je to jednoduché, samostatné, testovatelné. Fáze 3B
// přidá spatial index (grid / quadtree) pro sub-kvadratický chod.

#include "triangle_mesh.hpp"
#include "point.hpp"

#include <expected>
#include <span>
#include <string>

namespace xps::mesh_core
{

    struct DelaunayOptions
    {
        /// Snaze odstranit duplikátní body (v 1e-12 toleranci).
        bool deduplicate_points = true;
    };

    /// Triangulace 2D bodů. Z-souřadnice se zachová (interpolace ne).
    /// Selže pro < 3 body nebo pokud všechny body jsou kolineární.
    [[nodiscard]] std::expected<TriangleMesh, std::string>
    delaunay_triangulate_2d(std::span<const Point3> points,
                            const DelaunayOptions &opts = {});

    /// Varianta čistě 2D (z=0).
    [[nodiscard]] std::expected<TriangleMesh, std::string>
    delaunay_triangulate_2d(std::span<const Point2> points,
                            const DelaunayOptions &opts = {});

} // namespace xps::mesh_core
