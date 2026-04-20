# xpscenery — řídící plán projektu

**Autor:** SimulatorsCzech
**Datum založení:** 20. dubna 2026
**Status dokumentu:** živý (aktualizovaný po každém milníku)
**Jazyk:** čeština

> Tento soubor je **jediný zdroj pravdy** pro postup projektu xpscenery. Aktualizuj ho po každé dokončené akci. Anglická technická dokumentace je v `docs/`, ale řídící rozhodnutí jsou zde.

---

## 1. Vize projektu

**xpscenery** je moderní, nativní Windows aplikace pro tvorbu X-Plane 12 scenérie, která kombinuje:

- **GIS editor** — jako QGIS (shapefile, raster, vektor, topologie)
- **Scenery authoring** — jako WorldEditor (letiště, roads, polygony)
- **Procedurální generátor** — jako RenderFarm (zoning, mesh, DSF export)

Vše v **jediné profesionální aplikaci** s 60+ FPS GUI, docking panely, dark/light tématem a DPI-aware renderingem. Cílová platforma **Windows 11 native**, žádné WSL ani virtualizace.

Kanonický dokument vize: [`../Tvorba/ARCHITECTURE_V6.md`](../Tvorba/ARCHITECTURE_V6.md).

---

## 2. Filozofie práce

| Princip | Pravidlo |
|---|---|
| **Bleeding edge** | Používáme nejnovější preview verze nástrojů a knihoven; měsíční revize |
| **Inkrementální dodávky** | Každá fáze skončí funkční verzí, kterou lze použít i bez dalších fází |
| **CLI parita** | Každá GUI akce má ekvivalent v CLI; GUI je jen tenký obal nad core enginem |
| **Bit-identita DSF** | Výstup musí být identický s Linux RenderFarm baseline (regression testy) |
| **Vše zdarma** | Žádné komerční licence, GitHub Public, open source |
| **Čeština v řídicích dokumentech** | `PLAN.md`, zápisky, rozhodnutí; anglický kód a technická dokumentace |

---

## 3. Technologický stack (zamčený 2026-04-20)

| Vrstva | Technologie | Verze | Pozn. |
|---|---|---|---|
| OS | Windows 11 | 23H2+ | 64-bit |
| Kompilátor | MSVC | **19.50** (VS 2026 Community) | ✅ nainstalováno |
| Windows SDK | Win11 SDK | **10.0.26100** | ✅ nainstalováno |
| Build systém | CMake | **4.2.1** | ✅ nainstalováno |
| Dep. manager | vcpkg | master (rolling) | 🔧 instalace v průběhu |
| Verzování | Git | **2.52** | ✅ |
| GitHub CLI | gh | **2.86** | ✅ |
| Jazyk core | C++ | 20 (s cestou na 23) | |
| UI framework | Qt | 6.9+ (6.10 Preview jakmile dostupné) | ⏳ instalace později |
| CI | GitHub Actions | Windows 2022 runner | konfig hotov |

**Třetí strany přes vcpkg** (později podle potřeby): Boost 1.88+, CGAL 6.0+, GDAL 3.11+, GMP 6.3, MPFR 4.2, fmt 11, spdlog 1.15, Catch2 3.8, nlohmann/json 3.12.

---

## 4. Roadmapa — fáze projektu

### Fáze 0 — Skeleton a infrastruktura ✅ HOTOVO
*20.–21. dubna 2026*

- [x] Založit lokální repo `xpscenery/`
- [x] Git init, `main` větev
- [x] README, LICENSE, CONTRIBUTING, .gitignore, .editorconfig
- [x] `CMakeLists.txt` top-level, `CMakePresets.json` (5 presetů)
- [x] `vcpkg.json` s dependency manifestem
- [x] `core/` stub s C API (`xps_version`, `xps_init`)
- [x] `cli/` skeleton (`xpscenery-cli --version`)
- [x] `ui/` Qt 6 stub (SplitView, dark theme, MapPlaceholder)
- [x] `tests/` Catch2 smoke testy
- [x] GitHub Actions workflow (Windows MSVC matrix)
- [x] Docs: ARCHITECTURE.md, ADR-0001, ADR-0002, versions.md, LICENSING.md
- [x] `.vscode/` workspace konfigurace (extensions, settings, launch)
- [x] Nahrazení placeholderů `YOUR_USERNAME` → `SimulatorsCzech`

