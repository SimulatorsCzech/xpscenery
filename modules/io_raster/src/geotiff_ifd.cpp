#include "xpscenery/io_raster/geotiff_ifd.hpp"
#include "xpscenery/io_raster/tiff_detect.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <bit>
#include <cstring>
#include <format>
#include <limits>

namespace xps::io_raster
{

    namespace
    {

        // TIFF field type widths (tag 2 in each IFD entry).
        std::size_t type_width(std::uint16_t t) noexcept
        {
            switch (t)
            {
            case 1: // BYTE
            case 2: // ASCII
            case 6: // SBYTE
            case 7:
                return 1; // UNDEFINED
            case 3:       // SHORT
            case 8:
                return 2; // SSHORT
            case 4:       // LONG
            case 9:       // SLONG
            case 11:
                return 4; // FLOAT
            case 5:       // RATIONAL
            case 10:      // SRATIONAL
            case 12:
                return 8; // DOUBLE
            case 16:      // LONG8 (BigTIFF)
            case 17:      // SLONG8 (BigTIFF)
            case 18:      // IFD8 (BigTIFF)
                return 8;
            default:
                return 0;
            }
        }

        struct Bytes
        {
            const std::byte *ptr;
            std::size_t size;
            bool little;

            template <typename T>
            T load(std::size_t off) const noexcept
            {
                T v{};
                std::memcpy(&v, ptr + off, sizeof(T));
                if constexpr (sizeof(T) > 1)
                {
                    const bool need_swap = little
                                               ? (std::endian::native == std::endian::big)
                                               : (std::endian::native == std::endian::little);
                    if (need_swap)
                    {
                        if constexpr (sizeof(T) == 2)
                        {
                            std::uint16_t u;
                            std::memcpy(&u, &v, 2);
                            u = std::byteswap(u);
                            std::memcpy(&v, &u, 2);
                        }
                        else if constexpr (sizeof(T) == 4)
                        {
                            std::uint32_t u;
                            std::memcpy(&u, &v, 4);
                            u = std::byteswap(u);
                            std::memcpy(&v, &u, 4);
                        }
                        else if constexpr (sizeof(T) == 8)
                        {
                            std::uint64_t u;
                            std::memcpy(&u, &v, 8);
                            u = std::byteswap(u);
                            std::memcpy(&v, &u, 8);
                        }
                    }
                }
                return v;
            }
        };

        struct IfdEntry
        {
            std::uint16_t tag;
            std::uint16_t type;
            std::uint64_t count;              // 32b for classic, 64b for BigTIFF
            std::uint64_t value_or_offset;    // 32b for classic, 64b for BigTIFF
            std::size_t entry_offset;         // offset of the entry itself
            std::size_t inline_payload_width; // 4 (classic) or 8 (BigTIFF)
        };

        // Resolve the file offset where this entry's payload lives.
        // Classic TIFF: inline when total <= 4, else at value_or_offset.
        // BigTIFF:      inline when total <= 8, else at value_or_offset.
        std::size_t payload_offset(const IfdEntry &e, std::size_t type_w) noexcept
        {
            const std::size_t total = type_w * static_cast<std::size_t>(e.count);
            if (total <= e.inline_payload_width)
                return e.entry_offset + (e.inline_payload_width == 8 ? 12 : 8);
            return static_cast<std::size_t>(e.value_or_offset);
        }

        bool read_uint_from_entry(const Bytes &b, const IfdEntry &e,
                                  std::uint32_t &out) noexcept
        {
            const auto w = type_width(e.type);
            if (w == 0 || e.count == 0)
                return false;
            const auto off = payload_offset(e, w);
            if (off + w > b.size)
                return false;
            switch (e.type)
            {
            case 1:
                out = static_cast<std::uint32_t>(b.load<std::uint8_t>(off));
                return true;
            case 3:
                out = static_cast<std::uint32_t>(b.load<std::uint16_t>(off));
                return true;
            case 4:
                out = b.load<std::uint32_t>(off);
                return true;
            case 16: // LONG8 (BigTIFF only) — truncate to u32 when tag is a dimension
                out = static_cast<std::uint32_t>(b.load<std::uint64_t>(off));
                return true;
            default:
                return false;
            }
        }

