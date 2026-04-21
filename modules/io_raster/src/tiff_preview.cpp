// xpscenery — io_raster/tiff_preview.cpp  (Phase 2 minimal)
//
// Samostatný minimal-viable dekodér prvního stripu classic TIFF.
// BigTIFF se zatím neparsuje (vrátí chybu) — pro běžné GeoTIFF vstupy
// z RenderFarmu stačí classic.

#include "xpscenery/io_raster/tiff_preview.hpp"

#include <bit>
#include <cstring>
#include <format>
#include <fstream>

namespace xps::io_raster
{

    namespace
    {
        struct Reader
        {
            std::ifstream &f;
            bool little = true;

            template <typename T>
            T load_le(std::uint64_t off)
            {
                f.seekg(static_cast<std::streamoff>(off));
                T v{};
                f.read(reinterpret_cast<char *>(&v), sizeof(T));
                if constexpr (sizeof(T) > 1)
                {
                    const bool need_swap =
                        little ? (std::endian::native == std::endian::big)
                               : (std::endian::native == std::endian::little);
                    if (need_swap)
                    {
                        auto *b = reinterpret_cast<unsigned char *>(&v);
                        std::reverse(b, b + sizeof(T));
                    }
                }
                return v;
            }
        };

        // Extract the N-th value of an IFD entry (classic TIFF, type 3 SHORT
        // or 4 LONG). Handles both inline (count*width <= 4) and offset
        // storage. Returns 0 if out of range.
        std::uint64_t entry_value(
            Reader &r, std::uint16_t type, std::uint32_t count,
            std::uint32_t value_or_offset, std::uint32_t idx)
        {
            const std::size_t w = (type == 3) ? 2 : (type == 4) ? 4 : 0;
            if (w == 0 || idx >= count) return 0;
            const std::size_t total = std::size_t(count) * w;
            if (total <= 4)
            {
                // Inline: little-endian packed into value_or_offset (already
                // byte-swapped at field read time if needed; but value_or_offset
                // is host-side after load_le).
                const std::uint32_t v = value_or_offset;
                const std::uint8_t *b =
                    reinterpret_cast<const std::uint8_t *>(&v);
                if (type == 3)
                {
                    std::uint16_t s = 0;
                    std::memcpy(&s, b + idx * 2, 2);
                    return s;
                }
                std::uint32_t l = 0;
                std::memcpy(&l, b + idx * 4, 4);
                return l;
            }
            if (type == 3)
                return r.load_le<std::uint16_t>(value_or_offset + idx * 2);
            return r.load_le<std::uint32_t>(value_or_offset + idx * 4);
        }
    } // namespace

