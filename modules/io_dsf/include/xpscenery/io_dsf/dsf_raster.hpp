#pragma once
// xpscenery -- io_dsf/dsf_raster.hpp
//
// Parser pro DSF DEMS atom tree (rastrová data). Každý rastr má
// dvojici podsložek:
//   DEMI — 20-byte header (version, bpp, flags, width, height, scale, offset)
//   DEMD — raw pixel payload (width*height*bpp bytes, interpretováno dle flags)
//
// Tato vrstva parsuje pouze DEMI hlavičky; samotná dekoda DEMD
// (endianness, post/area, float/int) přijde v další fázi.

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

#include "xpscenery/io_dsf/dsf_atoms.hpp"

namespace xps::io_dsf
{

    enum class RasterFormat : std::uint8_t
    {
        // Mirror of dsf_Raster_Format_* from X-Plane DSFDefs.h.
        floating = 0,
        signed_int = 1,
        unsigned_int = 2,
        unsigned_norm = 3,
    };

    struct RasterHeader
    {
        std::uint8_t version = 0;
        std::uint8_t bytes_per_pixel = 0;
        std::uint16_t flags = 0;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        float scale = 1.0f;
        float offset = 0.0f;

        /// Name of the raster layer pulled from DEFN → DEMN (optional,
        /// populated by `read_rasters()` when layer names are available).
        std::string name{};

        [[nodiscard]] constexpr RasterFormat format() const noexcept
        {
            return static_cast<RasterFormat>(flags & 0x03);
        }
        [[nodiscard]] constexpr bool is_post() const noexcept
        {
            return (flags & 0x04) != 0;
        }
    };

    /// Enumerate all rasters declared in a DSF. Returns empty vector when
    /// the file has no DEMS atom.
    [[nodiscard]] std::expected<std::vector<RasterHeader>, std::string>
    read_rasters(const std::filesystem::path &dsf_path);

} // namespace xps::io_dsf
