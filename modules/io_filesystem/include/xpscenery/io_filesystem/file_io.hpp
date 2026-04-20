#pragma once
// xpscenery — io_filesystem/file_io.hpp
//
// Robust whole-file read/write. Binary-safe, reports size, handles empty
// files, returns meaningful errors instead of throwing.

#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace xps::io_filesystem {

/// Read an entire file into a byte buffer. Uses pread-like whole-file
/// allocate-and-read to avoid iostream overhead.
[[nodiscard]] std::expected<std::vector<std::byte>, std::string>
read_binary(const std::filesystem::path& path);

/// Read an entire text file into a UTF-8 string.
[[nodiscard]] std::expected<std::string, std::string>
read_text_utf8(const std::filesystem::path& path);

/// Write bytes to a file, atomically on failure (temp + rename).
[[nodiscard]] std::expected<std::monostate, std::string>
write_binary_atomic(const std::filesystem::path& path,
                    std::span<const std::byte> data);

/// Write UTF-8 text atomically.
[[nodiscard]] std::expected<std::monostate, std::string>
write_text_utf8_atomic(const std::filesystem::path& path, std::string_view text);

}  // namespace xps::io_filesystem
