# ADR-0001 — Use CMake + vcpkg for build & dependency management

**Date:** 2026-04-20
**Status:** Accepted
**Deciders:** project owner

## Context

The existing xptools260 codebase uses three separate build systems:
- Linux/macOS: `Makefile` + `makerules/`
- Windows: Visual Studio `.sln` + `.vcxproj`
- macOS: `SceneryTools.xcodeproj`

Third-party dependencies are pinned via git submodules in `libs/` (Linux/macOS) and a separate prebuilt tree `msvc_libs/` (Windows). This makes Windows builds effectively unreproducible and requires manual updates for each dependency.

For a greenfield Windows-focused project we need:
- **Reproducible builds** on CI and developer machines
- **Single source of truth** for dependency versions
- **First-class Visual Studio 2026 + CLion + VS Code integration**
- **Access to latest preview versions** of deps (CGAL 6.1 RC, GDAL 3.12 RC, Qt 6.10 Preview)

## Decision

- Use **CMake 4.2+** as the sole build system. No generated `.sln` checked in.
- Use **vcpkg in manifest mode** (`vcpkg.json` at repo root) for all C++ dependencies.
- Use **`CMakePresets.json` v6** for named configurations (MSVC, Clang-cl, ASan, Preview).
- Pin `builtin-baseline` in `vcpkg.json` to a specific vcpkg commit SHA for reproducibility.
- `VCPKG_ROOT` env var is required on every dev machine and CI runner.

## Consequences

### Positive
- `cmake --preset=... && cmake --build --preset=...` works identically on every machine.
- Visual Studio 2022/2026 opens the folder directly with full IntelliSense.
- Dependency bumps are one `vcpkg.json` edit + git commit.
- vcpkg manages transitive deps; no manual submodule babysitting.
- Works with VS Code CMake Tools extension out of the box.

### Negative
- Developers must install vcpkg once (`git clone https://github.com/microsoft/vcpkg`).
- First-time configure is slow (30–60 min) while vcpkg builds deps from source.
  - Mitigation: vcpkg binary caching to GitHub Packages / local cache.
- CGAL build in vcpkg occasionally lags upstream by a few weeks.
  - Mitigation: `overlay-ports` in vcpkg for custom CGAL port if bleeding edge required.

## References

- [vcpkg manifest mode docs](https://learn.microsoft.com/vcpkg/users/manifests)
- [CMakePresets.json schema](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)