### Fáze 0b — Ověření toolchainu ✅ LOKÁLNĚ HOTOVO (CI doladění)
*20. dubna 2026*

- [x] Audit systému: Git, CMake, VS 2026, Win SDK, gh, winget
- [x] Instalace vcpkg do `G:\RenderFarm_muj\vcpkg` + `VCPKG_ROOT` User env var
- [x] První lokální CMake configure (preset `windows-msvc`, 105 s)
- [x] První úspěšný build `xpscenery-cli.exe` (Release, 16/16 ninja steps)
- [x] `xpscenery-cli --version` → `xpscenery 0.1.0 (C++202002L)` ✅
- [x] `xpscenery-cli --help` vypíše plánované commands ✅
- [x] Unit testy `xps_tests.exe`: **4/4 test cases, 9/9 assertions** ✅
- [x] Oprava linker chyby: `XPS_STATIC` guard v `core/include/xpscenery/xpscenery.h`
     (static lib + `__declspec(dllimport)` způsobovalo LNK2019 na `__imp_xps_*`)
- [x] vcpkg manifest zjednodušen na Fázi 0b (fmt, spdlog, nlohmann-json, catch2);
     heavy deps přesunuty do feature `full`, Qt 6 do feature `ui`
- [x] Vytvoření GitHub repa **`SimulatorsCzech/xpscenery`** (public)
- [x] První `git push origin main` (commity: `8179015`, `2b2fd86`, `47ea3fa`)
- [x] **Zelená CI** — od commitu `765cded` (override VCPKG_ROOT krok
      přes `$GITHUB_ENV` po `ilammy/msvc-dev-cmd`), artefakty opraveny
      v `32b4ce3`. Debug i Release běží zeleně na `windows-2022` runneru.

### Fáze 1 — Port core enginu (6 měsíců)
*květen–říjen 2026*

**Status: � Fáze 1A+1B+1C+1D HOTOVO (2026-04-20 až 2026-04-22)**

Namísto původně plánovaného monolitického `utils` portu jsme zvolili
vrstvu malých, samostatně testovatelných modulů (každý ≤ ~200 LOC hlaviček +
src). Každý modul má vlastní `CMakeLists.txt`, vlastní unit testy a vlastní
public include pod `xpscenery/<module>/`.

**Dokončené moduly (všechny STATIC lib, C++23, ≥ 70 testů):**

| Modul | Role | LOC | Testy |
|---|---|---|---|
| `core_types` | `LatLon`, `TileCoord`, `BoundingBox` | 172 | ✔ |
| `io_logging` | spdlog wrapper | 100 | ✔ |
| `io_filesystem` | `read_binary`, `write_binary_atomic`, paths | 173 | ✔ |
| `io_config` | JSON tile config (nlohmann-json) | 148 | ✔ |
| `io_dsf` | header + atoms + strings + MD5 + raster + **POOL/SCAL** + **CMDS stats** | ~1 300 | ✔ |
| `io_raster` | TIFF/BigTIFF magic-byte detection | 116 | ✔ |
| `io_osm` | PBF + XML OSM detection | 119 | ✔ |
| `io_obj` | OBJ8 preamble reader | 159 | ✔ |
| `geodesy` | Vincenty inverse (WGS84) | 105 | ✔ |
| `app_cli` | 7 subcommandů (viz níže) | — | integrační |

**CLI subcommandy (release v0.1.0-dev):**

```
xpscenery-cli version
xpscenery-cli inspect   <path.dsf>     # header + atoms
xpscenery-cli validate  <path.dsf>     # cookie + version + MD5
xpscenery-cli distance  <lat1> <lon1> <lat2> <lon2>
xpscenery-cli tile      <lat> <lon>
xpscenery-cli bbox      <west> <south> <east> <north>
xpscenery-cli dsf-stats <path.dsf> [--json]   # plný report DSF
```

