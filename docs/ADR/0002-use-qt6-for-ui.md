# ADR-0002 — Use Qt 6 + QML for the desktop UI

**Date:** 2026-04-20
**Status:** Accepted
**Deciders:** project owner

## Context

The new xpscenery tool needs a professional, 60+ fps, dockable, DPI-aware Windows desktop UI — see §2.5 of the canonical vision document `Tvorba/ARCHITECTURE_V6.md`.

Three candidates were evaluated:

| Stack | Pros | Cons |
|---|---|---|
| **Tauri + React + MapLibre** | Fastest UI dev, modern web components, MapLibre killer feature | 3 languages (TS/Rust/C++), IPC overhead for 1M+ mesh vertices, npm churn |
| **Qt 6 + QML** | Direct C++ integration, zero IPC, mature (30 years), QGIS reference | Smaller ecosystem, LGPL constraints, weaker map layer |
| **.NET 9 + WinUI 3** | Native Windows, C# velocity | Windows-only, no GIS libs, WinUI 3 still incomplete |

## Decision

Use **Qt 6.9+ with QML (Qt Quick / Qt Quick Controls 2 / Qt Quick 3D)** for the desktop UI.

Rationale:
1. **Zero IPC overhead** — core engine holds up to 27 GB of mesh data; serializing that over a process boundary is a non-starter for interactive use.
2. **Mature C++ integration** — `Q_OBJECT` / signals-slots / property bindings map naturally to what RenderFarm already does.
3. **Proven reference (QGIS)** — the existing gold-standard GIS desktop tool is built in Qt, so the architectural patterns are well-understood.
4. **LGPL-3 is acceptable** — we will dynamically link Qt DLLs, preserving the user's right to substitute their own Qt build. No commercial Qt license required.
5. **Qt Quick 3D covers the 3D preview** requirement without pulling in a separate engine.
6. **Deployment is mature** — `windeployqt` ships all required DLLs; Inno Setup / WiX packaging is well-trodden.

## Consequences

### Positive
- Single process, single debugger session across UI + core.
- QML hot-reload gives React-like iteration speed for layout.
- Designers can use Qt Design Studio to author QML visually.
- Built-in accessibility support (`Qt Accessibility`) and i18n (`Qt Linguist`).

### Negative
- **Map view is harder** — there is no Qt-native MapLibre. Options:
  - Use `QtLocation` (adequate but dated).
  - Embed MapLibre Native (C++) inside a `QOpenGLWidget` — preferred, deferred to Phase 3.
  - Embed web-based MapLibre via `QtWebEngine` — heavy but works.
- Smaller frontend developer talent pool vs. React.
- Qt 6.10 Preview must be either built from source or installed via Qt Online Installer (vcpkg Qt port may lag).

## References

- [Qt 6 licensing overview](https://doc.qt.io/qt-6/licensing.html)
- [MapLibre Native C++](https://github.com/maplibre/maplibre-native)
- [QGIS architecture notes](https://docs.qgis.org/latest/en/docs/pyqgis_developer_cookbook/)
