#pragma once
// Internal — subcommand registrars.

namespace CLI
{
    class App;
}

namespace xps::app_cli::detail
{

    void register_version(CLI::App &root);
    void register_inspect(CLI::App &root);
    void register_validate(CLI::App &root);
    void register_distance(CLI::App &root);
    void register_tile(CLI::App &root);
    void register_bbox(CLI::App &root);
    void register_dsf_stats(CLI::App &root);
    void register_dsf_rewrite(CLI::App &root);
    void register_raster_info(CLI::App &root);

} // namespace xps::app_cli::detail
