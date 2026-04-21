#pragma once
// xpscenery — io_vector/shp_header.hpp
//
// ESRI Shapefile (.shp) main-file 100-byte header parser.
//
// Layout (per ESRI whitepaper, "ESRI Shapefile Technical Description",
// July 1998):
//     bytes  0..3   : int32 BE   file code (0x0000270A = 9994)
//     bytes  4..23  : 5 × int32 BE unused (must be 0)
//     bytes 24..27  : int32 BE   file length in 16-bit words (incl. header)
//     bytes 28..31  : int32 LE   version (1000)
//     bytes 32..35  : int32 LE   shape type (0=null, 1=point, 3=polyline,
//                                 5=polygon, 8=multipoint, 11=pointZ, …)
//     bytes 36..67  : 4 × double LE   Xmin, Ymin, Xmax, Ymax
//     bytes 68..99  : 4 × double LE   Zmin, Zmax, Mmin, Mmax
//
// We do byte-level decoding (no endian-casting surprises on BE hosts)
// via std::memcpy + manual swap where needed.

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>

namespace xps::io_vector
{

    /// ESRI shape-type constants — union of 2D/3D/measured variants.
    enum class ShapeType : std::int32_t
    {
        null_shape = 0,
        point = 1,
        polyline = 3,
        polygon = 5,
        multipoint = 8,
        point_z = 11,
        polyline_z = 13,
        polygon_z = 15,
        multipoint_z = 18,
        point_m = 21,
        polyline_m = 23,
        polygon_m = 25,
        multipoint_m = 28,
        multipatch = 31,
    };

    /// Human-readable ASCII name; "unknown(N)" for unexpected types.
    [[nodiscard]] std::string shape_type_name(std::int32_t raw) noexcept;

    struct ShpHeader
    {
        std::int32_t file_code = 0;      ///< 9994
        std::int32_t version = 0;        ///< 1000
        std::int32_t shape_type_raw = 0; ///< see ShapeType
        std::uint64_t file_length_bytes = 0; ///< header length-field × 2

        double x_min = 0, y_min = 0, x_max = 0, y_max = 0;
        double z_min = 0, z_max = 0, m_min = 0, m_max = 0;

        [[nodiscard]] ShapeType shape_type() const noexcept
        {
            return static_cast<ShapeType>(shape_type_raw);
        }
        [[nodiscard]] bool has_valid_bbox() const noexcept
        {
            return x_max > x_min && y_max > y_min;
        }
    };

    /// Read the first 100 bytes of `path` and parse as ESRI Shapefile
    /// header. Returns an error on I/O failure, short read, or when
    /// file_code != 9994.
    [[nodiscard]] std::expected<ShpHeader, std::string>
    read_shp_header(const std::filesystem::path &path);

} // namespace xps::io_vector
