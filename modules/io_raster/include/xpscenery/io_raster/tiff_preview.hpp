#pragma once
// xpscenery — io_raster/tiff_preview.hpp
//
// Minimal TIFF pixel preview: dekóduje první strip nekomprimovaného TIFF
// (compression = 1 / NoCompression) do raw pixel bufferu. Podporované
// kombinace:
//   - bits_per_sample = 8, samples_per_pixel = 1, photometric ∈ {0, 1}  → Gray8
//   - bits_per_sample = 8, samples_per_pixel = 3, photometric = 2       → RGB8
//
// Fáze 2: stačí rychlý náhled pro uživatele. Pro LZW, Deflate, JPEG a
// další kompresní schémata se vrátí `unsupported_compression` + čitelný
// text chyby; volající (UI) pak náhled přeskočí s upozorněním.

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace xps::io_raster
{

    enum class TiffPreviewFormat : std::uint8_t
    {
        gray8 = 1, ///< 1 B/px, grayscale
        rgb8  = 3, ///< 3 B/px, RGB interleaved
    };

    struct TiffStripPreview
    {
        std::uint32_t image_width  = 0;  ///< tag 256
        std::uint32_t image_height = 0;  ///< tag 257
        std::uint32_t strip_width  = 0;  ///< typicky == image_width
        std::uint32_t strip_height = 0;  ///< min(rows_per_strip, image_height - 0)
        std::uint16_t bits_per_sample   = 8;
        std::uint16_t samples_per_pixel = 1;
        std::uint16_t photometric       = 1;
        TiffPreviewFormat format = TiffPreviewFormat::gray8;
        std::vector<std::uint8_t> pixels; ///< velikost = strip_width * strip_height * samples
    };

    /// Načte první strip nekomprimovaného TIFF/GeoTIFF a vrátí raw pixely
    /// připravené k zobrazení (QImage, SDL, atd.). Selže s čitelným
    /// textem pro BigTIFF, neznámé kombinace bits/samples a pro kompresi
    /// != 1.
    [[nodiscard]] std::expected<TiffStripPreview, std::string>
    read_tiff_first_strip(const std::filesystem::path &path);

} // namespace xps::io_raster
