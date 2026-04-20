// xpscenery-cli inspect-config — load a tile config JSON and report
// its structure (schema version, tile, AOI, layers, export knobs).
// Surfaces the `io_config::TileConfig` type via the CLI for v0.2.0.
#include "commands.hpp"

#include "xpscenery/io_config/tile_config.hpp"
#include "xpscenery/io_filesystem/paths.hpp"

#include <CLI/CLI.hpp>

#include <cstdio>
#include <filesystem>
#include <print>
#include <string>

namespace xps::app_cli::detail
{

    void register_inspect_config(CLI::App &root)
    {
        auto *cmd = root.add_subcommand(
            "inspect-config",
            "Parse a tile config JSON and print its structure "
            "(schema, tile, AOI, layers, meshing, export)");

        static std::string src_arg;
        static bool as_json = false;
        cmd->add_option("src", src_arg, "Tile config JSON file")->required();
        cmd->add_flag("--json", as_json, "Echo a normalised JSON dump");

        cmd->callback([]
                      {
            namespace fs = std::filesystem;
            auto resolved = xps::io_filesystem::require_existing_file(fs::path{src_arg});
            if (!resolved) {
                std::fprintf(stderr, "inspect-config: %s\n", resolved.error().c_str());
                std::exit(1);
            }
            auto cfg = xps::io_config::load(*resolved);
            if (!cfg) {
                std::fprintf(stderr, "inspect-config: %s\n", cfg.error().c_str());
                std::exit(2);
            }

            if (as_json) {
                std::println("{}", xps::io_config::dump(*cfg));
                return;
            }

            std::println("source           : {}", resolved->string());
            std::println("schema version   : {}", cfg->schema_version);
            std::println("tile             : {}", cfg->tile.canonical_name());
            std::println("  lat,lon        : {},{}", cfg->tile.lat(), cfg->tile.lon());
            std::println("  supertile      : {}", cfg->tile.supertile_name());
            if (cfg->aoi) {
                std::println("aoi              : {}..{} , {}..{}",
                    cfg->aoi->sw().lat(), cfg->aoi->ne().lat(),
                    cfg->aoi->sw().lon(), cfg->aoi->ne().lon());
            } else {
                std::println("aoi              : (none — full tile)");
            }
            std::println("output dsf       : {}", cfg->output_dsf.string());
            std::println("");
            std::println("meshing");
            std::println("  max edge (m)   : {}",     cfg->meshing.max_edge_m);
            std::println("  min tri area   : {} m^2", cfg->meshing.min_triangle_area_m2);
            std::println("  smoothing      : {}",     cfg->meshing.smoothing_factor);
            std::println("");
            std::println("export");
            std::println("  bit-identical  : {}", cfg->xp_export.bit_identical_baseline ? "yes" : "no");
            std::println("  overlay        : {}", cfg->xp_export.generate_overlay      ? "yes" : "no");
            std::println("  compress       : {}", cfg->xp_export.compress              ? "yes" : "no");
            std::println("");
            std::println("layers ({})", cfg->layers.size());
            std::println("  {:<2}  {:<10}  {:<12}  {:<4}  {:<8}  path",
                         "#", "id", "kind", "prio", "enabled");
            std::println("  ---------------------------------------------------------------");
            for (std::size_t i = 0; i < cfg->layers.size(); ++i) {
                const auto& l = cfg->layers[i];
                std::println("  {:<2}  {:<10}  {:<12}  {:<4}  {:<8}  {}",
                    i, l.id.empty() ? "(none)" : l.id,
                    l.kind.empty() ? "(?)" : l.kind,
                    l.priority,
                    l.enabled ? "yes" : "no",
                    l.path.string());
            } });
    }

} // namespace xps::app_cli::detail
