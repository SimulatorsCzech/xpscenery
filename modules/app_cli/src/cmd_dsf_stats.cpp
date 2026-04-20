#include "commands.hpp"

#include "xpscenery/core_types/tile_coord.hpp"
#include "xpscenery/io_dsf/dsf_atoms.hpp"
#include "xpscenery/io_dsf/dsf_cmds.hpp"
#include "xpscenery/io_dsf/dsf_header.hpp"
#include "xpscenery/io_dsf/dsf_md5.hpp"
#include "xpscenery/io_dsf/dsf_pool.hpp"
#include "xpscenery/io_dsf/dsf_raster.hpp"
#include "xpscenery/io_dsf/dsf_strings.hpp"
#include "xpscenery/io_filesystem/paths.hpp"

#include <CLI/CLI.hpp>

#include <cstdio>
#include <filesystem>
#include <print>
#include <string>
#include <string_view>

namespace xps::app_cli::detail
{

    void register_dsf_stats(CLI::App &root)
    {
        auto *cmd = root.add_subcommand(
            "dsf-stats",
            "Comprehensive summary of a single DSF file "
            "(header, properties, DEFN counts, rasters, MD5)");

        static std::string path_arg;
        cmd->add_option("path", path_arg, "Path to a .dsf file")->required();

        static bool as_json = false;
        cmd->add_flag("--json", as_json, "Emit machine-readable JSON");

        cmd->callback([]
                      {
        namespace fs = std::filesystem;
        const fs::path input{path_arg};
        auto resolved = xps::io_filesystem::require_existing_file(input);
        if (!resolved) {
            std::fprintf(stderr, "dsf-stats: %s\n", resolved.error().c_str());
            std::exit(1);
        }
        if (!xps::io_dsf::looks_like_dsf(*resolved)) {
            std::fprintf(stderr,
                "dsf-stats: '%s' does not have a DSF magic cookie\n",
                resolved->string().c_str());
            std::exit(2);
        }

        auto hdr    = xps::io_dsf::read_header(*resolved);
        auto atoms  = xps::io_dsf::read_top_level_atoms(*resolved);
        auto props  = xps::io_dsf::read_properties(*resolved);
        auto md5    = xps::io_dsf::verify_md5_footer(*resolved);
        auto rast   = xps::io_dsf::read_rasters(*resolved);
        auto pools  = xps::io_dsf::read_point_pools(*resolved);
        auto cmds   = xps::io_dsf::read_cmd_stats(*resolved);

        std::error_code ec;
        const auto file_size = fs::file_size(*resolved, ec);

        using xps::io_dsf::DefnKind;
        struct DefnEntry { DefnKind kind; const char* label; };
        constexpr DefnEntry kDefns[] = {
            {DefnKind::terrain_types, "terrain"},
            {DefnKind::object_defs,   "objects"},
            {DefnKind::polygon_defs,  "polygons"},
            {DefnKind::network_defs,  "networks"},
            {DefnKind::raster_names,  "rasters"},
        };

        // Pull tile info from PROP sim/west, sim/south when available.
        std::string tile_name;
        if (props) {
            int west = 0, south = 0;
            bool have_w = false, have_s = false;
            for (const auto& [k, v] : *props) {
                try {
                    if (k == "sim/west")  { west  = std::stoi(v); have_w = true; }
                    if (k == "sim/south") { south = std::stoi(v); have_s = true; }
                } catch (...) {}
            }
            if (have_w && have_s) {
                if (auto tc = xps::core_types::TileCoord::from_lat_lon(south, west); tc) {
                    tile_name = tc->canonical_name();
                }
            }
        }

        if (!as_json) {
            std::println("DSF     : {}", resolved->string());
            std::println("  size  : {} bytes", file_size);
            if (hdr) {
                std::println("  header: version={}, magic=XPLNEDSF", hdr->version);
            }
            if (!tile_name.empty()) {
                std::println("  tile  : {}", tile_name);
            }
            if (atoms) {
                std::println("  atoms : {}", atoms->size());
                for (const auto& a : *atoms) {
                    std::println("    [{}] offset={} size={}",
                                 a.tag, a.offset, a.size);
                }
            }
            if (props && !props->empty()) {
                std::println("  properties ({}):", props->size());
                for (const auto& [k, v] : *props) {
                    std::println("    {} = {}", k, v);
                }
            }
            for (const auto& e : kDefns) {
                auto v = xps::io_dsf::read_defn_strings(*resolved, e.kind);
                if (v && !v->empty()) {
                    std::println("  defn/{:<8}: {}", e.label, v->size());
                }
            }
            if (rast && !rast->empty()) {
                std::println("  rasters ({}):", rast->size());
                for (std::size_t i = 0; i < rast->size(); ++i) {
                    const auto& r = (*rast)[i];
                    std::println(
                        "    [{}] {}  v{} bpp={} flags=0x{:04x} "
                        "{}x{} scale={} offset={}",
                        i,
                        r.name.empty() ? "(unnamed)" : r.name,
                        r.version, r.bytes_per_pixel, r.flags,
                        r.width, r.height, r.scale, r.offset);
                }
            }
            if (pools && !pools->empty()) {
                std::println("  pools ({}):", pools->size());
                for (std::size_t i = 0; i < pools->size(); ++i) {
                    const auto& pp = (*pools)[i];
                    std::println(
                        "    [{}] planes={} records={} bits={}",
                        i, pp.plane_count, pp.array_size,
                        pp.is_32bit ? 32 : 16);
                }
            }
            if (cmds) {
                std::println(
                    "  cmds  : objects={} chains={} polygons={} "
                    "patches={} triangles={} verts={} bytes={}",
                    cmds->objects_placed, cmds->network_chains,
                    cmds->polygons, cmds->terrain_patches,
                    cmds->triangles_emitted, cmds->triangle_vertices,
                    cmds->bytes_consumed);
            }
            if (md5) {
                if (md5->ok) {
                    std::println("  md5   : ok ({})",
                                 xps::io_dsf::to_hex(md5->stored));
                } else {
                    std::println("  md5   : MISMATCH  stored={}  computed={}",
                                 xps::io_dsf::to_hex(md5->stored),
                                 xps::io_dsf::to_hex(md5->computed));
                }
            }
        } else {
            // Minimal JSON — single line per top-level key for easy diffs.
            std::print("{{");
            std::print(R"("path":"{}","size":{})",
                       resolved->string(), file_size);
            if (hdr) {
                std::print(R"(,"header":{{"version":{}}})", hdr->version);
            }
            if (!tile_name.empty()) {
                std::print(R"(,"tile":"{}")", tile_name);
            }
            if (atoms) {
                std::print(R"(,"atoms":[)");
                for (std::size_t i = 0; i < atoms->size(); ++i) {
                    const auto& a = (*atoms)[i];
                    std::print(R"({}{{"tag":"{}","offset":{},"size":{}}})",
                               (i == 0 ? "" : ","),
                               a.tag, a.offset, a.size);
                }
                std::print("]");
            }
            if (props) {
                std::print(R"(,"properties":{{)");
                for (std::size_t i = 0; i < props->size(); ++i) {
                    const auto& [k, v] = (*props)[i];
                    std::print(R"({}"{}":"{}")",
                               (i == 0 ? "" : ","), k, v);
                }
                std::print("}}");
            }
            std::print(R"(,"defn":{{)");
            bool first = true;
            for (const auto& e : kDefns) {
                auto v = xps::io_dsf::read_defn_strings(*resolved, e.kind);
                if (v) {
                    std::print(R"({}"{}":{})",
                               (first ? "" : ","), e.label, v->size());
                    first = false;
                }
            }
            std::print("}}");
            if (rast) {
                std::print(R"(,"rasters":[)");
                for (std::size_t i = 0; i < rast->size(); ++i) {
                    const auto& r = (*rast)[i];
                    std::print(
                        R"({}{{"name":"{}","version":{},"bpp":{},"flags":{},"width":{},"height":{},"scale":{},"offset":{}}})",
                        (i == 0 ? "" : ","),
                        r.name, r.version, r.bytes_per_pixel,
                        r.flags, r.width, r.height, r.scale, r.offset);
                }
                std::print("]");
            }
            if (pools) {
                std::print(R"(,"pools":[)");
                for (std::size_t i = 0; i < pools->size(); ++i) {
                    const auto& pp = (*pools)[i];
                    std::print(
                        R"({}{{"planes":{},"records":{},"bits":{}}})",
                        (i == 0 ? "" : ","),
                        pp.plane_count, pp.array_size,
                        pp.is_32bit ? 32 : 16);
                }
                std::print("]");
            }
            if (cmds) {
                std::print(
                    R"(,"cmds":{{"objects":{},"chains":{},"polygons":{},"patches":{},"triangles":{},"verts":{},"bytes":{}}})",
                    cmds->objects_placed, cmds->network_chains,
                    cmds->polygons, cmds->terrain_patches,
                    cmds->triangles_emitted, cmds->triangle_vertices,
                    cmds->bytes_consumed);
            }
            if (md5) {
                std::print(
                    R"(,"md5":{{"ok":{},"stored":"{}","computed":"{}"}})",
                    md5->ok ? "true" : "false",
                    xps::io_dsf::to_hex(md5->stored),
                    xps::io_dsf::to_hex(md5->computed));
            }
            std::println("}}");
        } });
    }

} // namespace xps::app_cli::detail
