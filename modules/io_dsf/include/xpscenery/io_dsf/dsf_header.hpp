#pragma once
// xpscenery -- io_dsf/dsf_header.hpp
//
// Minimal reader for the X-Plane DSF file header. DSF layout:
//
//   [0..8)   "XPLNEDSF"  cookie
//   [8..12)  int32_t     master version (little-endian, always 1 today)
//   [12..N-16)           atom tree (4-byte id + 4-byte size, size includes header)
//   [N-16..N)            128-bit MD5 footer over the rest of the file
//
// We intentionally stop short of parsing atom payloads here; the
// top-level atom enumeration lives in dsf_atoms.hpp.

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>

namespace xps::io_dsf {

struct DsfHeaderInfo {
    std::int32_t version       = 0;   ///< master version (expected 1)
    std::uint64_t file_size    = 0;   ///< total file size in bytes
    std::uint64_t atoms_offset = 12;  ///< where the atom tree begins
    std::uint64_t atoms_end    = 0;   ///< file_size - sizeof(MD5 footer)
};

/// Read + validate the 12-byte header. Does NOT parse atoms.
/// On success: returns header info. On failure: descriptive message
/// (bad cookie, unsupported version, truncated file, I/O error...).
[[nodiscard]] std::expected<DsfHeaderInfo, std::string>
read_header(const std::filesystem::path& dsf_path);

/// Convenience: returns true iff `path` is a regular file whose first
/// 8 bytes are "XPLNEDSF". Swallows I/O errors as false.
[[nodiscard]] bool looks_like_dsf(const std::filesystem::path& path) noexcept;

}  // namespace xps::io_dsf
