# Changelog

Všechny významné změny v xpscenery jsou zapsány zde. Formát vychází z [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), verzování je [SemVer](https://semver.org/).

## [Unreleased]

### Added
- Řídící dokument `PLAN.md` (česky) — jediný zdroj pravdy pro plánování
- Changelog (tento soubor)
- PLAN.md v1.1: F\u00e1ze 0b označena hotová, rozpis F\u00e1ze 1 po modulech, sekce 12 „Naučené lekce"
- CI workflow: krok „Override VCPKG_ROOT" po msvc-dev-cmd — přepíše VS-bundled
  vcpkg cestu (která neexistuje na runneru) na workspace-lokální přes `$GITHUB_ENV`

### Fixed
- **core/include/xpscenery/xpscenery.h**: `XPS_API` makro nyní respektuje
  `XPS_STATIC` a nedává `__declspec(dllimport)` když je core static lib.
  Řeší LNK2019 `__imp_xps_version` atd. na Windows MSVC
- **core/CMakeLists.txt**: `XPS_STATIC=1` jako PUBLIC compile definition,
  aby se propagoval do všech konzumentů (cli, tests)
- **vcpkg.json**: odstraněn nestandardní klíč `_comment_deps` (vcpkg ho odmítal)

### Changed
- **vcpkg.json**: závislosti zeštíhleny na Fázi 0b (fmt, spdlog, nlohmann-json,
  catch2). Heavy deps (boost-*, cgal, gdal, gmp, mpfr, tinyxml2, zlib)
  přesunuty do feature `full`. Qt 6 zůstává ve feature `ui`.

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
