#pragma once
// xpscenery — io_logging/logger.hpp
//
// Thin, opinionated facade over spdlog. Goals:
//   • One place to configure log level and sinks for the whole app.
//   • Named loggers per subsystem, all routed through a shared sink set.
//   • Async by default, so logging never blocks hot paths.
//   • Works from either CLI or GUI without code changes.

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace spdlog { class logger; }

namespace xps::io_logging {

enum class Level {
    trace, debug, info, warn, err, critical, off
};

struct Config {
    Level       console_level   = Level::info;
    Level       file_level      = Level::debug;
    bool        console_enabled = true;
    bool        json_file       = false;
    std::filesystem::path log_dir;  ///< empty = no file logging
    std::size_t rotate_size_mb   = 32;
    std::size_t rotate_files     = 5;
};

/// Initialise logging once at program start. Safe to call repeatedly; later
/// calls replace sinks but keep loggers alive.
void init(const Config& cfg);

/// Shut down asynchronously; flushes all sinks.
void shutdown() noexcept;

/// Get or create a named logger. Do not store the returned pointer across
/// an `init()` call — retrieve again instead.
std::shared_ptr<spdlog::logger> get(std::string_view name);

}  // namespace xps::io_logging
