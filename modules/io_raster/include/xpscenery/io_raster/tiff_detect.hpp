#pragma once
// xpscenery — io_raster/tiff_detect.hpp
//
// Byte-level TIFF / BigTIFF / GeoTIFF detection. We deliberately avoid
// pulling in libtiff / GDAL at this stage — the goal is *pre-flight*
// filtering so the CLI can report reasonable error messages and skip
// obviously-wrong inputs before a heavier loader kicks in.

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>

namespace xps::io_raster {

enum class TiffKind {
    none,       ///< Not a TIFF at all.
    tiff_le,    ///< Classic TIFF, little-endian ("II*\0").
    tiff_be,    ///< Classic TIFF, big-endian    ("MM\0*").
    bigtiff_le, ///< BigTIFF, little-endian      ("II+\0", version 43).
    bigtiff_be, ///< BigTIFF, big-endian         ("MM\0+", version 43).
};

struct TiffInfo {
    TiffKind kind = TiffKind::none;
    /// True once a BigTIFF signature is recognised (offset/pointer width = 64b).
    bool     is_bigtiff = false;
    /// True if byte order marker is little-endian ("II").
    bool     little_endian = false;
    /// First IFD offset (32b for classic, 64b for BigTIFF). 0 if unavailable.
    std::uint64_t first_ifd_offset = 0;
};

/// Inspect the first 16 bytes of `path` and classify the file.
/// Returns `TiffKind::none` for non-TIFFs (still a success, *not* an error).
/// Errors are reserved for I/O failures (missing file, permission denied…).
[[nodiscard]] std::expected<TiffInfo, std::string>
detect_tiff(const std::filesystem::path& path);

/// Pure in-memory variant used primarily by tests. `data` must point to
/// at least `size` bytes. Never reads past the supplied buffer.
[[nodiscard]] TiffInfo detect_tiff_bytes(const std::uint8_t* data,
                                         std::size_t size) noexcept;

}  // namespace xps::io_raster
