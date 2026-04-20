#pragma once
// xpscenery — io_raster/geotiff_ifd.hpp
//
// Classic (32-bit) TIFF and BigTIFF (64-bit) IFD walker + GeoTIFF key
// extraction.
//
// Classic TIFF IFD = uint16 entry_count, then entry_count × 12-byte entries:
//     uint16 tag,
//     uint16 field_type,
//     uint32 count,
//     uint32 value_or_offset   (inline if payload ≤ 4 bytes)
// followed by uint32 next_ifd_offset (0 = end).
//
// BigTIFF IFD = uint64 entry_count, then entry_count × 20-byte entries:
//     uint16 tag,
//     uint16 field_type,
//     uint64 count,
//     uint64 value_or_offset   (inline if payload ≤ 8 bytes)
// followed by uint64 next_ifd_offset.
//
// We inspect only the first IFD (enough for preview / pre-flight).

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace xps::io_raster
{

    /// One (lat, lon) + (x, y) tie-point pair from tag 33922 ModelTiepointTag
    /// (6 doubles per tie: I, J, K, X, Y, Z).
    struct ModelTiepoint
    {
        double i = 0, j = 0, k = 0; ///< raster-space
        double x = 0, y = 0, z = 0; ///< world-space
    };

    /// Subset of keys from GeoKeyDirectoryTag (34735). Each key is
    /// (key_id, tiff_tag_location, count, value_offset).
    struct GeoKeyEntry
    {
        std::uint16_t key_id = 0;
        std::uint16_t location = 0;
        std::uint16_t count = 0;
        std::uint16_t value = 0;
    };

    /// Flat summary of the first IFD of a classic TIFF / GeoTIFF.
    struct GeoTiffInfo
    {
        bool is_bigtiff = false;             ///< true for BigTIFF inputs
        std::uint32_t width = 0;             ///< tag 256 ImageWidth
        std::uint32_t height = 0;            ///< tag 257 ImageLength
        std::uint16_t bits_per_sample = 0;   ///< tag 258 (first value)
        std::uint16_t samples_per_pixel = 1; ///< tag 277
        std::uint16_t photometric = 0;       ///< tag 262
        std::uint16_t compression = 1;       ///< tag 259
        std::uint16_t sample_format = 1;     ///< tag 339 (1=uint, 3=ieee)
        std::uint32_t rows_per_strip = 0;    ///< tag 278
        std::uint32_t strip_count = 0;       ///< from tag 273 count

        /// ModelPixelScaleTag (33550): 3 doubles (scale x, scale y, scale z).
        bool have_pixel_scale = false;
        double pixel_scale_x = 0, pixel_scale_y = 0, pixel_scale_z = 0;

        /// ModelTiepointTag (33922).
        std::vector<ModelTiepoint> tiepoints;

        /// GeoKeyDirectoryTag (34735) as parsed entries (skipping header).
        std::vector<GeoKeyEntry> geo_keys;
        std::uint16_t geo_key_revision = 0;
        std::uint16_t geo_key_minor_revision = 0;

        /// True if tag 33550 or 33922 or 34735 was present.
        bool has_geokeys() const noexcept { return !geo_keys.empty(); }
        bool is_georeferenced() const noexcept
        {
            return have_pixel_scale || !tiepoints.empty();
        }
    };

    /// Parse the first IFD of `path`. Supports both classic (32-bit) TIFF
    /// and BigTIFF (64-bit). Returns an error for non-TIFF inputs or
    /// malformed IFDs.
    [[nodiscard]] std::expected<GeoTiffInfo, std::string>
    read_geotiff_ifd(const std::filesystem::path &path);

} // namespace xps::io_raster
