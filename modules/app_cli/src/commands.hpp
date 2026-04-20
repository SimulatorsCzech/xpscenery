#pragma once
// Internal — subcommand registrars.

namespace CLI { class App; }

namespace xps::app_cli::detail {

void register_version(CLI::App& root);
void register_inspect(CLI::App& root);
void register_validate(CLI::App& root);

}  // namespace xps::app_cli::detail
