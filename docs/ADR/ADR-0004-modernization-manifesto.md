# ADR-0004 — Modernization Manifesto (Phase 1)

**Status:** Accepted
**Date:** 2026-03-12
**Supersedes:** partial: implicit "1:1 port of xptools" assumption in initial PLAN.

## Context

The legacy `xptools260` codebase is ≈20 years old. It is a pragmatic,
battle-tested corpus — and also carries all the weight of its era:
raw pointer APIs, ad-hoc string parsers, hand-rolled arg parsing,
`(lon,lat)` / `(lat,lon)` order confusion, scattered I/O error handling
via `fprintf(stderr, ...); return -1;`, globals, monolithic compilation
units and home-grown configuration schemas.

The user explicitly mandated ("většina kódu je velmi stará, staré
knihovny a funkce, chci vše modernizovat, zrychlit, zpřesnit, reálnější
a kvalitnější výsledky, klidně změnit strukturu pro logičtější postupy,
více volnosti pro tvůrce scenérií") that xpscenery should **not** be a
1:1 port. It should be a modern Windows-first rewrite that **selectively
reuses algorithms** from xptools where they are correct, but presents a
clean, typed, testable API surface on top.

## Decision

1. **C++23.** MSVC 19.50 (VS 2026) fully supports `/std:c++latest` ≈ C++23.
   We use `std::expected` for recoverable errors, `std::print`/`std::format`
   for output, `<ranges>`, `<filesystem>`, `<span>`, `<numbers>`.
2. **Expected over exceptions** on the expected-error path (parse failures,
   missing files, invalid config). Exceptions remain reserved for truly
   exceptional states (OOM, programmer bugs).
3. **Strong domain types.** `LatLon`, `TileCoord`, `BoundingBox` replace
   raw `double lat, lon` tuples. Construction is by named factory
   (`LatLon::from_lat_lon`, `TileCoord::parse("+50+015")`), preventing the
   classic (lon,lat) mix-up.
4. **Schema-versioned configs.** `TileConfig` carries an explicit
   `schema_version` field; unknown fields are preserved on round-trip
   (forward-compat). Unknown `schema_version` → hard error.
5. **CLI11 over hand-rolled arg parsing.** Subcommands are registered by
   each domain module (`register_version`, `register_inspect`,
   `register_validate`…) so they compose, test, and document themselves.
6. **spdlog async** with file+console sinks. Every module gets a named
   logger via `xps::io_logging::get("core_types")`.
7. **Atomic file writes.** All producer code uses `write_*_atomic`
   (temp file + rename) so a crash mid-write never leaves a torn output
   in `Custom Scenery/`.
8. **One module, one folder, one test directory.** Modules live under
   `modules/<name>/` with `include/xpscenery/<name>/*.hpp` and `src/*.cpp`.
   Tests mirror the structure under `tests/unit/<name>/`.
9. **STATIC libraries in v0.x** (see ADR-0003). We keep DLL transition
   possible but do not ship DLLs until the ABI stabilises in v1.0.
10. **Freedom for scenery authors.** The config file is typed but permissive:
    authors can add new `layers[]` of arbitrary `kind`, set per-layer
    `priority`, `enabled`, `srs`, `aoi`, and override `meshing` /
    `xp_export` knobs without touching the engine.

## Consequences

- The baseline API is new; legacy xptools command-line contracts (e.g.
  `DSFTool --text2dsf ...`) will be re-exposed as explicit subcommands
  with named options, not as positional relics. A compatibility shim
  `xps-compat` can be added later if needed.
- `std::expected` unwinding at the module boundary makes error messages
  actionable ("tile latitude out of range [-89,89]: 90") rather than a
  `return -1`.
- The project is now unambiguously Windows-first; POSIX support will be
  added only when an algorithm that diverges needs it.

## Status of this ADR

Implemented in the following new modules (commit introducing this ADR):
`core_types`, `io_logging`, `io_filesystem`, `io_config`, `app_cli`.
All new modules have unit tests (372 assertions at introduction).
