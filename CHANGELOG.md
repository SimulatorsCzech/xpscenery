# Changelog

Všechny významné změny v xpscenery jsou zapsány zde. Formát vychází z [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), verzování je [SemVer](https://semver.org/).

## [Unreleased]

### Added (Fáze 3A — mesh_core foundation — 2026-04-24/25)
- **mesh_core** (nový modul `xps::mesh_core`): základ pro TIN / Delaunay
  triangulaci v čistém C++23, bez CGAL (Fáze 3B přidá CGAL backend).
  Veřejná API:
  - `Point2`, `Point3`, `Bbox2` v `point.hpp` (POD, constexpr).
  - `TriangleMesh` v `triangle_mesh.hpp` — seznam vrcholů `Point3` +
    trojúhelníků `Triangle{v0,v1,v2}` (VertexId = uint32).
  - `orient_2d`, `in_circle_2d` v `predicates.hpp` (3×3 / 4×4 determinant,
    plovoucí predikáty — robustní CGAL verze přijde ve Fázi 3B).
  - `delaunay_triangulate_2d(span<Point2|Point3>, DelaunayOptions)
    → expected<TriangleMesh, string>` — Bowyer-Watson inkrementální
    algoritmus (O(N²) super-triangle varianta, postačuje pro desítky tisíc
    bodů). Volitelná deduplikace bodů s tolerancí 1e-12.
  - `write_obj(path, mesh)` a `write_ply_ascii(path, mesh)` v `mesh_io.hpp`
    (Wavefront OBJ, Stanford PLY ASCII — `std::format` s `{:.17g}`).
- **app_cli**: nový subcommand `xpscenery-cli triangulate SRC --out PATH
  [--format obj|ply] [--json]`. Vstup je textový soubor s body `x y [z]`
  (řádek = bod, `#` komentář). Exit kódy 1–4 pro chyby otevření, syntaxe,
  triangulace a zápisu.
- **tests**: `tests/unit/mesh_core/test_delaunay.cpp` — 8 test cases / 31
  assertions pokrývají predicates, jednotkový čtverec s centrem, trojúhelník,
  deduplikaci, CCW orientaci výstupu, kolineární vstup a < 3 body.

### Notes
- Delaunay používá *non-robust* float predikáty — pro produkční mesh z
  reálných geodat bude Fáze 3B napojena na CGAL 6.1.1
  (`CGAL::Delaunay_triangulation_2` + `CGAL::Exact_predicates_inexact_constructions_kernel`).
- První smoke test: jednotkový čtverec + střed → 4 trojúhelníky se společným
  centrem (ověřeno `xpscenery-cli triangulate`).

### Added (OSM PBF minimální viewer — 2026-04-24)
- **io_osm**: nové API `read_pbf_header(path) → expected<PbfHeaderInfo, string>`
  v `pbf_header.hpp`. Dekóduje top-level rámec `.osm.pbf`:
  1) BE uint32 délka prvního BlobHeader,
  2) BlobHeader protobuf (tag 1 string `type`, tag 3 int32 `datasize`).
  Nerozbaluje samotný Blob (to je zlib-komprimovaný OSMHeader/OSMData) —
  stačí pro ověření formátu a základní metadata. Implementace má ruční
  varint dekodér (žádná nová dependence).
- **PbfViewerView**: nová záložka **„OSM PBF"** v MainWindow. Ukazuje
  `file size`, `first_blob_header_len`, `first_blob_type` (očekáváno
  "OSMHeader") a `first_blob_datasize`. Deep parse (bbox z OSMHeader,
  node/way/relation) explicitně odložen do pozdější fáze.
- **Drag&Drop**: `.pbf` dosud jen přidalo řádek do Layers —
  `open_layer(kind=osm_pbf)` nyní otevírá vrstvu přímo v novém viewru.
