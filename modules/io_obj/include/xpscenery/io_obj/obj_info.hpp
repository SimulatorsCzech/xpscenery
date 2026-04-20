#pragma once
// xpscenery — io_obj/obj_info.hpp
//
// Reader for X-Plane OBJ8 text format. Extracts header + summary
// counts without loading the full mesh into memory. Useful for CLI
// listings and sanity checks over a scenery library's .obj files.

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>

namespace xps::io_obj {

enum class Platform : char {
    unknown   = '?',
    ibm       = 'I',   ///< little-endian
    apple     = 'A',   ///< historical; modern X-Plane reads both
};

/// Summary of an OBJ8 text file. Counts reflect the TRIS/LINES/LIGHTS
/// draw commands encountered (not POINT_COUNTS, which is a
/// buffer-size hint and may differ from the number of draw calls).
struct ObjInfo {
    Platform     platform       = Platform::unknown;
    std::uint32_t version        = 0;  ///< e.g. 800, 850, 900, 1100
    std::string  texture;              ///< albedo texture path
    std::string  texture_lit;          ///< _LIT night texture path
    std::string  texture_normal;       ///< normal map (if TEXTURE_NORMAL)

    /// POINT_COUNTS <vt> <vline> <vlight> <idx>  (0 when absent)
    std::uint32_t vt_count       = 0;
    std::uint32_t vline_count    = 0;
    std::uint32_t vlight_count   = 0;
    std::uint32_t idx_count      = 0;

    /// Draw commands encountered in the ATTRIBUTES block.
    std::uint32_t tris_commands  = 0;
    std::uint32_t lines_commands = 0;
    std::uint32_t lights_commands = 0;

    std::uint32_t anim_begin     = 0;  ///< ANIM_begin markers
};

/// Read the header + draw command counts from an OBJ8 file.
[[nodiscard]] std::expected<ObjInfo, std::string>
read_obj_info(const std::filesystem::path& obj_path);

/// Quick byte check: first non-blank line is "A" or "I", then a line
/// starting with a 3-4 digit number, then "OBJ". Does not validate
/// that the file is syntactically correct beyond the preamble.
[[nodiscard]] bool looks_like_obj8(const std::filesystem::path& obj_path);

}  // namespace xps::io_obj