    std::expected<TiffStripPreview, std::string>
    read_tiff_first_strip(const std::filesystem::path &path)
    {
        std::ifstream f(path, std::ios::binary);
        if (!f)
            return std::unexpected(std::format(
                "tiff_preview: cannot open '{}'", path.string()));

        // Header
        char bom[2];
        f.read(bom, 2);
        if (f.gcount() < 2)
            return std::unexpected("tiff_preview: file too short");

        Reader r{f};
        if (bom[0] == 'I' && bom[1] == 'I') r.little = true;
        else if (bom[0] == 'M' && bom[1] == 'M') r.little = false;
        else return std::unexpected("tiff_preview: not a TIFF (bad BOM)");

        const auto magic = r.load_le<std::uint16_t>(2);
        if (magic == 43)
            return std::unexpected(
                "tiff_preview: BigTIFF zatím nepodporován");
        if (magic != 42)
            return std::unexpected(std::format(
                "tiff_preview: unexpected magic {}", magic));

        const auto ifd0 = r.load_le<std::uint32_t>(4);
        const auto n_entries = r.load_le<std::uint16_t>(ifd0);

        std::uint32_t width = 0, height = 0;
        std::uint16_t bits = 8, samples = 1, photo = 1, compression = 1;
        std::uint32_t rows_per_strip = 0;
        std::uint16_t strip_type = 4;
        std::uint32_t strip_count = 0, strip_value = 0;
        std::uint16_t count_type = 4;
        std::uint32_t count_count = 0, count_value = 0;

        for (std::uint16_t i = 0; i < n_entries; ++i)
        {
            const std::uint64_t base = std::uint64_t(ifd0) + 2 + i * 12;
            const auto tag   = r.load_le<std::uint16_t>(base + 0);
            const auto type  = r.load_le<std::uint16_t>(base + 2);
            const auto count = r.load_le<std::uint32_t>(base + 4);
            const auto vou   = r.load_le<std::uint32_t>(base + 8);

            auto first_as_u32 = [&]() -> std::uint32_t {
                return static_cast<std::uint32_t>(
                    entry_value(r, type, count, vou, 0));
            };
            switch (tag)
            {
            case 256: width  = first_as_u32(); break;
            case 257: height = first_as_u32(); break;
            case 258: bits   = static_cast<std::uint16_t>(first_as_u32()); break;
            case 259: compression = static_cast<std::uint16_t>(first_as_u32()); break;
            case 262: photo  = static_cast<std::uint16_t>(first_as_u32()); break;
            case 277: samples = static_cast<std::uint16_t>(first_as_u32()); break;
            case 278: rows_per_strip = first_as_u32(); break;
            case 273: strip_type  = type; strip_count = count; strip_value = vou; break;
            case 279: count_type  = type; count_count = count; count_value = vou; break;
            default: break;
            }
        }

        if (width == 0 || height == 0)
            return std::unexpected("tiff_preview: missing width/height");
        if (compression != 1)
            return std::unexpected(std::format(
                "tiff_preview: pouze nekomprimovaný TIFF (compression={})", compression));
        if (bits != 8)
            return std::unexpected(std::format(
                "tiff_preview: podporováno jen 8 b/sample (nalezeno {})", bits));
        if (!(samples == 1 || samples == 3))
            return std::unexpected(std::format(
                "tiff_preview: podporováno samples=1 nebo 3 (nalezeno {})", samples));
        if (strip_count == 0 || count_count == 0)
            return std::unexpected("tiff_preview: chybí StripOffsets/ByteCounts");

        const std::uint64_t strip0_offset =
            entry_value(r, strip_type, strip_count, strip_value, 0);
        const std::uint64_t strip0_bytes  =
            entry_value(r, count_type, count_count, count_value, 0);

        if (strip0_offset == 0 || strip0_bytes == 0)
            return std::unexpected("tiff_preview: strip 0 prázdný");

        // Safety clamp: max 64 MB (prevent pathological files).
        constexpr std::uint64_t kMaxBytes = 64ull * 1024 * 1024;
        if (strip0_bytes > kMaxBytes)
            return std::unexpected(std::format(
                "tiff_preview: strip 0 {} B > limit {} B", strip0_bytes, kMaxBytes));

        if (rows_per_strip == 0) rows_per_strip = height;
        const std::uint32_t strip_h =
            (rows_per_strip < height) ? rows_per_strip : height;
        const std::uint64_t expected_bytes =
            std::uint64_t(width) * strip_h * samples;
        if (strip0_bytes < expected_bytes)
            return std::unexpected(std::format(
                "tiff_preview: strip 0 má jen {} B, čekáme {} B",
                strip0_bytes, expected_bytes));

        TiffStripPreview out;
        out.image_width       = width;
        out.image_height      = height;
        out.strip_width       = width;
        out.strip_height      = strip_h;
        out.bits_per_sample   = bits;
        out.samples_per_pixel = samples;
        out.photometric       = photo;
        out.format = (samples == 1) ? TiffPreviewFormat::gray8
                                    : TiffPreviewFormat::rgb8;
        out.pixels.resize(static_cast<std::size_t>(expected_bytes));
        f.clear();
        f.seekg(static_cast<std::streamoff>(strip0_offset));
        f.read(reinterpret_cast<char *>(out.pixels.data()),
               static_cast<std::streamsize>(out.pixels.size()));
        if (f.gcount() < static_cast<std::streamsize>(out.pixels.size()))
            return std::unexpected("tiff_preview: krátké čtení stripu 0");

        // PhotometricInterpretation = 0 (WhiteIsZero): invert for display.
        if (samples == 1 && photo == 0)
            for (auto &b : out.pixels) b = static_cast<std::uint8_t>(255 - b);

        return out;
    }

} // namespace xps::io_raster
