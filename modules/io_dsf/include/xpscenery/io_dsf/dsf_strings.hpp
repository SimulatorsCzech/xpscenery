#pragma once
// xpscenery -- io_dsf/dsf_strings.hpp
//
// Helpers for descending into DSF atom-of-atoms ("HEAD", "DEFN",
// "GEOD", "DEMS") and reading the string-table payload atoms
// ("PROP" / "TERT" / "OBJT" / "POLY" / "NETW" / "DEMN").
//
// All string tables in DSF are identical in structure: a contiguous
// sequence of NUL-terminated C strings. For PROP specifically, pairs
// of consecutive strings are interpreted as (key, value).

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "xpscenery/io_dsf/dsf_atoms.hpp"

namespace xps::io_dsf {

using PropertyList = std::vector<std::pair<std::string, std::string>>;

/// Enumerate atoms nested inside a parent atom (atom-of-atoms style).
/// `parent` must be one of HEAD/DEFN/GEOD/DEMS. Returns the direct
/// children with their absolute file offsets / sizes.
[[nodiscard]] std::expected<std::vector<TopLevelAtom>, std::string>
read_child_atoms(const std::filesystem::path& dsf_path,
                 const TopLevelAtom& parent);

/// Read a flat string table atom (PROP/TERT/OBJT/POLY/NETW/DEMN): the
/// payload (between the 8-byte atom header and the atom's end) is a
/// run of NUL-terminated strings. Empty strings are preserved only
/// when they appear between non-empty strings (a trailing NUL is
/// standard and is not reported as an extra empty entry).
[[nodiscard]] std::expected<std::vector<std::string>, std::string>
read_string_table(const std::filesystem::path& dsf_path,
                  const TopLevelAtom& atom);

/// Convenience: locate HEAD → PROP and decode it as a property list.
/// Returns an empty list (not an error) if HEAD has no PROP child.
[[nodiscard]] std::expected<PropertyList, std::string>
read_properties(const std::filesystem::path& dsf_path);

/// Identifier for a DEFN string-table child.
enum class DefnKind {
    terrain_types,  ///< "TERT" — `.ter` filenames
    object_defs,    ///< "OBJT" — `.obj` filenames
    polygon_defs,   ///< "POLY" — `.pol`, `.ags`, `.agb`, `.for`, `.fac`
    network_defs,   ///< "NETW" — `.net` filenames
    raster_names,   ///< "DEMN" — DEM/raster layer names
};

/// Read one of the DEFN string tables. Returns empty vector (not an
/// error) if DEFN is absent or the requested child is missing.
[[nodiscard]] std::expected<std::vector<std::string>, std::string>
read_defn_strings(const std::filesystem::path& dsf_path, DefnKind kind);

}  // namespace xps::io_dsf