**Stále chybí do v0.3.0 „bit-identický DSF writer":**

- [ ] Writer side (DEFN/GEOD/CMDS emise)
- [ ] Geometry decoding (rozvoj CMDS stats na plný geometrický výstup)
- [ ] GeoTIFF IFD parser pro raster tagy (aktuálně jen magic-byte detekce)
- [ ] `XESCore` mesh + zoning port

**Port xptools260 → xpscenery — výstupy releasu:**

1. **v0.1.x** (nyní): `xpscenery-cli inspect | validate | dsf-stats` ✅
2. **v0.2.0**: + `inspect-config <file.json>` (tile config)
3. **v0.3.0**: + `dsf-write` round-trip (read → re-emit bit-identical)
4. **v0.4.0**: + `obj-stats <file.obj>` (hluboká analýza vrcholů/anim)
5. **v0.5.0**: + `build --subset=mesh <tile.json>` (XESCore port)
6. **v0.6.0**: + `build <tile.json>` (plný RenderFarm na Windows)

**Přístup**: greenfield implementace v moderním C++23 (`std::expected`,
`std::span`, `std::byteswap`, `std::println`), referenční sémantika
převzata z `xptools260/src/DSF/DSFLib.cpp` + `XChunkyFileUtils.cpp`.
Regression testy porovnávají proti Linux RenderFarm baseline DSF.

**Výstup Fáze 1**: `xpscenery-cli.exe` produkující (bit-)identický DSF
jako Linux RenderFarm, pokrytý unit + snapshot testy.

### Fáze 2 — Qt shell a základní UI (3 měsíce)
*listopad 2026 – leden 2027*

- Docking panely (Qt Advanced Docking)
- Project manager, layer panel, properties panel
- Command palette (Ctrl+Shift+P)
- Dark/light theme switch
- Základní mapa přes QtLocation (dočasná)

### Fáze 3 — Interaktivní editace (3 měsíce)
*únor–duben 2027*

- Digitalizace vektorů (jako QGIS Advanced Digitizing)
- Editor letišť (runway, taxi, ramp, frekvence)
- Editor zoning pravidel

### Fáze 4 — 3D preview a live export (3 měsíce)
*květen–červenec 2027*

- Qt Quick 3D scéna X-Plane
- MapLibre Native integrace (lepší mapa)
- Live export DSF do běžícího X-Plane 12

### Fáze 5 — Polish a v1.0 release (3 měsíce)
*srpen–říjen 2027*

- Pluginy (3rd party import/export)
- Python scripting API
- Instalační balíček (Inno Setup nebo WiX)
- Dokumentace a tutoriály
- **v1.0 release**

---

## 5. Aktuální stav — 22. dubna 2026

### Hotovo

- ✅ Kompletní skeleton + všechny podpůrné dokumenty (ADR-0001…0005)
- ✅ Fáze 0 + 0b: toolchain, vcpkg, CI zelená
- ✅ Fáze 1A: `core_types`, `io_logging`, `io_filesystem`, `io_config`
- ✅ Fáze 1B: `geodesy` (Vincenty), `io_raster`, `io_osm`, `dsf_strings`,
     CLI `distance` / `tile` / `bbox`
- ✅ Fáze 1C: `dsf_md5` verifier, `dsf_raster` (DEMI/DEMS header),
     CLI `dsf-stats`, `io_obj` preamble reader
- ✅ **Fáze 1D (nová): `dsf_pool` (POOL/SCAL/PO32/SC32 planar numeric
     decoder) + `dsf_cmds` (35-opcode CMDS stream walker, stats)**
- ✅ 73/73 unit testů zelené v Release, sub-2-sekundový běh
- ✅ `xpscenery-cli dsf-stats <file.dsf>` reportuje pools + cmds

### Další krok

