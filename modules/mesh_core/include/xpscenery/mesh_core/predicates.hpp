#pragma once
// xpscenery — mesh_core/predicates.hpp
//
// Geometrické predikáty nad double precision. Fáze 3A používá naivní
// determinanty (fast, ne robustní na degenerate vstupy). Přesné
// robustní predikáty (Shewchuk, CGAL) přijdou ve Fázi 3B jako backend.
//
// orient_2d(a, b, c):
//   > 0   → c leží vlevo od AB (CCW)
//   < 0   → c leží vpravo od AB (CW)
//   = 0   → kolineární
//
// in_circle_2d(a, b, c, d):
//   Vstup ABC v CCW orientaci.
//   > 0   → d uvnitř circumcircle(ABC)
//   < 0   → d venku
//   = 0   → na kružnici

#include "point.hpp"

namespace xps::mesh_core
{

    [[nodiscard]] inline double orient_2d(const Point2 &a, const Point2 &b,
                                          const Point2 &c) noexcept
    {
        // | ax-cx  ay-cy |
        // | bx-cx  by-cy |
        const double acx = a.x - c.x;
        const double acy = a.y - c.y;
        const double bcx = b.x - c.x;
        const double bcy = b.y - c.y;
        return acx * bcy - acy * bcx;
    }

    /// Incircle test pomocí 3×3 determinantu (s odečteným D pro číselnou
    /// stabilitu). Vstup očekává ABC v CCW orientaci.
    [[nodiscard]] inline double in_circle_2d(
        const Point2 &a, const Point2 &b, const Point2 &c,
        const Point2 &d) noexcept
    {
        const double adx = a.x - d.x, ady = a.y - d.y;
        const double bdx = b.x - d.x, bdy = b.y - d.y;
        const double cdx = c.x - d.x, cdy = c.y - d.y;

        const double ad2 = adx * adx + ady * ady;
        const double bd2 = bdx * bdx + bdy * bdy;
        const double cd2 = cdx * cdx + cdy * cdy;

        // | adx ady ad² |
        // | bdx bdy bd² |
        // | cdx cdy cd² |
        return adx * (bdy * cd2 - bd2 * cdy)
             - ady * (bdx * cd2 - bd2 * cdx)
             + ad2 * (bdx * cdy - bdy * cdx);
    }

} // namespace xps::mesh_core
