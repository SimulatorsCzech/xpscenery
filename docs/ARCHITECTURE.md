# xpscenery Architecture

**Status:** skeleton (Phase 0 — pre-port)
**Canonical vision:** see [`Tvorba/ARCHITECTURE_V6.md`](../../Tvorba/ARCHITECTURE_V6.md) in the sibling workspace folder.

## Three-layer design

```
┌──────────────────────────────────────────────┐
│  ui/         Qt 6 + QML desktop application  │  ← interactive
│              (Qt Quick, Qt Quick 3D, QML)    │
├──────────────────────────────────────────────┤
│  cli/        xpscenery-cli.exe               │  ← batch / CI
├──────────────────────────────────────────────┤
│  core/       libxpscenery (C++20)            │  ← ported engine
│              ├── utils/   (from XESUtils)    │
│              ├── dsf/     (from DSFLib)      │
│              ├── xescore/ (from XESCore)     │
│              ├── xestools/(from XESTools)    │
│              └── capi/    C ABI for FFI      │
└──────────────────────────────────────────────┘
```

### Contract rules

1. **Core knows nothing about UI.** No Qt in core; only STL + vendored C-friendly types on the public surface.
2. **UI talks to core only via C API** (`core/include/xpscenery/xpscenery.h`). This keeps core swappable and ABI-stable.
3. **CLI is thin.** It parses arguments and calls the same core APIs the UI uses.
4. **Tests target core directly** — fast, deterministic, headless.

## Directory conventions

- `core/include/` — public headers (visible to CLI + UI + tests)
- `core/src/`     — private implementation (not installed)
- `core/src/capi/` — extern "C" shim layer
- `ui/src/`       — Qt backend (controllers, models, bridge to core)
- `ui/qml/`       — QML views

## Module port order (Phase 1)

Ported from `xptools260/src/`:

1. **Utils** (74k LOC) — math, geometry primitives, parsers. Easiest, self-contained.
2. **DSF** (11k LOC) — binary reader/writer. Small, critical, well-tested.
3. **Obj** (6k LOC) — X-Plane .obj reader.
4. **XESCore** (57k LOC) — mesh, zoning, triangulation. Hardest, uses CGAL heavily.
5. **XESTools** (28k LOC) — DSFTool, command dispatchers.
6. **RenderFarmUI-equivalent** — skipped (replaced by new Qt UI).

Each module gets its own `core/<module>/` subtree with `CMakeLists.txt`, `include/`, `src/`, `tests/`.

## Build matrix

| Preset | Compiler | C++ std | Purpose |
|---|---|---|---|
| `windows-msvc` | MSVC 19.40 (VS 2022 17.12) | C++20 | stable baseline |
| `windows-msvc-preview` | MSVC 19.50+ (VS 2026) | C++23 | bleeding edge |
| `windows-clang-cl` | Clang-cl 19 | C++20 | cross-check for portability |
| `windows-msvc-asan` | MSVC + ASan | C++20 | memory debugging |

See [`CMakePresets.json`](../CMakePresets.json).

## See also

- [ADR-0001 — Use CMake + vcpkg](ADR/0001-use-cmake-and-vcpkg.md)
- [ADR-0002 — Use Qt 6 + QML for UI](ADR/0002-use-qt6-for-ui.md)
- [versions.md](versions.md) — pinned dependency versions