1. GeoTIFF IFD parser (aktuálně jen magic-byte detekce)
2. DSF writer (round-trip: read → emit bit-identical)
3. Qt 6 online installer + `-DXPS_ENABLE_UI=ON` preset

### Blokátory / otevřené otázky

- žádné blokátory, CI v pořádku

---

## 6. Repozitáře a cesty

| Cesta | Účel |
|---|---|
| `G:\RenderFarm_muj\xpscenery\` | **Nový projekt xpscenery** (tento) |
| `G:\RenderFarm_muj\xptools260\` | Stávající RenderFarm (Linux) — paralelní Fáze 0 fixy |
| `G:\RenderFarm_muj\Tvorba\` | Plánovací dokumenty, JSON configy, analýzy |
| `G:\RenderFarm_muj\default scenery\` | X-Plane assets (tvoje scenérie) |
| `G:\RenderFarm_muj\vcpkg\` | vcpkg instalace (sdílená) — *bude vytvořeno dnes* |
| GitHub | `github.com/SimulatorsCzech/xpscenery` *(vytvoříme dnes)* |

---

## 7. Pracovní workflow

### IDE — jaký na co

| Úkol | Nástroj |
|---|---|
| Daily C++ vývoj, build, git, Copilot chat | **VS Code** |
| Krok-po-kroku debug, profiler, ASan analýza | **Visual Studio 2026** |
| QML visual designer | **Qt Creator** *(součást Qt instalace, později)* |
| Analýza commitů, review PRs | VS Code (GitLens) nebo web |

### Build commandy (po dokončení Fáze 0b)

```powershell
# Konfigurace (jen CLI)
cmake --preset=windows-msvc

# Build Release
cmake --build --preset=windows-msvc-release

# Spuštění
.\build\windows-msvc\cli\Release\xpscenery-cli.exe --version

