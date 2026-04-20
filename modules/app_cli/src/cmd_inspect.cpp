#include "commands.hpp"

#include "xpscenery/core_types/lat_lon.hpp"
#include "xpscenery/core_types/tile_coord.hpp"
#include "xpscenery/io_dsf/dsf_atoms.hpp"
#include "xpscenery/io_dsf/dsf_header.hpp"
#include "xpscenery/io_filesystem/paths.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"
#include "xpscenery/io_osm/osm_detect.hpp"
#include "xpscenery/io_raster/tiff_detect.hpp"

#include <CLI/CLI.hpp>

#include <cstdio>
#include <filesystem>
#include <print>
#include <string>
#include <string_view>

namespace xps::app_cli::detail {

void register_inspect(CLI::App& root) {
    auto* cmd = root.add_subcommand(
        "inspect",
        "Inspect a scenery artifact (DSF file, tile name, or config JSON)");

    static std::string target;
    cmd->add_option("target", target,
        "A file path or a tile name like '+50+015'")->required();

    static bool as_json = false;
    cmd->add_flag("--json", as_json, "Emit machine-readable JSON");

    cmd->callback([] {
        namespace fs = std::filesystem;

        // First: is `target` a tile name?
        if (auto tc = xps::core_types::TileCoord::parse(target); tc) {
            if (as_json) {
                std::println(
                    R"({{"kind":"tile","name":"{}","lat":{},"lon":{},"sw":"{}","ne":"{}","supertile":"{}"}})",
                    tc->canonical_name(), tc->lat(), tc->lon(),
                    tc->sw_corner().to_string(),
                    tc->ne_corner().to_string(),
                    tc->supertile_name());
            } else {
                std::println("Tile      : {}", tc->canonical_name());
                std::println("  lat,lon : {},{}", tc->lat(), tc->lon());
                std::println("  sw,ne   : {}  →  {}",
                    tc->sw_corner().to_string(), tc->ne_corner().to_string());
                std::println("  group   : {}", tc->supertile_name());
            }
            return;
        }

        // Otherwise treat as a file path.
        const fs::path path{target};
        auto resolved = xps::io_filesystem::require_existing_file(path);
        if (!resolved) {
            std::fprintf(stderr, "inspect: %s\n", resolved.error().c_str());
            std::exit(1);
        }

        std::error_code ec;
        const auto size  = fs::file_size(*resolved, ec);
        const auto ext   = resolved->extension().string();
        const auto stem  = resolved->stem().string();

        // If the filename matches a tile pattern, enrich with tile info.
        auto maybe_tile = xps::core_types::TileCoord::parse(stem);

        if (as_json) {
            std::println(
                R"({{"kind":"file","path":"{}","size":{},"ext":"{}","tile":{}}})",
                resolved->string(), size, ext,
                maybe_tile ? ("\"" + maybe_tile->canonical_name() + "\"")
                           : std::string{"null"});
        } else {
            std::println("File   : {}", resolved->string());
            std::println("  size : {} bytes", size);
            std::println("  ext  : {}", ext);
            if (maybe_tile) {
                std::println("  tile : {}  (sw {}, ne {})",
                    maybe_tile->canonical_name(),
                    maybe_tile->sw_corner().to_string(),
                    maybe_tile->ne_corner().to_string());
            }
        }

        // DSF enrichment: if the file looks like a DSF, dump header + atoms.
        if (xps::io_dsf::looks_like_dsf(*resolved)) {
            auto hdr = xps::io_dsf::read_header(*resolved);
            if (!hdr) {
                std::fprintf(stderr, "inspect: DSF header: %s\n",
                             hdr.error().c_str());
                return;
            }
            auto atoms = xps::io_dsf::read_top_level_atoms(*resolved);
            if (!atoms) {
                std::fprintf(stderr, "inspect: DSF atoms: %s\n",
                             atoms.error().c_str());
                return;
            }
            if (!as_json) {
                std::println("  dsf  : version={}, atoms={}",
                             hdr->version, atoms->size());
                for (const auto& a : *atoms) {
                    std::println("    [{}] {}  offset={} size={}",
                                 a.tag, a.tag, a.offset, a.size);
                }
            } else {
                std::println(R"({{"dsf":{{"version":{},"atoms":[)",
                             hdr->version);
                for (std::size_t i = 0; i < atoms->size(); ++i) {
                    const auto& a = (*atoms)[i];
                    std::println(
                        R"({}{{"tag":"{}","offset":{},"size":{}}})",
                        (i == 0 ? "" : ","), a.tag, a.offset, a.size);
                }
                std::println("]}}}}");
            }
        }

        // TIFF / GeoTIFF enrichment.
        if (auto tiff = xps::io_raster::detect_tiff(*resolved); tiff) {
            using xps::io_raster::TiffKind;
            std::string_view kind;
            switch (tiff->kind) {
                case TiffKind::tiff_le:    kind = "tiff_le";    break;
                case TiffKind::tiff_be:    kind = "tiff_be";    break;
                case TiffKind::bigtiff_le: kind = "bigtiff_le"; break;
                case TiffKind::bigtiff_be: kind = "bigtiff_be"; break;
                case TiffKind::none:       kind = {};           break;
            }
            if (!kind.empty()) {
                if (!as_json) {
                    std::println("  tiff : kind={}, first_ifd={}",
                                 kind, tiff->first_ifd_offset);
                } else {
                    std::println(
                        R"({{"tiff":{{"kind":"{}","first_ifd":{}}}}})",
                        kind, tiff->first_ifd_offset);
                }
            }
        }

        // OSM enrichment.
        if (auto osm = xps::io_osm::detect_osm(*resolved); osm) {
            using xps::io_osm::OsmKind;
            std::string_view kind;
            switch (osm->kind) {
                case OsmKind::pbf:  kind = "pbf"; break;
                case OsmKind::xml:  kind = "xml"; break;
                case OsmKind::none: kind = {};    break;
            }
            if (!kind.empty()) {
                if (!as_json) {
                    std::println("  osm  : kind={}{}",
                                 kind,
                                 osm->first_blob_header_len
                                     ? (", first_blob_header_len="
                                        + std::to_string(osm->first_blob_header_len))
                                     : std::string{});
                } else {
                    std::println(
                        R"({{"osm":{{"kind":"{}","first_blob_header_len":{}}}}})",
                        kind, osm->first_blob_header_len);
                }
            }
        }
    });
}

}  // namespace xps::app_cli::detail
