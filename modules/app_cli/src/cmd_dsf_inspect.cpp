// xpscenery-cli dsf-inspect — enumerate top-level atoms via the
// decomposing writer API (DsfBlob). Output is compact and suitable
// for scripting (tag / offset / size per atom).
#include "commands.hpp"

#include "xpscenery/io_dsf/dsf_writer.hpp"
#include "xpscenery/io_filesystem/paths.hpp"

#include <CLI/CLI.hpp>

#include <cstdio>
#include <filesystem>
#include <print>
#include <string>

namespace xps::app_cli::detail
{

    void register_dsf_inspect(CLI::App &root)
    {
        auto *cmd = root.add_subcommand(
            "dsf-inspect",
            "Enumerate top-level atoms of a DSF via the DsfBlob API. "
            "Prints the blob version, each atom's tag and size in bytes, "
            "and totals suitable for scripting.");

        static std::string src_arg;
        static bool as_json = false;
        cmd->add_option("src", src_arg, "Source .dsf")->required();
        cmd->add_flag("--json", as_json, "Emit machine-readable JSON");

        cmd->callback([]
                      {
            namespace fs = std::filesystem;
            auto resolved = xps::io_filesystem::require_existing_file(fs::path{src_arg});
            if (!resolved) {
                std::fprintf(stderr, "dsf-inspect: %s\n", resolved.error().c_str());
                std::exit(1);
            }
            auto blob = xps::io_dsf::read_dsf_blob(*resolved);
            if (!blob) {
                std::fprintf(stderr, "dsf-inspect: %s\n", blob.error().c_str());
                std::exit(2);
            }

            std::uint64_t total = 0;
            for (const auto& a : blob->atoms)
                total += a.bytes.size();

            if (as_json) {
                std::print(R"({{"source":"{}","version":{},"atoms":[)",
                           resolved->string(), blob->version);
                for (std::size_t i = 0; i < blob->atoms.size(); ++i) {
                    const auto& a = blob->atoms[i];
                    if (i) std::print(",");
                    std::print(R"({{"tag":"{}","raw_id":{},"bytes":{}}})",
                               a.tag, a.raw_id, a.bytes.size());
                }
                std::println(R"(],"total_atom_bytes":{},"count":{}}})",
                             total, blob->atoms.size());
            } else {
                std::println("source      : {}", resolved->string());
                std::println("version     : {}", blob->version);
                std::println("atom count  : {}", blob->atoms.size());
                std::println("");
                std::println("  {:<4}   {:>12}   raw_id", "tag", "bytes");
                std::println("  ----   ------------   ----------");
                for (const auto& a : blob->atoms) {
                    std::println("  {:<4}   {:>12}   0x{:08X}",
                                 a.tag, a.bytes.size(), a.raw_id);
                }
                std::println("");
                std::println("total atom bytes: {}", total);
            } });
    }

} // namespace xps::app_cli::detail
