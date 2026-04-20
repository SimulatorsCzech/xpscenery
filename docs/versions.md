# Pinned dependency versions

**Last updated:** 2026-04-20
**Policy:** bleeding-edge preview where safe; stable where critical (GMP/MPFR/CGAL numerics).

Monthly review: bump versions, run full regression suite, commit updated baseline.

## Toolchain

| Tool | Version | Source |
|---|---|---|
| Visual Studio | 2026 Preview 2 (or 2022 17.12 LTS fallback) | Microsoft |
| MSVC | 19.50 (Preview) / 19.40 (LTS) | bundled with VS |
| CMake | 4.2 RC | [cmake.org/download](https://cmake.org/download/) |
| Ninja | 1.12 | bundled with VS |
| vcpkg | rolling master | [github.com/microsoft/vcpkg](https://github.com/microsoft/vcpkg) |
| Git | 2.45+ | gitforwindows.org |
| Python (build scripts) | 3.13 | python.org |

## C++ libraries (via vcpkg manifest)

| Library | Pinned | Notes |
|---|---|---|
| Boost | ≥ 1.88 (1.89 beta when available) | Graph, Geometry, Polygon, Filesystem, ProgramOptions |
| CGAL | ≥ 6.0 (6.1 RC when available) | header-only; Lazy_exact_nt numerics |
| GDAL | ≥ 3.11 (3.12 RC when available) | raster + vector I/O |
| GMP | ≥ 6.3.0 | stable — arbitrary precision |
| MPFR | ≥ 4.2.1 | stable — floating point |
| fmt | ≥ 11.0 | string formatting |
| spdlog | ≥ 1.15 | logging |
| nlohmann-json | ≥ 3.12 | config/project files |
| tinyxml2 | ≥ 11.0 | XES / WED parsing |
| zlib | ≥ 1.3.1 | compression |
| Catch2 | ≥ 3.8 | testing framework |

## UI (feature `ui`)

| Library | Pinned | Notes |
|---|---|---|
| Qt Base | ≥ 6.9 (6.10 Preview target) | Core, Gui, Widgets, Concurrent |
| Qt Declarative | ≥ 6.9 | QML, Qt Quick |
| Qt Quick 3D | ≥ 6.9 | 3D preview |
| Qt Location | ≥ 6.9 | fallback map (until MapLibre Native integrated) |
| Qt Tools | ≥ 6.9 | Linguist, assistant |
| Qt Shader Tools | ≥ 6.9 | shader compilation for Quick 3D |

## Version bump policy

1. Open a PR titled `deps: bump <library> to <version>`.
2. CI must pass across all presets (`windows-msvc`, `windows-clang-cl`, `windows-msvc-asan`).
3. Regression suite must produce bit-identical DSF output (once Phase 1 port exists).
4. Update the `Last updated` date at the top of this file.
