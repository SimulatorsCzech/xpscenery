#include "xpscenery/app_cli/cli_app.hpp"
#include "commands.hpp"

#include "xpscenery/io_logging/logger.hpp"

#include <CLI/CLI.hpp>

#include <exception>
#include <iostream>
#include <string>

namespace xps::app_cli
{

    namespace
    {

        void install_default_logging(const std::string &log_level)
        {
            using xps::io_logging::Level;
            Level lvl = Level::info;
            if (log_level == "trace")
                lvl = Level::trace;
            else if (log_level == "debug")
                lvl = Level::debug;
            else if (log_level == "info")
                lvl = Level::info;
            else if (log_level == "warn")
                lvl = Level::warn;
            else if (log_level == "error")
                lvl = Level::err;
            else if (log_level == "critical")
                lvl = Level::critical;
            else if (log_level == "off")
                lvl = Level::off;

            xps::io_logging::Config cfg;
            cfg.console_level = lvl;
            cfg.file_level = Level::debug;
            cfg.console_enabled = true;
            xps::io_logging::init(cfg);
        }

    } // namespace

    int run(int argc, char **argv) noexcept
    {
        try
        {
            CLI::App app{
                "xpscenery — modern Windows GIS + scenery authoring tool for X-Plane 12"};
            app.require_subcommand(0, 1); // allow no subcommand (for --help/--version)
            app.footer(
                "Docs : https://github.com/SimulatorsCzech/xpscenery\n"
                "Issue: https://github.com/SimulatorsCzech/xpscenery/issues");

            std::string log_level = "info";
            app.add_option("-L,--log-level", log_level,
                           "Global log verbosity: trace|debug|info|warn|error|critical|off")
                ->default_val("info")
                ->capture_default_str();

            detail::register_version(app);
            detail::register_inspect(app);
            detail::register_validate(app);
            detail::register_distance(app);
            detail::register_tile(app);
            detail::register_bbox(app);
            detail::register_dsf_stats(app);

            CLI11_PARSE(app, argc, argv);

            install_default_logging(log_level);

            if (app.get_subcommands().empty())
            {
                std::cout << app.help();
                return 0;
            }
            return 0;
        }
        catch (const CLI::ParseError &e)
        {
            return CLI::App{}.exit(e);
        }
        catch (const std::exception &e)
        {
            std::cerr << "xpscenery: fatal: " << e.what() << '\n';
            return 2;
        }
        catch (...)
        {
            std::cerr << "xpscenery: fatal: unknown exception\n";
            return 3;
        }
    }

} // namespace xps::app_cli
