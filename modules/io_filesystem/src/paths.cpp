#include "xpscenery/io_filesystem/paths.hpp"

#include <format>
#include <system_error>

namespace xps::io_filesystem {

namespace fs = std::filesystem;

fs::path dsf_path_for_tile(const fs::path& scenery_root,
                           const xps::core_types::TileCoord& tile) {
    return scenery_root
        / "Earth nav data"
        / tile.supertile_name()
        / (tile.canonical_name() + ".dsf");
}

std::expected<fs::path, std::string>
require_existing_file(const fs::path& path) {
    std::error_code ec;
    if (!fs::exists(path, ec) || ec) {
        return std::unexpected(std::format(
            "file does not exist: {}", path.string()));
    }
    if (!fs::is_regular_file(path, ec)) {
        return std::unexpected(std::format(
            "path is not a regular file: {}", path.string()));
    }
    auto canonical = fs::weakly_canonical(path, ec);
    if (ec) canonical = path;
    return canonical;
}

std::expected<fs::path, std::string>
require_existing_dir(const fs::path& path) {
    std::error_code ec;
    if (!fs::exists(path, ec) || ec) {
        return std::unexpected(std::format(
            "directory does not exist: {}", path.string()));
    }
    if (!fs::is_directory(path, ec)) {
        return std::unexpected(std::format(
            "path is not a directory: {}", path.string()));
    }
    auto canonical = fs::weakly_canonical(path, ec);
    if (ec) canonical = path;
    return canonical;
}

std::expected<fs::path, std::string>
ensure_directory(const fs::path& path) {
    std::error_code ec;
    fs::create_directories(path, ec);
    if (ec) {
        return std::unexpected(std::format(
            "mkdir failed for {}: {}", path.string(), ec.message()));
    }
    return path;
}

}  // namespace xps::io_filesystem
