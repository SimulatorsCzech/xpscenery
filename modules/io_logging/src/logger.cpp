#include "xpscenery/io_logging/logger.hpp"

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace xps::io_logging {

namespace {

spdlog::level::level_enum to_spd(Level l) noexcept {
    using L = Level;
    switch (l) {
        case L::trace:    return spdlog::level::trace;
        case L::debug:    return spdlog::level::debug;
        case L::info:     return spdlog::level::info;
        case L::warn:     return spdlog::level::warn;
        case L::err:      return spdlog::level::err;
        case L::critical: return spdlog::level::critical;
        case L::off:      return spdlog::level::off;
    }
    return spdlog::level::info;
}

struct State {
    std::mutex mtx;
    std::vector<spdlog::sink_ptr> sinks;
    std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> registry;
    bool initialised = false;
};

State& state() {
    static State s;
    return s;
}

}  // namespace

void init(const Config& cfg) {
    auto& s = state();
    std::lock_guard lock{s.mtx};

    s.sinks.clear();

    if (cfg.console_enabled) {
        auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console->set_level(to_spd(cfg.console_level));
        s.sinks.push_back(console);
    }

    if (!cfg.log_dir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(cfg.log_dir, ec);
        const auto file_path = cfg.log_dir / "xpscenery.log";
        auto file = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            file_path.string(),
            cfg.rotate_size_mb * 1024u * 1024u,
            cfg.rotate_files);
        file->set_level(to_spd(cfg.file_level));
        if (cfg.json_file) {
            // Minimal JSON-ish line format. A dedicated JSON sink can come later.
            file->set_pattern(
                R"({"ts":"%Y-%m-%dT%H:%M:%S.%e","lvl":"%l","logger":"%n","msg":"%v"})");
        }
        s.sinks.push_back(file);
    }

    // Re-point existing loggers at the new sink set.
    for (auto& [name, logger] : s.registry) {
        logger->sinks() = s.sinks;
    }

    // Global fallback logger.
    if (!s.initialised) {
        spdlog::init_thread_pool(8192, 1);
        s.initialised = true;
    }
}

void shutdown() noexcept {
    try {
        spdlog::shutdown();
        auto& s = state();
        std::lock_guard lock{s.mtx};
        s.registry.clear();
        s.sinks.clear();
        s.initialised = false;
    } catch (...) {
        // Never throw from shutdown.
    }
}

std::shared_ptr<spdlog::logger> get(std::string_view name) {
    auto& s = state();
    std::lock_guard lock{s.mtx};

    const std::string key{name};
    if (auto it = s.registry.find(key); it != s.registry.end()) {
        return it->second;
    }

    // If init() hasn't been called, install a sane default so early callers
    // don't crash.
    if (s.sinks.empty()) {
        s.sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    }

    auto logger = std::make_shared<spdlog::logger>(key, begin(s.sinks), end(s.sinks));
    logger->set_level(spdlog::level::trace);  // sinks do their own filtering
    s.registry.emplace(key, logger);
    return logger;
}

}  // namespace xps::io_logging
