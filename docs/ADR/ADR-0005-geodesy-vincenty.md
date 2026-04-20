# ADR-0005 — Geodesy: Vincenty inverse, no GeographicLib (v0.x)

**Status:** Accepted
**Date:** 2026-04-21

## Context

xpscenery needs sub-meter-accurate geodetic distance and bearing
calculations for tasks such as:

- computing runway end coordinates from a heading + length,
- measuring inter-object spacing in autogen/zoning rules,
- validating tile boundaries against WGS84 distance limits,
- driving the `xpscenery-cli distance` diagnostic command.

The classic Haversine formula (already available on `LatLon::haversine_distance_m`)
is only accurate to ~0.3 % on WGS84 because it assumes a sphere. For
the geodesic problem on the ellipsoid we need Vincenty's inverse formula
or Karney's more recent `GeographicLib::Geodesic`.

## Decision

For the **v0.x** line we ship a **self-contained Vincenty inverse**
implementation in `modules/geodesy/` (WGS84 ellipsoid, 200-iteration cap,
1e-12 relative convergence tolerance). No external dependency is added.

We explicitly decline to pull in **GeographicLib** at this stage, because:

1. GeographicLib has no first-class vcpkg port on Windows with current
   MSVC 19.50 — every extra dependency is extra CI surface.
2. Vincenty's inverse is numerically well-understood and converges for
   all realistic pairs we will encounter (the Prague–Berlin, tile-corner,
   airport-scale queries). The pathological antipodal case triggers a
   clean `std::unexpected("Vincenty did not converge …")`.
3. Adding GeographicLib later is a zero-cost replacement: the public
   API is a free function returning `std::expected<InverseResult, std::string>`,
   and call sites (currently only `cmd_distance`) don't leak the backend.

## Consequences

- **Zero new third-party deps** in v0.x.
- Accuracy: ~1 mm for typical distances up to a few thousand km. Meets
  all current scenery authoring needs.
- **Migration path to GeographicLib** (when we need Karney's method for
  near-antipodal points, or the direct problem at scale): swap
  `modules/geodesy/src/vincenty.cpp` to delegate to
  `GeographicLib::Geodesic::WGS84().Inverse(...)`. The public header
  stays stable. This is tracked for Phase 3+ (full scenery build
  pipeline), not Phase 1.

## Alternatives considered

- **Haversine only** — rejected, 0.3 % error is visible in meter-scale
  diagnostics and unacceptable for runway geometry.
- **GeographicLib now** — deferred, not rejected. Re-evaluate at v0.5
  when the `xpscenery-cli build` command lands and geodesy is on the
  critical path for production outputs.
- **Karney by hand** — more complex than Vincenty, no tangible benefit
  for our current use cases; if we need it we should just adopt the
  canonical library.

## References

- T. Vincenty, "Direct and Inverse Solutions of Geodesics on the
  Ellipsoid with Application of Nested Equations," Survey Review,
  Vol. 23, No. 176, April 1975.
- https://geographiclib.sourceforge.io/C++/doc/ (reference implementation).
