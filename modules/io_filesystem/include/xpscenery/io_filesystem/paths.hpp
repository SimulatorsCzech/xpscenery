#pragma once
// xpscenery — io_filesystem/paths.hpp
//
// Convenience wrappers around std::filesystem for X-Plane layout.

#include <expected>
#include <filesystem>
#include <string>

#include "xpscenery/core_types/tile_coord.hpp"

namespace xps::io_filesystem {

/// Build "Earth nav data/<supertile>/<tile>.dsf" path relative to
/// an X-Plane scenery root directory.
[[nodiscard]] std::filesystem::path
dsf_path_for_tile(const std::filesystem::path& scenery_root,
                  const xps::core_types::TileCoord& tile);

/// Verify that `path` exists and is a regular file. Returns the resolved
/// (canonical) path on success.
[[nodiscard]] std::expected<std::filesystem::path, std::string>
require_existing_file(const std::filesystem::path& path);

/// Verify that `path` exists and is a directory.
[[nodiscard]] std::expected<std::filesystem::path, std::string>
require_existing_dir(const std::filesystem::path& path);

/// Ensure `path` exists as a directory (mkdir -p).
[[nodiscard]] std::expected<std::filesystem::path, std::string>
ensure_directory(const std::filesystem::path& path);

}  // namespace xps::io_filesystem