# Testy (Debug build)
cmake --build --preset=windows-msvc-debug
ctest --preset=windows-msvc-debug
```

### Git workflow

- `main` = vždy zelená, buildí, testy prošly
- `feature/<jmeno>` = single-feature větve, rebase před merge
- PR squash-merge, žádné force-push na `main`

---

## 8. Commity a changelog

### 2026-04-20

| SHA | Popis |
|---|---|
| `8179015` | Initial skeleton (30 souborů) |
| `2b2fd86` | YOUR_USERNAME → SimulatorsCzech + .vscode config |
| `47ea3fa` | **fix(core): XPS_STATIC** guard + vcpkg manifest |
| `765cded` | **ci: override VCPKG_ROOT** po msvc-dev-cmd |
| `32b4ce3` | **feat(phase1): scaffold xps_utils** + ADR-0003 + Node 24 |

### 2026-04-21 (Fáze 1A + 1B)

| SHA | Popis |
|---|---|
| (multiple) | `core_types`, `io_logging`, `io_filesystem`, `io_config` |
| (multiple) | `geodesy` (Vincenty), `io_raster`, `io_osm`, `dsf_strings` |

### 2026-04-22 (Fáze 1C + 1D)

| SHA | Popis |
|---|---|
| `caf39f1` | **feat(io_dsf): MD5 verify + DEMI raster header** |
| `bb82bdf` | **feat(app_cli): dsf-stats subcommand + io_obj module** |
| *(HEAD)*  | **feat(io_dsf): POOL/SCAL + CMDS stats decoders; docs aligned (Fáze 1D)** |

---

## 9. Rizika a mitigace

| Riziko | Pravděpodobnost | Dopad | Mitigace |
|---|---|---|---|
| vcpkg build CGAL selže (preview verze) | Středně | Vysoký | Pin na CGAL 6.0 stable; overlay-ports pro custom port |
| Qt 6.10 Preview má regressions | Nízké | Středně | Fallback na Qt 6.9 stable |
| MSVC 19.50 ICEs na CGAL Lazy_exact_nt | Nízké | Vysoký | Fallback MSVC 19.40 (VS 2022 vedle) |
| Čas na port Fáze 1 přesáhne 6 měsíců | Vysoké | Středně | Inkrementální releasy, modul po modulu |
| Bit-identita DSF nedosažitelná | Středně | Nízký | Cíl je funkční ekvivalence, bit-identita nice-to-have |
| Výpadek motivace / burnout | Středně | Vysoký | Reality check po každé fázi; pokud nepokračuji, Fáze 1 sama o sobě je hodnotná |

---

## 10. Definice hotovo (Definition of Done)

Každá fáze je **hotova**, když platí:

1. ✅ Kód buildí v CI (Windows, Debug + Release)
2. ✅ Všechny unit testy zelené
3. ✅ Žádné nové `TODO/FIXME/XXX` v commitu než bylo dohodnuto
4. ✅ `docs/ARCHITECTURE.md` aktualizováno
5. ✅ `CHANGELOG.md` má záznam s datem a verzí
6. ✅ `PLAN.md` (tento soubor) má status fáze `HOTOVO`
7. ✅ Ruční smoke test: `xpscenery-cli.exe --version` (po Fázi 1 také `build` command)

---

## 11. Další schůzka s sebou samým

**Datum:** 1. května 2026
**Agenda:**
- Retro Fáze 0 a 0b
- Status portu `Utils` modulu (první měřitelný pokrok Fáze 1)
- Aktualizace `PLAN.md`

---

## 12. Naučené lekce (průběžné)

### 2026-04-20 — první lokální build

- **MSVC link error `__imp_*` = zmatený XPS_API makro**: Když je core
  postavený jako STATIC, hlavičky *nesmí* mít `__declspec(dllimport)`
  pro konzumenty. Řešení: definovat `XPS_STATIC` PUBLIC v CMake a
  v hlavičce dávat přednost této větvi před Win32 import/export.
- **vcpkg manifest nesmí mít nestandardní klíče**: pole `_comment_deps`
  bylo odmítnuto. Komentáře dávat mimo manifest (např. do CHANGELOG.md).
- **GitHub Actions + ilammy/msvc-dev-cmd přepisuje `VCPKG_ROOT`**:
  runner má v image pre-set VS-bundled vcpkg cestu, která často
  neexistuje. Job-level `env:` to *neodchytí*. Nutno přepsat skrz
  `$GITHUB_ENV` v samostatném kroku *po* msvc-dev-cmd.
- **VS Dev Shell aktivace**: před každou novou PowerShell session
  pro build spustit `Launch-VsDevShell.ps1 -Arch amd64 -HostArch amd64`,
  jinak `cl.exe`/`ninja.exe` nejsou v PATH.
- **vcpkg v Release first-time build**: ~1.5 min pro fmt/spdlog/
  nlohmann-json/catch2 (už cachované). Full manifest (s CGAL/GDAL/Boost)
  bude očekávaně 30–60 min při prvním buildu — zapnout binary caching
  před tím.

---

**Konec dokumentu.** Verze: 1.6 (2026-04-22, večer) — po dokončení Fáze 1D.

### Poznámka v1.5 — Fáze 1B (DSF properties + bbox)

- `io_dsf::read_properties()` — HEAD → PROP dekodér (null-terminated
  string table → `(key,value)` páry); `inspect` je vypisuje.
- `io_dsf::read_defn_strings()` + `DefnKind` — čtení TERT/OBJT/POLY/
  NETW/DEMN tabulek; `inspect` dává souhrnné počty.
- `io_dsf::read_child_atoms()` / `read_string_table()` — znovupoužitelné
  stavební bloky pro budoucí hlubokou analýzu atomů GEOD/CMDS/DEMS.
- `xpscenery-cli bbox --tile` — pro danou dlaždici spočítá délky hran
  a diagonálu přes Vincenty + přibližnou plochu v km².
- ADR-0005 dokumentuje volbu Vincenty vs. GeographicLib pro v0.x.

Stav buildu v1.5: lokálně zelený, **52 test cases / 453 assertions**.
CLI subcommands: `version`, `inspect`, `validate`, `distance`
(s `--haversine`), `tile`, `bbox`. Všechny mají `--json` variantu.
