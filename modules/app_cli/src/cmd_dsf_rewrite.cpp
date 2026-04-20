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
            "Rewrite a DSF to <dst>: identity (default) copies atoms "
            "verbatim; decomposed splits into atoms and recomposes. "
            "Both recompute the MD5 footer and write atomically.");

        static std::string src_arg;
        static std::string dst_arg;
        static std::string mode_arg = "identity";
        cmd->add_option("src", src_arg, "Source .dsf")->required();
        cmd->add_option("dst", dst_arg, "Destination .dsf")->required();
        cmd->add_option("--mode", mode_arg,
                        "Rewrite mode: 'identity' or 'decomposed'")
            ->check(CLI::IsMember({"identity", "decomposed"}))
            ->default_val("identity");

        cmd->callback([]
                      {
            namespace fs = std::filesystem;
            auto resolved = xps::io_filesystem::require_existing_file(fs::path{src_arg});
            if (!resolved) {
                std::fprintf(stderr, "dsf-rewrite: %s\n", resolved.error().c_str());
                std::exit(1);
            }
            const fs::path dst{dst_arg};
            auto rep = (mode_arg == "decomposed")
                ? xps::io_dsf::rewrite_dsf_decomposed(*resolved, dst)
                : xps::io_dsf::rewrite_dsf_identity(*resolved, dst);
            if (!rep) {
                std::fprintf(stderr, "dsf-rewrite: %s\n", rep.error().c_str());
                std::exit(2);
            }
            std::println("rewrote {} ({} bytes, mode={})",
                         resolved->string(), rep->source_size, mode_arg);
            std::println("    -> {} ({} bytes)", dst_arg, rep->output_size);
            std::println("    md5  {}  stored   {}",
                         rep->md5_unchanged ? "ok      " : "changed ",
                         xps::io_dsf::to_hex(rep->stored));
            std::println("                         computed {}",
                         xps::io_dsf::to_hex(rep->computed)); });
    }

} // namespace xps::app_cli::detail
