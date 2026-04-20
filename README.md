# xpscenery

> Modern Windows GIS + scenery authoring tool for X-Plane 12.
> Combines QGIS-like GIS editing, WorldEditor-like authoring, and RenderFarm-like procedural generation in a single native Windows 11 application.

**Status:** 🧪 Early development — skeleton only (April 2026)

## Goals

- **Native Windows 11** — no WSL, no virtualization, no cross-compile
- **Professional UX** — 60+ fps, dockable workspace, dark/light theme, DPI-aware (Qt 6.10 + QML)
- **Accurate + fast** — C++20 core ported from the existing xptools260 RenderFarm engine (~170k LOC)
- **CLI + GUI parity** — same engine runs batch (100+ tiles headless) and interactive
- **Bit-identical DSF output** — regression-tested against Linux RenderFarm baseline

## Tech stack

| Layer | Technology |
|---|---|
| **Core engine** | C++20, CMake 4.2, vcpkg (manifest mode) |
| **GIS / geometry** | CGAL 6.1 RC, GDAL 3.12 RC, Boost 1.89 beta, GMP 6.3, MPFR 4.2 |
| **UI** | Qt 6.10 Preview + QML + Qt Quick 3D |
| **Build** | Visual Studio 2026 Preview (MSVC 19.50), Ninja |
| **CI** | GitHub Actions (Windows runners) |
| **Docs** | MkDocs Material |

See [docs/versions.md](docs/versions.md) for exact pinned versions.

## Repository layout

```
xpscenery/
├── core/            # libxpscenery — C++ engine (ported from xptools260)
│   ├── utils/       # XES utils, math, geometry primitives
│   ├── dsf/         # DSFLib — X-Plane .dsf reader/writer
│   ├── xescore/     # mesh, triangulation, zoning
│   ├── xestools/    # DSFTool, RenderFarm orchestration
│   └── capi/        # C API surface for FFI/UI integration
├── cli/             # xpscenery-cli.exe
├── ui/              # Qt 6 + QML desktop application
│   ├── src/         # C++ backend (controllers, models)
│   └── qml/         # QML components (panels, map, 3D view)
├── tests/           # regression DSF, perf benchmarks
├── docs/            # architecture, ADRs, user docs
├── scripts/         # build helpers, CI support
├── .github/         # GitHub Actions workflows
├── CMakeLists.txt   # top-level CMake
├── CMakePresets.json
├── vcpkg.json       # dependency manifest
└── README.md
```

## Quick start (developer)

**Prerequisites (Windows 11):**
- Visual Studio 2026 Preview with "Desktop C++" workload
- CMake 4.2+
- Git 2.45+
- Qt 6.10 Preview (installed via vcpkg or Qt Online Installer)

**Build:**

```powershell
git clone https://github.com/SimulatorsCzech/xpscenery.git
cd xpscenery
git submodule update --init --recursive
cmake --preset=windows-msvc-preview
cmake --build --preset=windows-msvc-preview-release
```

Artefacts land in `build/windows-msvc-preview/bin/`.

## License

- **Own code:** MIT License — see [LICENSE](LICENSE)
- **Qt 6:** dynamically linked under LGPL-3.0 — users can substitute their own Qt build
- **CGAL:** GPL-3.0 — the distributed binary is therefore GPL-3.0 combined work
- **GDAL, Boost, GMP, MPFR:** permissive, no restrictions

See [docs/LICENSING.md](docs/LICENSING.md) for details.

## Roadmap

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) and the upstream vision document in `Tvorba/ARCHITECTURE_V6.md`.

- [ ] **Phase 0** — tactical fixes in xptools260 (ongoing, separate repo)
- [ ] **Phase 1** (6 months) — port core to MSVC, CLI MVP, regression suite
- [ ] **Phase 2** (3 months) — Qt shell + MapLibre embed, dockable workspace
- [ ] **Phase 3** (3 months) — interactive editing (digitize, airport, zoning)
- [ ] **Phase 4** (3 months) — 3D preview (Qt Quick 3D), live sim export
- [ ] **Phase 5** (3 months) — polish, plugins, v1.0 release

## Related projects

- [xptools260](https://gitlab.com/x-plane/xptools) — upstream RenderFarm C++ engine (Linux)
- [Laminar WED](https://developer.x-plane.com/tools/worldeditor/) — airport editor
- [QGIS](https://qgis.org/) — reference professional GIS UX
- [Ortho4XP](https://github.com/oscarpilote/Ortho4XP) — ortho/mesh generator

## Contributing

Project is in skeleton phase. Issues and discussions welcome; code contributions will open after Phase 1 core port reaches CLI MVP.

---

© 2026 — MIT License
