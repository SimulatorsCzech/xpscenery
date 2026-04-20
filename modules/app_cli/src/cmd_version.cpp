#include "commands.hpp"

#include "xpscenery/core_types/version_info.hpp"

#include <CLI/CLI.hpp>

#include <cstdio>
#include <print>
#include <string>

namespace xps::app_cli::detail {

void register_version(CLI::App& root) {
    auto* cmd = root.add_subcommand("version", "Print version + build info");
    static bool json = false;
    cmd->add_flag("--json", json, "Emit machine-readable JSON");

    cmd->callback([] {
        const auto& v = xps::core_types::current_version();
        if (json) {
            std::println(
                R"({{"name":"xpscenery","version":"{}","git":"{}","build":"{}","compiler":"{}","cxx":{}}})",
                v.semver, v.git_sha, v.build_type, v.cxx_compiler, v.cplusplus);
        } else {
            std::println("xpscenery {}  (git {}, {}, {}, C++{})",
                v.semver, v.git_sha, v.build_type, v.cxx_compiler, v.cplusplus);
        }
    });
}

}  // namespace xps::app_cli::detail
