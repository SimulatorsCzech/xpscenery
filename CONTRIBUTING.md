# Contributing to xpscenery

> **Status:** skeleton. Contributions welcome but expect breaking changes daily.

## Development setup (Windows)

1. Install **Visual Studio 2022 17.12** (or **2026 Preview**) with the "Desktop C++" workload.
2. Install **Git for Windows** 2.45+.
3. Clone the repo:
   ```powershell
   git clone https://github.com/SimulatorsCzech/xpscenery.git
   cd xpscenery
   ```
4. Install **vcpkg** anywhere on your machine and set `VCPKG_ROOT`:
   ```powershell
   git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
   C:\vcpkg\bootstrap-vcpkg.bat -disableMetrics
   [Environment]::SetEnvironmentVariable("VCPKG_ROOT","C:\vcpkg","User")
   ```
5. Configure + build:
   ```powershell
   cmake --preset=windows-msvc
   cmake --build --preset=windows-msvc-release
   ```

First configure takes 30–60 min as vcpkg builds CGAL, GDAL, Boost from source. Subsequent builds are fast (binary cache).

## Code style

- **C++20** minimum, no exceptions on hot paths, STL acceptable.
- Formatting enforced by `.clang-format` (will be added in week 2).
- Warnings are errors (`/WX`).
- One header per translation unit; prefer forward declarations.
- Public core API is C only (`extern "C"`) — keep it minimal.

## Branching

- `main` — always green, Windows build passes, tests pass.
- `feature/<short-name>` — single feature branches, rebase before merge.
- No force-push on `main` ever. PRs squash-merged.

## Commit messages

Conventional-ish:

```
area: short imperative subject (<= 72 chars)

Optional body explaining why, not what. Wrap at 80.
```

Areas: `core`, `ui`, `cli`, `tests`, `docs`, `ci`, `deps`.

Example: `core/dsf: add DSF atom byte-order validation`

## Pull requests

- Small PRs preferred (< 400 lines diff).
- Link the ADR or issue being addressed.
- CI must pass before review.

## Adding a dependency

1. Add to `vcpkg.json` with a `version>=` constraint.
2. Update `docs/versions.md`.
3. Bump baseline SHA if needed.
4. Link in `core/CMakeLists.txt` (or `ui/CMakeLists.txt` for UI-only).
5. Document licensing impact in `docs/LICENSING.md`.

## Reporting bugs

Use GitHub Issues with the `bug` label. Include:
- Windows version (`winver`)
- VS / MSVC version
- `cmake --version`
- Full build log if relevant
- Repro steps

Thanks for contributing.
