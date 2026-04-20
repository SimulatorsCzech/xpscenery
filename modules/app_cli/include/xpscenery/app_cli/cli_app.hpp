#pragma once
// xpscenery — app_cli/cli_app.hpp
//
// Thin wrapper around CLI11 that owns the subcommand set and runs them.

namespace xps::app_cli {

/// Parse argv, dispatch to the right subcommand, return a Unix exit code.
int run(int argc, char** argv) noexcept;

}  // namespace xps::app_cli