- **open_path**: `.pbf` přípona rozpoznaná jako `FileKind::Pbf` —
  funguje File → Open, Recent Files i drag&drop.

### Added (TIFF pixel preview — 2026-04-24)
- **io_raster**: nové API `read_tiff_first_strip(path) → expected<TiffStripPreview, string>`
  v `tiff_preview.hpp`. Dekóduje první strip classic (32-bit) TIFF se
  `compression = 1` (nekomprimovaný), `bits_per_sample = 8` a
  `samples_per_pixel ∈ {1, 3}`. Správně reaguje na
  `photometric = 0 (WhiteIsZero)` invertováním. BigTIFF a kompresní
  schémata (LZW, Deflate, JPEG) vrací čitelnou chybu — UI je přeskočí.
- **RasterViewerView**: pod mini-mapou přibyl panel **„Pixel preview
  (první strip)"** se `QLabel` na který se po úspěšném dekódu nakreslí
  `QPixmap::fromImage(QImage::scaled(…, KeepAspectRatio, SmoothTransformation))`.
  Při neúspěchu label zobrazí důvod (např. „pouze nekomprimovaný TIFF"),
  vše se zároveň loguje.
- Limit: první strip je omezen na 64 MiB pro ochranu proti patologickým
  souborům. Typické EPSG:4326 GeoTIFFy mají rows_per_strip ≪ height,
  takže se preview vyrenderuje rychle i u velkých dlaždic.

### Added (Shapefile minimální viewer — 2026-04-24)
- **Nový modul `xps::io_vector`** (static library): parser 100-bytového
  hlavičky ESRI Shapefile (.shp) dle whitepaper z července 1998. Extrahuje
  `file_code` (validace 9994), `version`, `shape_type` (0/1/3/5/8/11…/31),
  `file_length_bytes` a 2D/3D/M bounding box (X/Y/Z/M). Funkce
  `shape_type_name()` překládá raw int na lidsky čitelný řetězec
  ("polygon", "point (Z)", …). API: `read_shp_header(path) → expected<ShpHeader, string>`.
- **ShpViewerView**: nová záložka **„Shapefile"** v MainWindow — klasické
  schéma (path + Browse + „Zobrazit v mapě", summary + key/value tabulka,
  mini-map overview 160–220 px). Pokud je bbox v EPSG:4326 rozsahu
  (±180°×±90°), emituje `shp_bbox` → v hlavní mapě i mini-mapě se zobrazí
  oranžový overlay (sdílen s rasterem) a automaticky se na něj přiblíží.
  Mimo WGS84 (projektovaný SRS) se mini-mapa přeskočí s varováním.
- **Drag&Drop**: `.shp` bylo již v projektu uznáváno jako `shapefile` —
  nyní dispatcher `open_layer` otevírá vrstvu přímo v novém viewru místo
  warningu. Dvojklik na řádek Shapefile v Projectu funguje také.
- **open_path**: `.shp` přípona rozpoznaná jako `FileKind::Shp` — funguje
  File → Open, Recent Files a drag&drop na hlavní okno.

### Added (Otevření vrstvy dvojklikem — 2026-04-24)
- **ProjectView**: nový signál `open_layer(kind, path)` emitovaný při
  dvojkliku na sloupec 0 (ID) nebo 5 (Enabled) — ostatní sloupce
  zůstávají editable jako dříve.
- **MainWindow** wiring: `kind=geotiff/tiff` → RasterViewer;
  `kind=dsf` → DsfInspector. Tab se přepne + `open_file(path)` proběhne.
- Nepodporované typy (shapefile, osm_pbf) zatím zahlásí warn v logu.

### Added (Mini-map overview — 2026-04-24)
- **RasterViewerView + DsfInspectorView**: vlastní instance `TileGridView`
  (160–220 px vysoká) jako náhled polohy souboru ve světě. Po úspěšném
  načtení se nastaví bbox a automaticky se na něj přiblíží (87 % fill,
  clamp 0.5..50 px/°). Uživatel vidí pokrytí okamžitě, bez přepínání
  na záložku Mapa.
- Čistá `TileGridView` (QPainter) — žádná duplicitní logika.

### Added (Show in map buttons — 2026-04-24)
- **RasterViewerView**: tlačítko "Zobrazit v mapě" vedle Browse —
  přepne na záložku Mapa a zavolá `zoom_to_raster_bbox`.
- **DsfInspectorView**: ditto pro DSF bbox (`zoom_to_dsf_bbox`).
- **MainWindow** wiring přes `map_view_->parentWidget()` (map panel
  obalený v QWidget s toolbar-em).

### Added (Drag & drop layers — 2026-04-24)
- **ProjectView akceptuje drop souborů** (.tif/.tiff, .shp, .pbf, .dsf)
  kamkoli do plochy záložky projektu → automaticky přidá nový řádek
  do tabulky Layers s detekovaným `kind` (geotiff/shapefile/osm_pbf/dsf),
  ID odvozeným z `completeBaseName()` (zkrácené na 32 znaků), SRS
  `EPSG:4326`, priority 0, enabled=checked.
- Nepodporované přípony se tiše ignorují (kurzor dropu ukáže "ignore").
- Log: `drop: přidáno N vrstev`.

### Added (UX — Zoom-to-bbox, 2026-04-24)
- **`TileGridView::zoom_to_bbox(W,S,E,N)`** + wrappery
  `zoom_to_raster_bbox/dsf_bbox/aoi`: spočítají `pixels_per_deg` tak,
  aby bbox vyplnil ~87 % viewportu, clampují do rozsahu 0.5..50, a
  vycentrují střed pohledu na střed bbox. Extrémně malé bboxy
  (< 1e-4°) jsou ignorovány.
- **Toolbar mapy**: tři nové akce "Přiblížit: raster / DSF / AOI"
  umístěné za separátorem vpravo od přepínače "Raster bbox → AOI".

### Added (Fáze 2C — DSF coverage overlay, 2026-04-24)
- **DSF bbox v mapě**: `DsfInspectorView` po načtení souboru čte
  HEAD/PROP (`io_dsf::read_properties`), vytáhne hodnoty `sim/west`,
  `sim/south`, `sim/east`, `sim/north` a emituje signál
  `dsf_bbox(W,S,E,N)`. Neplatné/chybějící hodnoty se tiše ignorují.
- **`TileGridView::set_dsf_bbox/clear_dsf_bbox`**: zelený (RGB 46/204/64,
  alpha 50) dash-dot overlay kreslený **pod** raster bbox i AOI — tři
  vrstvy lze zobrazit současně bez vzájemného překryvu logiky.
- **Toolbar**: přibyla akce "Smazat DSF bbox" vedle "Smazat raster bbox".
- **MainWindow wiring**: `DsfInspectorView::dsf_bbox →
  TileGridView::set_dsf_bbox`.

### Added (UX polish — 2026-04-23 večer)
- **Přejít na dlaždici…** (Ctrl+G) — QInputDialog přijme buď lat/lon
  (`50 15`, `50,15`, `50;15`) nebo kanonický X-Plane název (`+50+015`,
  `-22-043`). Po validaci: `ProjectView::set_tile`, highlight v mapě,
  vycentrování pohledu. Regex `^([+-])(\d{1,2})([+-])(\d{1,3})$`.
- **Status bar live feedback**: aktuální tile (`+50+015`) nebo AOI
  (`1.2°×0.8° [W=14.1 S=49.7 E=15.3 N=50.5]`) je vidět vedle zprávy z logu.
- **Auto-center mapy** při změně tile v `ProjectView` — když uživatel
  upraví lat/lon spinboxy, mapa se na tile posune (a rozzoomuje, pokud
  byl zoom pod 10 px/°).
- **Persistence viewportu mapy** (QSettings `Map/centerLon|Lat|ppd`):
  zoom i střed pohledu se obnoví po restartu aplikace.

### Added (Fáze 2C start — raster bbox → mapa, 2026-04-23)
- **GeoTIFF extent overlay v mapě**: `RasterViewerView` po načtení
  souboru spočítá geografický bounding box z `ModelPixelScale` (33550)
  + `ModelTiepointTag` (33922) + width/height IFD a emituje signál
  `raster_bbox(W,S,E,N)`.
- **`TileGridView::set_raster_bbox/clear_raster_bbox`**: oranžový
  dotted overlay (RGB 255/140/0, alpha 60) pod AOI vrstvou — nepřepisuje
  uživatelský AOI výběr, pouze informativně ukazuje pokrytí rasteru.
- **`TileGridView::use_raster_bbox_as_aoi`** + akce toolbaru
  "Raster bbox → AOI": jedním klikem se AOI nastaví přesně na rozsah
  rasteru (emituje `aoi_changed`, synchronizuje se s `ProjectView`).
- **MainWindow** napojuje `raster_view_::raster_bbox → map_view_::set_raster_bbox`
  a přidává akce **"Smazat raster bbox"** a **"Raster bbox → AOI"** do
  mini-toolbaru mapy.
- Sanity check: pokud souřadnice leží mimo WGS84 rozsah
  (typicky projektované rastery), overlay se přeskočí a do logu jde
  varování "tiepoint/scale mimo WGS84 rozsah — mapa bbox přeskočena".

### Added (Fáze 2B pokračování — 2026-04-23)
- **File menu rozšíření**: `Ctrl+N` Nový projekt, `Ctrl+O` Otevřít
  projekt (už existovalo), `Ctrl+S` Uložit projekt jako…
- **Window title** nyní reflektuje cestu načteného projektu
  (`xpscenery — +50+015.xpsproj`), pro nový projekt zobrazí
  `(nový projekt)`.
- **Mapa — toolbar** s tlačítky Fit world / Smazat AOI / +/-
  nad `TileGridView` (ikony ze standard Qt stylu).
- **Mapa — kanonické názvy dlaždic** (`+50+015` formát) vykresleny
  uvnitř každé 1°×1° buňky při zoom ≥ 20 px/°.

### Added (Fáze 2B start — TileGridView QPainter map, 2026-04-22)
- **Nový 5. tab "Mapa"** — `TileGridView` widget: pan/zoom/klik/AOI
  výběr nad 1°×1° světovou sítí. Čistý QPainter, bez qtlocation
  dependencies (žádný vcpkg rebuild).
- **Interakce**:
  - Kolečko myši → zoom pod kurzorem (min 0.5 px/°, max 50 px/°)
  - Tažení → pan (otevřená ruka kurzor)
  - Shift+tažení → AOI rectangle (polopropustná modrá překryva)
  - Krátký klik → výběr dlaždice (zelený highlight)
  - Tooltip pod kurzorem s aktuální lat/lon
- **Obousměrná synchronizace Mapa ↔ Project**:
  - `TileGridView::tile_clicked` → `ProjectView::set_tile`
  - `TileGridView::aoi_changed` → `ProjectView::set_aoi`
    (výsledek uložen v `TileConfig.aoi`)
  - `ProjectView::tile_changed` → `TileGridView::set_highlighted_tile`
  - `ProjectView::aoi_loaded` → `TileGridView::set_aoi` (po načtení
    `.xpsproj` se AOI automaticky zobrazí v mapě)
- **Klávesové zkratky** v mapě: šipky = pan 1° (Shift = 10°),
  `+`/`-` = zoom, `Home` = fit world, `Esc` = smazat AOI.
- **Reálný satelitní basemap** (OSM/MapLibre Native) je naplánován
  pro "Fáze 2B full" — bude vyžadovat velký Qt rebuild nebo externí
  MapLibre knihovnu. MVP verze teď stačí pro plánování.

### Added (Fáze 2A polish — toolbar/drag-drop/recent/layers CRUD, 2026-04-22)
- **Unified open_path() dispatcher** — jedno místo pro file routing:
  menu, toolbar, drag-drop a recent files volají tentýž kód; `FileKind`
  enum sniffuje koncovku (dsf/tif/tiff/obj/xpsproj/json).
- **Toolbar** s QStyle standardními ikonami pro DSF/TIFF/OBJ/Project,
  custom QSS styling (hover + pressed states, Fluent-lite).
- **Drag-and-drop** — `dragEnterEvent`/`dropEvent` přijímá lokální URL
  z Průzkumníka; více souborů najednou (každý je otevřen ve správném tabu).
- **Recent Files menu** — strop 10 záznamů, perzistence v `QSettings`,
  po startu se odstraní neexistující cesty, tlačítko "Vymazat seznam".
- **Čeština v UI** — menu, dialogy a log hlášky přeloženy.
- **About Qt** položka v Help menu.

### Added (Fáze 2A polish — ProjectView layer CRUD)
- **Editovatelná tabulka vrstev** se toolbarem `+ Přidat / – Odebrat /
  ▲ / ▼ / Vybrat cestu…`. Přesun řádků zachovává item pointers
  (take/remove/insert) ať nic neuteče na GC.
- **`collect()` nyní shromažďuje řádky tabulky** zpět do
  `std::vector<io_config::LayerSource>`, prázdné řádky přeskočí.
- **Auto-detekce "Kind"** — vybraná cesta se souborem `.tif`/`.tiff`
  → `geotiff`, `.shp` → `shapefile`, `.pbf` → `osm_pbf`, `.dsf` → `dsf`.

### Added (Fáze 2A polish — DSF/OBJ inspectors)
- **DsfInspectorView summary bar**: velikost souboru na disku,
  overhead (file − payload), check přítomnosti core atomů
  `HEAD/DEFN/GEOD/CMDS`, zelené ✓ / červené ✗.
- **ObjViewerView** nyní validuje textury (albedo/LIT/normal) proti
  adresáři .obj: ✓ zelená (nalezeno), ✗ červená (chybí), šedé
  "(nenastaveno)" když field je prázdný.

### Added (Fáze 2A — windeployqt post-build deploy)
- `ui/CMakeLists.txt` přidává POST_BUILD `windeployqt` krok, takže
  `build/.../ui/Release/xpscenery.exe` běží samostatně bez PATH tweaků
  (23 Qt DLL + plugin adresáře vedle exe).

### Added (Fáze 2A — MVP Qt 6 Widgets UI, 2026-04-22)
- **`xpscenery-ui` cílí first-class desktop aplikace** — Qt 6.10.2
  Widgets (ne QML) pro MVP. Ship on Windows, x64. `qtbase`-only
  vcpkg feature (~12 GB build), full stack schován do `ui-full`.
- **MainWindow** — QTabWidget se 4 taby, File menu s Open pro každý
  typ, QDockWidget s log panelem, dark Fluent-lite QSS theme
  v-source, QSettings persist geometry/state (`HKCU\Software\xpscenery`).
- **DSF Inspector tab** — dropzone + browse, volá
  `io_dsf::read_dsf_blob`, strom top-level atomů (HEAD/DEFN/GEOD/
  CMDS/DEMS…), per-atom hex preview (prvních 256 B). Summary s
  DSF verzí, počtem atomů a celkovým payload.
- **Raster Viewer tab** — `io_raster::read_geotiff_ifd`; Classic TIFF
  i BigTIFF, width/height, samples, sample format, compression,
  ModelPixelScale, ModelTiepoints tabulka, GeoKeyDirectory tabulka.
- **OBJ Viewer tab** — `io_obj::read_obj_info`; platform, verze,
  textury (albedo/LIT/normal), POINT_COUNTS, TRIS/LINES/LIGHTS
  commands, ANIM_begin markers.
- **Project Editor tab** — formulář nad `io_config::TileConfig`:
  tile lat/lon, schema version, output DSF, meshing knoby
  (max_edge_m, min_triangle_area_m2, smoothing_factor), export
  flags (bit-identical / overlay / compress), read-only layers
  tabulka. Save/Load `.xpsproj` JSON přes `io_config::load/dump`.
- **Build**: `cmake --preset windows-msvc-ui` → `cmake --build
  --preset windows-msvc-ui-release --target xpscenery-ui`. 13 compile
  units, linkuje proti všem 8 backend modulům + Qt6::{Core,Gui,Widgets}.
- **Žádné regresní chyby**: 85/85 unit testů dál zelené.

### Direction shift (2026-04-22) — UI-first, variant C
- Rozhodnuto **po Fázi 1H** spustit Fázi 2 okamžitě jako variantu C:
  MVP Qt 6.9 UI shell nad existujícím backendem (12 CLI subcommandů,
  read-only surface). v0.5.0 + v0.6.0 poručí později bez blokování UI.
- PLAN.md v3.0: nová kapitola 1.1 (co GUI umí v každé fázi) + 1.2
  (konkrétní MVP featury); Fáze 2 přepsána na podfáze 2A/2B/2C.
- **Žádný kus kódu z xptools260 se nekopíruje.** Staré `Tvorba/*.json`
  RenderFarm command-listy zůstávají pouze jako historická záloha —
  xpscenery má vlastní deklarativní projekt formát `.xpsproj`.
- ADR-0008 připravováno: „UI architecture — Qt 6.9 / QML 3D / MVVM“.

### Added (Fáze 1H — inspect-config + obj-stats CLI, v0.2.0/v0.4.0 milníky)
- **`xpscenery-cli inspect-config <src.json>`** — v0.2.0 milník.
  Parsuje tile config JSON přes `io_config::load`, výpis schema
  verze, tile + supertile, AOI (pokud je), layers tabulka, meshing
  knobs, export flags. `--json` přepne na normalizovaný dump.
- **`xpscenery-cli obj-stats <file.obj>`** — v0.4.0 milník.
  Surfacuje `io_obj::read_obj_info`: platform, OBJ verze, textury
  (texture / lit / normal), POINT_COUNTS (vt/vline/vlight/idx),
  draw commands (TRIS/LINES/LIGHTS), počet ANIM_begin. `--json`
  varianta pro scripting.
- CLI teď **12 subcommandů** (přibyly `inspect-config` a
  `obj-stats`).
- **85/85 unit testů** zelené (beze změny od Fáze 1G — nové příkazy
  jsou tenké wrappery nad již otestovanými moduly).

### Added (Fáze 1G — BigTIFF IFD + dsf-inspect CLI, pre-Phase-2 closure)
- **BigTIFF support v `io_raster::read_geotiff_ifd()`** — 64-bit IFD
  walker. Rozpoznává magic `0x002B`, 20-byte entries s uint64 count +
  uint64 value_or_offset, inline payload až do 8 bajtů, LONG8 (type 16)
  / SLONG8 (17) / IFD8 (18) type-width podpora. `GeoTiffInfo`
  dostal pole `bool is_bigtiff`. ModelPixelScale/Tiepoint/GeoKeys
  fungují identicky jako v classic TIFF. 2 nové unit testy.
- **CLI `xpscenery-cli dsf-inspect <src> [--json]`** — expose
  `DsfBlob`ového atom table přes CLI. Textový výstup:
  `tag | bytes | raw_id` + součet. JSON výstup je scriptable.
  Navržené pro budoucí UI binding (Fáze 2).
- `raster-info` implicitně pracuje i pro BigTIFF (byl už napojen na
  `read_geotiff_ifd`, nyní bez chyby "not implemented").
- CLI teď exponuje **10 subcommandů** (přidán `dsf-inspect`).
- Celkem **85/85 unit testů** zelené v Release (+ 2 oproti Fázi 1F).

### Added (Fáze 1F — atom-level decomposing DSF writer, v0.3.0 foundation)
- **`io_dsf::DsfBlob` + `AtomBlob`** — in-memory representation of a
  DSF as `{int32 version, vector<AtomBlob>}` kde každý blob drží
  originální 8-byte header + payload (`std::vector<std::byte> bytes`).
- **`io_dsf::read_dsf_blob(src)`** — dekomposer. Kombinuje
  `read_header` + `read_top_level_atoms` a do blobu připojí každý
  atom jako opaque byte range.
- **`io_dsf::write_dsf_blob(dst, blob)`** — rekomposer. Validuje
  invariant `declared_size == bytes.size()` každého atomu,
  zřetězí je za hlavičku a připojí čerstvý MD5 footer. Zápis
  atomicky přes `write_binary_atomic`.
- **`io_dsf::rewrite_dsf_decomposed(src, dst)`** — `read_dsf_blob` →
  `write_dsf_blob`. **Bit-identický** round-trip na jakémkoliv
  well-formed DSF (unit test to potvrzuje `memcmp`em).
- **CLI `dsf-rewrite --mode {identity|decomposed}`** — default zůstává
  `identity`; `--mode decomposed` volí nový writer. Validátor CLI11
  odmítne neznámé hodnoty.
- **ADR-0007**: decomposing writer jako v0.3.0 foundation; plná
  atom-level dekódování POOL/SCAL/CMDS stále odloženo na v0.5.0.
- Celkem **83/83 unit testů** zelené v Release (+ 4 oproti Fázi 1E).

### Added (Fáze 1E — GeoTIFF IFD + DSF identity writer)
- **`io_raster::read_geotiff_ifd()`** — klasický (32-bit) TIFF IFD
  walker. Parsuje základní tagy (256 `ImageWidth`, 257 `ImageLength`,
  258 `BitsPerSample`, 259 `Compression`, 262 `PhotometricInterpretation`,
  273 `StripOffsets` count, 277 `SamplesPerPixel`, 278 `RowsPerStrip`,
  339 `SampleFormat`) i geokódovací tagy (33550 `ModelPixelScaleTag`,
  33922 `ModelTiepointTag`, 34735 `GeoKeyDirectoryTag` včetně
  key-entries). BigTIFF vrátí `std::unexpected(...)`. 3 nové testy.
- **`xpscenery-cli raster-info <path.tif>`** — pre-flight dump
  TIFF/GeoTIFF hlavičky a první IFD (byte-order, velikost, bpp,
  pixel scale, tiepointy, počet GeoKeys).
- **`io_dsf::rewrite_dsf_identity(src, dst)`** — identity rewriter.
  Zkopíruje vše mezi hlavičkou a MD5 patičkou byte-for-byte, přepočítá
  MD5 a zapíše atomicky přes `write_binary_atomic`. Vrací
  `RewriteReport{source_size, output_size, md5_unchanged, stored, computed}`.
  Použitelné jako opravář zkorumpované patičky. 3 nové testy
  (round-trip identity, repair bad footer, reject non-DSF).
- **`xpscenery-cli dsf-rewrite <src> <dst>`** — CLI wrapper. Po dokončení
  vypíše `md5 ok/changed` + oba digesty.
- **ADR-0006**: DSF reader first, writer is identity rewriter for v0.x
  (odkládá atom-level writer na v0.5.0 spolu s XESCore portem).
- Celkem **79/79 unit testů** zelené v Release (+ 6 oproti Fázi 1D).

### Added (Fáze 1D — POOL/SCAL + CMDS decodery)
- **`io_dsf::read_point_pools()`** — dekodér DSF planar-numeric point-pool
  atomů (POOL+SCAL pro 16-bit, PO32+SC32 pro 32-bit). Podporuje všechny
  čtyři X-Plane kódovací módy (raw, differenced, RLE, RLE+differenced)
  a vrací interleaved `double[plane_count * array_size]` hodnoty přepočtené
  přes per-plane scale/offset (`value = raw/divisor * scale + offset`,
  divisor = 65535 nebo 4 294 967 295). Samostatný planar walker,
  žádná závislost na xptools260.
- **`io_dsf::read_cmd_stats()`** — prochází CMDS atomický stream a počítá
  statistiky pro všech 35 opcodů (PoolSelect, Object/ObjectRange,
  NetworkChain/32/Range, Polygon/Range/Nested(+Range), TerrainPatch(+Flags/LOD),
  Triangle/Strip/Fan + CrossPool + Range varianty, Comment8/16/32).
  Reportuje `opcode_counts[35]`, `pool_selects`, `def_selects`,
  `objects_placed`, `network_chains`, `polygons`, `terrain_patches`,
  `triangles_emitted`, `triangle_vertices`, `bytes_consumed`.
  Chyby (krátký buffer, neznámý opcode, rezervovaný 0) vrací jako
  `std::unexpected`, nikdy nezacyklí. 4 nové unit testy (synthetic
  streamy + truncation + terrain-patch fall-through).
- **`xpscenery-cli dsf-stats`** — rozšířen o řádek `pools (N):` s
  per-pool `planes × records × bits` a `cmds : objects=… chains=…
  polygons=… patches=… triangles=… verts=… bytes=…`; ekvivalentní
  klíče v `--json` výstupu.
- **3 nové unit testy v `tests/unit/io_dsf/test_dsf_pool.cpp`**: raw
  16-bit pool s různými scale/offset, RLE+differenced re-akumulace,
  detekce POOL bez SCAL. Celkem 73/73 testů zelené v Release.

### Added (Fáze 1C — DSF prohloubení)
- **`xpscenery-cli dsf-stats <file>`** — jeden komplexní souhrn DSF:
  velikost, header verze, atomy, properties, DEFN počty, rastry,
  MD5 status, odvozená dlaždice z `sim/west`/`sim/south`. `--json`.
- **`modules/io_obj/`** — reader X-Plane OBJ8 textového formátu:
  platforma (A/I), verze (700/800/850/…), `TEXTURE`/`TEXTURE_LIT`/
  `TEXTURE_NORMAL`, `POINT_COUNTS`, počty draw-commandů (TRIS/LINES/
  LIGHTS) a `ANIM_begin`. `inspect` vypíše obj souhrn. 4 testy
  (minimal preamble, junk, kompletní příklad, invalid preamble).
- **`io_dsf::verify_md5_footer()`** — ověření 16-bajtového MD5 podpisu,
  který X-Plane zapisuje na konec každého DSF. Samostatná RFC 1321
  implementace (třída `Md5`, `Md5::of()`, `to_hex()`), bez externí
  závislosti. `inspect` vypíše `md5 : ok (<hex>)` nebo `md5 : MISMATCH`
  s oběma digesty. 7 nových jednotkových testů včetně RFC test-vectors
  (empty, "abc", "message digest", 80-char) a inkrementální vs.
  one-shot hashování.
- **`io_dsf::read_rasters()`** — parser DEMI rastrových hlaviček
  (verze, bpp, flags, šířka×výška, scale, offset), skládá jména vrstev
  z DEFN → DEMN. `inspect` vypisuje řádek na rastr s názvem, formátem
  a rozlišením.

### Added (Fáze 1B — io_dsf + geodesy + detectors)
- **`xpscenery-cli bbox --tile`** — pro DSF dlaždici vypíše délky
  hran S/N/W/E, diagonálu a přibližnou plochu v km² (vše přes Vincenty).
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
