#include "commands.hpp"

#include "xpscenery/io_dsf/dsf_md5.hpp"
#include "xpscenery/io_dsf/dsf_writer.hpp"
#include "xpscenery/io_filesystem/paths.hpp"

#include <CLI/CLI.hpp>

#include <cstdio>
#include <filesystem>
#include <print>
#include <string>

namespace xps::app_cli::detail
{

    void register_dsf_rewrite(CLI::App &root)
    {
        auto *cmd = root.add_subcommand(
            "dsf-rewrite",
            "Byte-faithful rewrite of a DSF: copies atoms verbatim, "
            "recomputes MD5 footer, writes atomically to <dst>");

        static std::string src_arg;
        static std::string dst_arg;
        cmd->add_option("src", src_arg, "Source .dsf")->required();
        cmd->add_option("dst", dst_arg, "Destination .dsf")->required();

        cmd->callback([]
                      {
            namespace fs = std::filesystem;
            auto resolved = xps::io_filesystem::require_existing_file(fs::path{src_arg});
            if (!resolved) {
                std::fprintf(stderr, "dsf-rewrite: %s\n", resolved.error().c_str());
                std::exit(1);
            }
            auto rep = xps::io_dsf::rewrite_dsf_identity(*resolved, fs::path{dst_arg});
            if (!rep) {
                std::fprintf(stderr, "dsf-rewrite: %s\n", rep.error().c_str());
                std::exit(2);
            }
            std::println("rewrote {} ({} bytes)", resolved->string(), rep->source_size);
            std::println("    -> {} ({} bytes)", dst_arg, rep->output_size);
            std::println("    md5  {}  stored   {}",
                         rep->md5_unchanged ? "ok      " : "changed ",
                         xps::io_dsf::to_hex(rep->stored));
            std::println("                         computed {}",
                         xps::io_dsf::to_hex(rep->computed)); });
    }

} // namespace xps::app_cli::detail