        bool read_ushort_from_entry(const Bytes &b, const IfdEntry &e,
                                    std::uint16_t &out) noexcept
        {
            std::uint32_t v = 0;
            if (!read_uint_from_entry(b, e, v))
                return false;
            out = static_cast<std::uint16_t>(v);
            return true;
        }

        bool read_doubles(const Bytes &b, const IfdEntry &e,
                          std::vector<double> &out)
        {
            if (e.type != 12)
                return false; // DOUBLE
            const auto w = type_width(e.type);
            const auto off = payload_offset(e, w);
            if (off + w * e.count > b.size)
                return false;
            out.resize(e.count);
            for (std::uint32_t i = 0; i < e.count; ++i)
            {
                std::uint64_t raw = b.load<std::uint64_t>(off + i * 8);
                double d;
                std::memcpy(&d, &raw, 8);
                out[i] = d;
            }
            return true;
        }

    } // namespace

    std::expected<GeoTiffInfo, std::string>
    read_geotiff_ifd(const std::filesystem::path &path)
    {
        auto info = detect_tiff(path);
        if (!info)
            return std::unexpected(std::move(info.error()));
        if (info->kind == TiffKind::none)
            return std::unexpected(std::format(
                "{} is not a TIFF file", path.string()));

        auto bytes = xps::io_filesystem::read_binary(path);
        if (!bytes)
            return std::unexpected(std::move(bytes.error()));

        Bytes b{bytes->data(), bytes->size(), info->little_endian};
        const std::size_t ifd_off = static_cast<std::size_t>(info->first_ifd_offset);

        std::vector<IfdEntry> entries;

        if (info->is_bigtiff)
        {
            // BigTIFF: uint64 num_entries, 20-byte entries, uint64 next_ifd.
            if (ifd_off + 8 > b.size)
                return std::unexpected("first IFD offset past EOF (BigTIFF)");
            const std::uint64_t n = b.load<std::uint64_t>(ifd_off);
            // Guard against absurd entry counts that would overflow.
            if (n > (std::numeric_limits<std::uint64_t>::max() - 16) / 20)
                return std::unexpected("BigTIFF entry count overflow");
            const std::size_t entries_off = ifd_off + 8;
            if (entries_off + n * 20 + 8 > b.size)
                return std::unexpected("IFD truncated (BigTIFF)");
            entries.reserve(static_cast<std::size_t>(n));
            for (std::uint64_t i = 0; i < n; ++i)
            {
                const std::size_t eo = entries_off + static_cast<std::size_t>(i) * 20;
                IfdEntry e;
                e.entry_offset = eo;
                e.inline_payload_width = 8;
                e.tag = b.load<std::uint16_t>(eo);
                e.type = b.load<std::uint16_t>(eo + 2);
                e.count = b.load<std::uint64_t>(eo + 4);
                e.value_or_offset = b.load<std::uint64_t>(eo + 12);
                entries.push_back(e);
            }
        }
        else
        {
            // Classic TIFF: uint16 num_entries, 12-byte entries, uint32 next_ifd.
            if (ifd_off + 2 > b.size)
                return std::unexpected("first IFD offset past EOF");
            const std::uint16_t n = b.load<std::uint16_t>(ifd_off);
            const std::size_t entries_off = ifd_off + 2;
            if (entries_off + static_cast<std::size_t>(n) * 12 + 4 > b.size)
                return std::unexpected("IFD truncated");
            entries.reserve(n);
            for (std::uint16_t i = 0; i < n; ++i)
            {
                const std::size_t eo = entries_off + static_cast<std::size_t>(i) * 12;
                IfdEntry e;
                e.entry_offset = eo;
                e.inline_payload_width = 4;
                e.tag = b.load<std::uint16_t>(eo);
                e.type = b.load<std::uint16_t>(eo + 2);
                e.count = static_cast<std::uint64_t>(b.load<std::uint32_t>(eo + 4));
                e.value_or_offset = static_cast<std::uint64_t>(b.load<std::uint32_t>(eo + 8));
                entries.push_back(e);
            }
        }

        GeoTiffInfo out;
        out.is_bigtiff = info->is_bigtiff;
        std::vector<double> tmp_d;

        for (const auto &e : entries)
        {
            switch (e.tag)
            {
            case 256:
                read_uint_from_entry(b, e, out.width);
                break;
            case 257:
                read_uint_from_entry(b, e, out.height);
                break;
            case 258:
                read_ushort_from_entry(b, e, out.bits_per_sample);
                break;
            case 259:
                read_ushort_from_entry(b, e, out.compression);
                break;
            case 262:
                read_ushort_from_entry(b, e, out.photometric);
                break;
            case 273:
                out.strip_count = static_cast<std::uint32_t>(e.count);
                break;
            case 277:
                read_ushort_from_entry(b, e, out.samples_per_pixel);
                break;
            case 278:
                read_uint_from_entry(b, e, out.rows_per_strip);
                break;
            case 339:
                read_ushort_from_entry(b, e, out.sample_format);
                break;
            case 33550: // ModelPixelScaleTag — 3 doubles
                if (read_doubles(b, e, tmp_d) && tmp_d.size() >= 3)
                {
                    out.have_pixel_scale = true;
                    out.pixel_scale_x = tmp_d[0];
                    out.pixel_scale_y = tmp_d[1];
                    out.pixel_scale_z = tmp_d[2];
                }
                break;
            case 33922: // ModelTiepointTag — 6*N doubles
                if (read_doubles(b, e, tmp_d) && (tmp_d.size() % 6) == 0)
                {
                    for (std::size_t k = 0; k + 6 <= tmp_d.size(); k += 6)
                    {
                        out.tiepoints.push_back(ModelTiepoint{
                            tmp_d[k + 0], tmp_d[k + 1], tmp_d[k + 2],
                            tmp_d[k + 3], tmp_d[k + 4], tmp_d[k + 5]});
                    }
                }
                break;
            case 34735: // GeoKeyDirectoryTag — array of SHORT, multiples of 4
                if (e.type == 3 && e.count >= 4 && (e.count % 4) == 0)
                {
                    const auto w = type_width(e.type);
                    const auto off = payload_offset(e, w);
                    if (off + w * static_cast<std::size_t>(e.count) > b.size)
                        return std::unexpected("GeoKeyDirectoryTag payload past EOF");
                    const auto read16 = [&](std::uint64_t idx)
                    {
                        return b.load<std::uint16_t>(off + static_cast<std::size_t>(idx) * 2);
                    };
                    // First entry = (KeyDirectoryVersion, KeyRevision,
                    //                MinorRevision, NumberOfKeys)
                    const std::uint16_t dir_version = read16(0);
                    out.geo_key_revision = read16(1);
                    out.geo_key_minor_revision = read16(2);
                    const std::uint16_t num_keys = read16(3);
                    (void)dir_version;
                    if (static_cast<std::uint64_t>(4 + num_keys * 4) <= e.count)
                    {
                        out.geo_keys.reserve(num_keys);
                        for (std::uint32_t k = 0; k < num_keys; ++k)
                        {
                            GeoKeyEntry ge;
                            ge.key_id = read16(4 + k * 4 + 0);
                            ge.location = read16(4 + k * 4 + 1);
                            ge.count = read16(4 + k * 4 + 2);
                            ge.value = read16(4 + k * 4 + 3);
                            out.geo_keys.push_back(ge);
                        }
                    }
                }
                break;
            default:
                break;
            }
        }

        return out;
    }

} // namespace xps::io_raster
