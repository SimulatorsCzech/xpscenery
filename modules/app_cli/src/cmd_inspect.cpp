#include "commands.hpp"

#include "xpscenery/core_types/lat_lon.hpp"
#include "xpscenery/core_types/tile_coord.hpp"
#include "xpscenery/io_filesystem/paths.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <CLI/CLI.hpp>

#include <cstdio>
#include <filesystem>
#include <print>
#include <string>

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
    });
}

}  // namespace xps::app_cli::detail
