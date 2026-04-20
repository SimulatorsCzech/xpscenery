// xpscenery-cli obj-stats — report OBJ8 preamble + draw-command
// counts. Surfaces io_obj::read_obj_info() via the CLI for v0.4.0.
#include "commands.hpp"

#include "xpscenery/io_filesystem/paths.hpp"
#include "xpscenery/io_obj/obj_info.hpp"

#include <CLI/CLI.hpp>

#include <cstdio>
#include <filesystem>
#include <print>
#include <string>

namespace xps::app_cli::detail
{

    void register_obj_stats(CLI::App &root)
    {
        auto *cmd = root.add_subcommand(
            "obj-stats",
            "Report the OBJ8 preamble and draw-command counts "
            "(textures, POINT_COUNTS, TRIS/LINES/LIGHTS, ANIM_begin)");

        static std::string src_arg;
        static bool as_json = false;
        cmd->add_option("src", src_arg, "OBJ8 file")->required();
        cmd->add_flag("--json", as_json, "Emit machine-readable JSON");

        cmd->callback([]
                      {
            namespace fs = std::filesystem;
            auto resolved = xps::io_filesystem::require_existing_file(fs::path{src_arg});
            if (!resolved) {
                std::fprintf(stderr, "obj-stats: %s\n", resolved.error().c_str());
                std::exit(1);
            }
            auto info = xps::io_obj::read_obj_info(*resolved);
            if (!info) {
                std::fprintf(stderr, "obj-stats: %s\n", info.error().c_str());
                std::exit(2);
            }

            const char platform_c = static_cast<char>(info->platform);

            if (as_json) {
                std::print(R"({{"source":"{}","platform":"{}","version":{})",
                           resolved->string(), platform_c, info->version);
                std::print(R"(,"texture":"{}","texture_lit":"{}","texture_normal":"{}")",
                           info->texture, info->texture_lit, info->texture_normal);
                std::print(R"(,"vt":{},"vline":{},"vlight":{},"idx":{})",
                           info->vt_count, info->vline_count,
                           info->vlight_count, info->idx_count);
                std::println(R"(,"tris_cmds":{},"lines_cmds":{},"lights_cmds":{},"anim_begin":{}}})",
                             info->tris_commands, info->lines_commands,
                             info->lights_commands, info->anim_begin);
                return;
            }

            std::println("source           : {}",    resolved->string());
            std::println("platform         : {}",    platform_c);
            std::println("version          : {}",    info->version);
            std::println("texture          : {}",    info->texture.empty()
                                                         ? "(none)" : info->texture);
            if (!info->texture_lit.empty())
                std::println("texture (lit)    : {}", info->texture_lit);
            if (!info->texture_normal.empty())
                std::println("texture (normal) : {}", info->texture_normal);
            std::println("");
            std::println("POINT_COUNTS     vt={}  vline={}  vlight={}  idx={}",
                         info->vt_count, info->vline_count,
                         info->vlight_count, info->idx_count);
            std::println("draw commands    TRIS={}  LINES={}  LIGHTS={}",
                         info->tris_commands, info->lines_commands,
                         info->lights_commands);
            std::println("animation blocks ANIM_begin={}", info->anim_begin); });
    }

} // namespace xps::app_cli::detail
