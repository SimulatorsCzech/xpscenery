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

### Fáze 0b — Ověření toolchainu 🔧 PROBÍHÁ
*20. dubna 2026*

- [x] Audit systému: Git, CMake, VS 2026, Win SDK, gh, winget
- [ ] Instalace vcpkg + nastavení `VCPKG_ROOT`
- [ ] První lokální CMake configure (bez Qt, jen CLI)
- [ ] První úspěšný build `xpscenery-cli.exe`
- [ ] Spuštění `xpscenery-cli --version` a `--help`
- [ ] Spuštění unit testů (`ctest`)
- [ ] Vytvoření GitHub repa `SimulatorsCzech/xpscenery`
- [ ] První `git push`
- [ ] Ověření zelené CI

### Fáze 1 — Port core enginu (6 měsíců)
*květen–říjen 2026*

1. **Utils** (74k LOC) — nejjednodušší, self-contained
2. **DSF** (11k LOC) — binární čtení/zápis X-Plane DSF
3. **Obj** (6k LOC) — X-Plane .obj reader
4. **XESCore** (57k LOC) — mesh, zoning, CGAL triangulace
5. **XESTools** (28k LOC) — DSFTool + orchestrace

Výstup: `xpscenery-cli.exe` produkující bit-identický DSF jako Linux RenderFarm.

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

## 5. Aktuální stav — dnes (20. dubna 2026)

### Hotovo

- ✅ Kompletní skeleton 30+ souborů, commit `2b2fd86` na `main`
- ✅ Master dokument vize `Tvorba/ARCHITECTURE_V6.md` se sekcí §2.5 (profesionální GUI požadavky)
- ✅ Analýza RenderFarmUI `Tvorba/ANALYZA_RENDERFARMUI_2026-04-19.md`
- ✅ Lokální toolchain ověřen (MSVC 19.50, Win SDK 26100, CMake 4.2.1, Git 2.52, gh 2.86)

### Dnes ještě uděláme

1. Instalace vcpkg do `G:\RenderFarm_muj\vcpkg`
2. Nastavení `VCPKG_ROOT` uživatelské environment proměnné
3. CMake configure lokálně (preset `windows-msvc`)
4. Build `xpscenery-cli.exe`
5. Ověření `--version`
6. GitHub repo + push
7. Ověření CI běhu

### Blokátory / otevřené otázky

*Žádné k dnešnímu dni.*

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
| `2b2fd86` | Nahrazení YOUR_USERNAME → SimulatorsCzech + .vscode config |

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

**Konec dokumentu.** Verze: 1.0 (2026-04-20).
