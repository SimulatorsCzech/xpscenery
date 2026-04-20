# ADR-0008 — UI architecture: Qt 6.9 / QML 3D / MVVM

**Status:** Accepted · 2026-04-22
**Context after:** Fáze 1H complete (12 CLI subcommands, 85/85 tests green)
**Supersedes:** none
**Superseded by:** none

## Context

Po dokončení Fáze 1H má xpscenery plnohodnotnou read-only backend surface:
`io_dsf`, `io_raster`, `io_obj`, `io_config`, `core_types`, `geodesy`. CLI
`xpscenery-cli` má 12 subcommandů. Uživatelský požadavek: **profesionální
interaktivní GUI desktop aplikace**, ne CLI-over-GUI ani JSON editor.

Rozhodovali jsme mezi třemi cestami:

- **A** — dokončit backend (v0.5.0 CGAL mesh + v0.6.0 full DSF writer) a
  teprve pak stavět UI. Bezpečné, ale uživatel 3+ měsíce nic neuvidí.
- **B** — paralelní UI team + backend team proti stubům. Vyžaduje víc lidí.
- **C** — MVP UI nad existujícím read-only backendem hned. Uživatel dostane
  použitelnou aplikaci (Scenery Inspector) okamžitě. UI se postupně
  rozšiřuje, jak přibývá backend (mesh, build, export).

## Decision

**Varianta C.** Start Fáze 2A hned. Rozdělíme na podfáze 2A/2B/2C, kde
každá přidá viditelné featury bez blokování dalších.

### 2A — MVP UI Shell (tento týden)

- **Framework**: Qt 6.9 (vcpkg `qtbase`, `qtdeclarative`, `qtquick3d`,
  `qtlocation`, `qttools`, `qtshadertools`).
- **Docking**: [Qt Advanced Docking System](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System)
  přes vcpkg (nebo FetchContent, pokud vcpkg port zatím chybí).
- **Jazyk**: C++23 stejně jako backend. QML jen tam, kde dává smysl (3D
  scéna, animace, custom controls). Hlavní okno je Qt Widgets (lepší
  docking support, nativnější look).
- **Architektura**: MVVM — každý Tab má ViewModel (Qt `QObject`) nad
  `io_*` moduly. View (Widget/QML) pouze renderuje, nemodifikuje model.
- **Modul**: `modules/app_ui/` (STATIC lib) + `ui/xpscenery.exe` cíl.
- **CMake option**: `XPS_BUILD_UI=ON` (už existuje v `CMakePresets.json`,
  preset `windows-msvc-ui`).

### Taby MVP

1. **DSF Inspector** — `QTreeView` s atom tree (HEAD/DEFN/GEOD/CMDS/DEMS),
   `QTableView` pro properties, panel s raster metadaty, CMDS stats.
2. **Raster Viewer** — metadata z `io_raster::geotiff_ifd` (classic TIFF
   vs BigTIFF, IFD entries, GeoKeys). V 2C přidáme GDAL preview do
   `QImage`.
3. **OBJ8 Viewer** — preamble, textures, point counts, draw commands.
4. **Project** — GUI wrapper nad `io_config::TileConfig`, persistuje jako
   `.xpsproj` (JSON). Uživatel **needituje JSON ručně**, jen formulář.

### Infrastruktura

- **Logging**: `spdlog` (už existuje) + vlastní sink, který pushuje do
  `QPlainTextEdit` v dockovatelném panelu.
- **Settings**: `QSettings` (HKCU\Software\SimulatorsCzech\xpscenery).
- **Theme**: QSS dark theme (Fluent-like), přepínatelné. High-DPI přes
  `Qt::AA_EnableHighDpiScaling`.
- **i18n**: připraveno pro `QTranslator`, ale MVP je anglicky.

### Co NENÍ v MVP

- Mapa / AOI — Fáze 2B (vyžaduje Qt Location integraci + tile server)
- Raster image preview — Fáze 2C (vyžaduje GDAL binding přes vcpkg `full`)
- 3D mesh preview — Fáze 3 (vyžaduje v0.5.0 mesh_core)
- Export / build tlačítko — Fáze 4 (vyžaduje v0.6.0 full pipeline)
- Undo/redo, command palette, Python scripting — Fáze 5

## Consequences

### Positive

- Uživatel má funkční desktop aplikaci **tento týden** (ne za 3 měsíce).
- MVP testuje Qt 6.9 integraci, docking, theming, QSettings — všechno to,
  co Fáze 2B/2C přebírá.
- Každý backend milník (v0.5.0, v0.6.0) přidá nový tab nebo rozšíří
  existující — lineární, bezpečný incrementální vývoj.
- `.xpsproj` formát je čistý start — žádná dluhová zátěž z `Tvorba/*.json`
  RenderFarm command-listů (ty zůstávají pouze historická záloha).

### Negative

- `vcpkg install --feature=ui` stáhne ~10 GB a buildne 1–3 hodiny poprvé.
  Mitigace: vcpkg binary cache (už zapnuta v `VCPKG_FEATURE_FLAGS`).
- Qt 6.9 je velká závislost. Mitigace: static linking (`qtbase` staticky),
  ale jen pokud licence dovolí; jinak dynamic + `windeployqt`.
- Qt Advanced Docking zatím nemá oficiální vcpkg port. Fallback:
  CMake `FetchContent` z upstream repa.

### Neutral

- CLI `xpscenery-cli` zůstává plnohodnotný první-class občan. Každá
  GUI akce má CLI ekvivalent (CLI parita princip z PLAN §2).

## Alternatives considered

- **FLTK** (jako starý xptools260) — zamítnuto, nepodporuje moderní
  docking, DPI awareness, dark themes, 3D integraci.
- **ImGui** — zvažováno pro rychlost, ale nevhodné pro persistent docking
  layouts + QSS theming + property-grid editing.
- **Dear ImGui + Magnum** pro 3D — ekosystém slabší než Qt Quick 3D,
  méně nástrojů pro scripting a i18n.
- **Web UI (Electron / Tauri)** — porušuje princip "native Windows",
  výkon + instalace horší.

## References

- PLAN.md v3.0 §1.1, §1.2, Fáze 2A/2B/2C
- Qt 6.9 docs: https://doc.qt.io/qt-6/
- Qt Advanced Docking: https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System
