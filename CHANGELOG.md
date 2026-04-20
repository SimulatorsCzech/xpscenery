# Changelog

Všechny významné změny v xpscenery jsou zapsány zde. Formát vychází z [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), verzování je [SemVer](https://semver.org/).

## [Unreleased]

### Added (Fáze 1B — io_dsf + geodesy + detectors)
- **`io_dsf::read_defn_strings()`** — generický reader DEFN string
  tabulek (TERT/OBJT/POLY/NETW/DEMN). `inspect` vypisuje počty:
  `terrain: N`, `objects: N`, `polygons: N`, `networks: N`, `rasters: N`.
- **`io_dsf` — string-table reader**: přidány `read_child_atoms()`,
  `read_string_table()` a `read_properties()`. `inspect` nyní vypisuje
  HEAD → PROP pole jako `key = value` (včetně JSON varianty).
- **`xpscenery-cli tile --lat --lon`** — resolve bodu do X-Plane 1°×1°
  DSF dlaždice, vypisuje canonical name, SW/NE roh, supertile (10°×10°).
- **`xpscenery-cli distance --haversine`** — volitelné srovnání s
  kulovou vzdáleností (ukazuje delta vs. Vincenty).
- **ADR-0005** — Geodesy: Vincenty inverse, no GeographicLib (v0.x).
- **`modules/io_raster/`** — byte-level detekce TIFF / BigTIFF (II*/MM*,
  verze 42/43), čtení first-IFD offsetu (32b / 64b pro BigTIFF), bez
  libtiff/GDAL. Integrace do `inspect` — vypisuje `tiff : kind=…, first_ifd=…`.
- **`modules/io_osm/`** — detekce OSM PBF (BlobHeader s `OSMHeader`/
  `OSMData` typem) a OSM XML (`<osm` root tag). Integrace do `inspect`.
- **`xpscenery-cli distance`** — nový subcommand pro geodetickou vzdálenost:
  - `--lat1/--lon1/--lat2/--lon2` → vzdálenost [m/km] + oba azimuty + počet iterací
  - `--json` varianta s fixed-precision výstupem
- **`modules/geodesy/`** — WGS84 Vincenty inverzní formule:
  - `vincenty_inverse(a, b)` → `InverseResult { distance_m,
    initial_bearing_deg, final_bearing_deg, iterations }`
  - sub-mm přesnost do ~20 000 km, detekce nekonvergence u antipodálních bodů
  - žádné externí deps (čistá matematika + `<numbers>`)
  - 4 test cases: Praha-Berlín, čtvrt rovníku, shodné body, neplatné vstupy
- **`modules/io_dsf/`** — minimální reader X-Plane DSF formátu:
  - `looks_like_dsf()` — rychlá detekce 8-bajtového magic `"XPLNEDSF"`
  - `read_header()` — validace cookie + master version (1)
  - `read_top_level_atoms()` — průchod atom tree (`HEAD`, `DEFN`, `GEOD`,
    `CMDS`, `DEMS`, …) s korektním mapováním 4-bajtových ID na
    lidsky čitelné tagy
  - všechny funkce vrací `std::expected<T, std::string>` — žádné výjimky
    na expected-error path
- `xpscenery-cli inspect <dsf>` nyní automaticky rozpozná DSF a vypíše
  `version`, počet atomů a seznam `[TAG] offset=… size=…`; režim `--json`
  emituje strukturovaný výstup (`{"dsf":{"version":1,"atoms":[…]}}`)
- Syntetický DSF generátor + 5 nových test cases pro `io_dsf`
  (magic detection, version validation, truncation, atom walk, bogus size)

### Changed
- PLAN.md bumped na **v1.3**
- Top-level `CMakeLists.txt` — přidán `XPS_BUILD_MOD_IO_DSF` option
- `modules/app_cli` linkuje `xps::io_dsf`

### Added (Fáze 1A — modernization rewrite)
- Řídící dokument `PLAN.md` (česky) — jediný zdroj pravdy pro plánování
- Changelog (tento soubor)
- PLAN.md v1.1: F\u00e1ze 0b označena hotová, rozpis F\u00e1ze 1 po modulech, sekce 12 „Naučené lekce"
- CI workflow: krok „Override VCPKG_ROOT" po msvc-dev-cmd — přepíše VS-bundled
  vcpkg cestu (která neexistuje na runneru) na workspace-lokální přes `$GITHUB_ENV`
- **Fáze 1 — modernizační přepis** (ADR-0004): 5 nových modulů místo 1:1 portu
  - `modules/core_types/` — silně typované `LatLon`, `TileCoord`, `BoundingBox`,
    `VersionInfo` (žádné volné `double lat, lon` dvojice, parsing přes
    `std::expected`, haversine vzdálenost, kanonická DSF jména)
  - `modules/io_logging/` — tenká spdlog fasáda (async, rotující soubor + konzole,
    pojmenované loggery per-modul)
  - `modules/io_filesystem/` — atomické zápisy (temp + rename), UTF-8 BOM strip,
    `dsf_path_for_tile` skládá X-Plane layout, vše vrací `std::expected`
  - `modules/io_config/` — typovaný `TileConfig` se `schema_version`, volitelný
    `aoi`, vektor `LayerSource` (id/kind/path/srs/priority/enabled), parametry
    `meshing` a `export`; unknown keys preserved on round-trip
  - `modules/app_cli/` — CLI11 subcommand framework (`version`, `inspect`,
    `validate`) s `-L/--log-level`, `--json` výstupy a sdílenou dispatch hlavičkou
- Unit testy pro všech 5 nových modulů (30 test cases, **372 assertions**)
- ADR-0004 *Modernization Manifesto* — zdůvodnění C++23, expected-over-exceptions,
  silně typovaných doménových typů, CLI11, schema-versioned configů

### Changed
- **C++ standard posunut na C++23** (z C++20): `std::expected`, `std::print`,
  `std::format`, `<ranges>`, nové MSVC přepínače `/Zc:lambda /Zc:inline`
  `/Zc:throwingNew /bigobj /diagnostics:caret`
- **vcpkg.json**: přidán `cli11` (odstraněna ruční argumentová analýza)
- **cli/src/main.cpp**: zeštíhlený na 15 řádků — veškerou dispatch dělá
  `xps::app_cli::run(argc, argv)`; entry point jen obstarává `xps_init`/`xps_shutdown`
- **Top-level CMakeLists.txt**: 5 granulárních `XPS_BUILD_MOD_*` options místo
  jediného `XPS_BUILD_MODULE_UTILS`, přísnější MSVC flagy
- **tests/CMakeLists.txt**: přelinkováno na nové moduly, odstraněn mrtvý
  `xps::utils` target

### Removed
- `modules/utils/` placeholder (nahrazen 5 skutečnými moduly)
- Ručně psaný argparser v `cli/src/main.cpp` (nahrazen CLI11)

### Fixed
- **core/include/xpscenery/xpscenery.h**: `XPS_API` makro nyní respektuje
  `XPS_STATIC` a nedává `__declspec(dllimport)` když je core static lib.
  Řeší LNK2019 `__imp_xps_version` atd. na Windows MSVC
- **core/CMakeLists.txt**: `XPS_STATIC=1` jako PUBLIC compile definition,
  aby se propagoval do všech konzumentů (cli, tests)
- **vcpkg.json**: odstraněn nestandardní klíč `_comment_deps` (vcpkg ho odmítal)

## [0.1.0-alpha.0] — 2026-04-20

### Added
- Initial project skeleton (30 souborů, 1762 řádků)
- CMake 4.2 + vcpkg manifest (Boost, CGAL, GDAL, GMP, MPFR, fmt, spdlog, Catch2)
- CMake presety: `windows-msvc`, `windows-msvc-preview`, `windows-clang-cl`, `windows-msvc-asan`, `windows-msvc-ui`
- `core/` — libxpscenery stub s C API (`xps_version`, `xps_init`, `xps_shutdown`, `xps_last_error`)
- `cli/` — xpscenery-cli skeleton (`--version`, `--help`)
- `ui/` — Qt 6 + QML desktop shell (SplitView, MenuBar, dark téma, MapPlaceholder, StatusBar)
- `tests/` — Catch2 unit testy (verze + smoke)
- GitHub Actions workflow pro Windows MSVC build (s vcpkg binary cache)
- Dokumentace: `README.md`, `CONTRIBUTING.md`, `docs/ARCHITECTURE.md`, `docs/versions.md`, `docs/LICENSING.md`
- ADR-0001 (CMake + vcpkg), ADR-0002 (Qt 6 + QML)
- MIT License (vlastní kód) + LGPL/GPL poznámky pro závislosti
- `.vscode/` workspace konfigurace (extensions, settings, launch)
- `.editorconfig`, `.gitignore`
