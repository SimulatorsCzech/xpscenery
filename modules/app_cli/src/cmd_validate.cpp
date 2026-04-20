#include "commands.hpp"

#include "xpscenery/io_config/tile_config.hpp"

#include <CLI/CLI.hpp>

#include <cstdio>
#include <filesystem>
#include <print>
#include <string>

namespace xps::app_cli::detail {

void register_validate(CLI::App& root) {
    auto* cmd = root.add_subcommand(
        "validate",
        "Validate a tile configuration JSON file against the schema");

    static std::string path_str;
    cmd->add_option("path", path_str, "Path to tile config JSON")->required();

    static bool dump_back = false;
    cmd->add_flag("--dump", dump_back,
        "Also print the normalised config (canonical round-trip)");

    cmd->callback([] {
        auto cfg = xps::io_config::load(std::filesystem::path{path_str});
        if (!cfg) {
            std::fprintf(stderr, "validate: %s\n", cfg.error().c_str());
            std::exit(1);
        }
        std::println("OK  schema_version={} tile={} layers={}",
            cfg->schema_version, cfg->tile.canonical_name(), cfg->layers.size());
        if (dump_back) {
            std::println("{}", xps::io_config::dump(*cfg));
        }
    });
}

}  // namespace xps::app_cli::detail
