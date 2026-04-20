#pragma once
// xpscenery -- io_dsf/dsf_atoms.hpp
//
// Top-level atom walker. An atom is an 8-byte header (uint32 id +
// uint32 size-incl-header) followed by payload bytes.
//
// Atom IDs are stored as 4 little-endian bytes; the X-Plane source
// uses multi-char literals like 'HEAD' which on little-endian hosts
// produce the same int32 that ends up in the file. We decode back
// into a human-readable 4-char tag (big-endian view) for display.

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

#include "xpscenery/io_dsf/dsf_header.hpp"

namespace xps::io_dsf
{

    struct TopLevelAtom
    {
        std::string tag; ///< 4-character human-readable, e.g. "HEAD","DEFN","GEOD"
        std::uint32_t raw_id = 0;
        std::uint64_t offset = 0;
        std::uint32_t size = 0; ///< includes the 8-byte atom header
    };

    /// Walk the top-level atoms of a DSF file. Stops cleanly at the MD5
    /// footer boundary. Does NOT recurse into atom-of-atoms.
    [[nodiscard]] std::expected<std::vector<TopLevelAtom>, std::string>
    read_top_level_atoms(const std::filesystem::path &dsf_path);

} // namespace xps::io_dsf
