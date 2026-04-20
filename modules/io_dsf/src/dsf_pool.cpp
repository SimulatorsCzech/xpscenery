#include "xpscenery/io_dsf/dsf_pool.hpp"
#include "xpscenery/io_dsf/dsf_atoms.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <bit>
#include <cstring>
#include <format>
#include <limits>

namespace xps::io_dsf {

namespace {

constexpr std::uint32_t make_id4(char a, char b, char c, char d) noexcept {
    return (static_cast<std::uint32_t>(a) << 24) |
           (static_cast<std::uint32_t>(b) << 16) |
           (static_cast<std::uint32_t>(c) <<  8) |
            static_cast<std::uint32_t>(d);
}
constexpr std::uint32_t kGeodId = make_id4('G','E','O','D');
constexpr std::uint32_t kPoolId = make_id4('P','O','O','L');
constexpr std::uint32_t kScalId = make_id4('S','C','A','L');
constexpr std::uint32_t kPo32Id = make_id4('P','O','3','2');
constexpr std::uint32_t kSc32Id = make_id4('S','C','3','2');

template <typename T>
T load_le(const std::byte* p) noexcept {
    T v{};
    std::memcpy(&v, p, sizeof(T));
    if constexpr (std::endian::native == std::endian::big) {
        if constexpr (sizeof(T) == 4) {
            std::uint32_t u; std::memcpy(&u, &v, 4);
            u = std::byteswap(u);
            std::memcpy(&v, &u, 4);
        } else if constexpr (sizeof(T) == 2) {
            std::uint16_t u; std::memcpy(&u, &v, 2);
            u = std::byteswap(u);
            std::memcpy(&v, &u, 2);
        }
    }
    return v;
}

/// Planar numeric stream decoder for uint16 / uint32. Output buffer
/// is interleaved: `out[i*planes + p]` for record i, plane p.
template <typename T>
bool decode_planes(const std::byte* cursor,
                   const std::byte* end,
                   std::uint32_t plane_count,
                   std::uint32_t plane_size,
                   std::vector<T>& out) noexcept
{
    out.assign(static_cast<std::size_t>(plane_count) * plane_size, T{});
    for (std::uint32_t plane = 0; plane < plane_count; ++plane) {
        if (cursor >= end) return false;
        const std::uint8_t mode = static_cast<std::uint8_t>(*cursor++);
        if (mode > 3) return false;

        const bool differenced = (mode & 1) != 0;
        const bool rle         = (mode & 2) != 0;

        T last = 0;
        std::uint32_t i = 0;
        if (!rle) {
            // Flat stream: plane_size values back-to-back.
            if (static_cast<std::size_t>(end - cursor) <
                static_cast<std::size_t>(plane_size) * sizeof(T))
                return false;
            for (; i < plane_size; ++i) {
                T v = load_le<T>(cursor);
                cursor += sizeof(T);
                if (differenced) { v = static_cast<T>(last + v); last = v; }
                out[i * plane_count + plane] = v;
            }
        } else {
            // RLE: code byte; high bit = run, low 7 bits = length.
            while (i < plane_size) {
                if (cursor >= end) return false;
                const std::uint8_t code = static_cast<std::uint8_t>(*cursor++);
                const std::uint32_t len = code & 0x7F;
                if (len == 0) return false;
                if (code & 0x80) {
                    // run: one value, repeat `len` times
                    if (static_cast<std::size_t>(end - cursor) < sizeof(T))
                        return false;
                    T v = load_le<T>(cursor);
                    cursor += sizeof(T);
                    for (std::uint32_t k = 0; k < len && i < plane_size; ++k, ++i) {
                        T out_v = v;
                        if (differenced) { out_v = static_cast<T>(last + out_v); last = out_v; }
                        out[i * plane_count + plane] = out_v;
                    }
                } else {
                    // individual: `len` distinct values
                    if (static_cast<std::size_t>(end - cursor) <
                        static_cast<std::size_t>(len) * sizeof(T))
                        return false;
                    for (std::uint32_t k = 0; k < len && i < plane_size; ++k, ++i) {
                        T v = load_le<T>(cursor);
                        cursor += sizeof(T);
                        if (differenced) { v = static_cast<T>(last + v); last = v; }
                        out[i * plane_count + plane] = v;
                    }
                }
            }
        }
    }
    return true;
}

struct ScaleVec { std::vector<double> scale, offset; };

[[nodiscard]] std::expected<ScaleVec, std::string>
decode_scal_payload(std::span<const std::byte> payload) {
    if ((payload.size() % 8) != 0) {
        return std::unexpected(std::format(
            "SCAL payload length {} is not a multiple of 8 (float32 pair)",
            payload.size()));
    }
    const std::size_t count = payload.size() / 8;
    ScaleVec out;
    out.scale.reserve(count);
    out.offset.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto* p = payload.data() + i * 8;
        out.scale.push_back(load_le<float>(p));
        out.offset.push_back(load_le<float>(p + 4));
    }
    return out;
}

}  // namespace

std::expected<std::vector<PointPool>, std::string>
read_point_pools(const std::filesystem::path& dsf_path)
{
    auto atoms = read_top_level_atoms(dsf_path);
    if (!atoms) return std::unexpected(std::move(atoms.error()));

    const TopLevelAtom* geod = nullptr;
    for (const auto& a : *atoms) if (a.raw_id == kGeodId) { geod = &a; break; }
    if (!geod) return std::vector<PointPool>{};

    auto bytes = xps::io_filesystem::read_binary(dsf_path);
    if (!bytes) return std::unexpected(std::move(bytes.error()));
    const std::byte* file = bytes->data();

    // Walk GEOD's payload for direct child atoms (POOL/SCAL/PO32/SC32).
    std::vector<TopLevelAtom> kids;
    {
        const std::uint64_t child_begin = geod->offset + 8;
        const std::uint64_t child_end   = geod->offset + geod->size;
        std::uint64_t c = child_begin;
        while (c + 8 <= child_end) {
            const std::byte* h = file + c;
            const std::uint32_t id = (static_cast<std::uint32_t>(std::to_integer<unsigned>(h[3])) << 24)
                                   | (static_cast<std::uint32_t>(std::to_integer<unsigned>(h[2])) << 16)
                                   | (static_cast<std::uint32_t>(std::to_integer<unsigned>(h[1])) <<  8)
                                   |  static_cast<std::uint32_t>(std::to_integer<unsigned>(h[0]));
            std::uint32_t sz;
            std::memcpy(&sz, file + c + 4, 4);
            if constexpr (std::endian::native == std::endian::big) sz = std::byteswap(sz);
            if (sz < 8 || c + sz > child_end) {
                return std::unexpected(std::format(
                    "GEOD child atom at offset {} has invalid size {}", c, sz));
            }
            TopLevelAtom a;
            a.raw_id = id;
            a.offset = c;
            a.size   = sz;
            kids.push_back(a);
            c += sz;
        }
    }

    // Collect by index so POOL[k] pairs with SCAL[k], and PO32[k] with SC32[k].
    std::vector<const TopLevelAtom*> pools16, scales16, pools32, scales32;
    for (const auto& c : kids) {
        if      (c.raw_id == kPoolId) pools16.push_back(&c);
        else if (c.raw_id == kScalId) scales16.push_back(&c);
        else if (c.raw_id == kPo32Id) pools32.push_back(&c);
        else if (c.raw_id == kSc32Id) scales32.push_back(&c);
    }

    std::vector<PointPool> result;
    result.reserve(pools16.size() + pools32.size());

    auto payload_of = [&](const TopLevelAtom& a, std::size_t skip)
        -> std::span<const std::byte>
    {
        const std::size_t start = a.offset + 8 + skip;
        return {file + start, a.size - 8 - skip};
    };

    auto decode_one = [&](const TopLevelAtom& pool_atom,
                          const TopLevelAtom& scal_atom,
                          bool is_32) -> std::expected<PointPool, std::string>
    {
        // POOL payload = int32 array_size + uint8 plane_count + streams
        if (pool_atom.size < 8 + 5) {
            return std::unexpected("POOL atom too small for header");
        }
        const std::byte* hdr = file + pool_atom.offset + 8;
        const std::uint32_t array_size  = load_le<std::uint32_t>(hdr);
        const std::uint8_t  plane_count = static_cast<std::uint8_t>(hdr[4]);
        if (plane_count == 0) {
            return std::unexpected("POOL has plane_count=0");
        }

        auto scal_payload = payload_of(scal_atom, 0);
        auto scalers = decode_scal_payload(scal_payload);
        if (!scalers) return std::unexpected(std::move(scalers.error()));
        if (scalers->scale.size() < plane_count) {
            return std::unexpected(std::format(
                "SCAL has {} planes but POOL declares {}",
                scalers->scale.size(), plane_count));
        }

        const std::byte* body     = hdr + 5;
        const std::byte* body_end = file + pool_atom.offset + pool_atom.size;

        PointPool pp;
        pp.plane_count = plane_count;
        pp.array_size  = array_size;
        pp.is_32bit    = is_32;
        pp.scales.assign(scalers->scale.begin(),
                         scalers->scale.begin() + plane_count);
        pp.offsets.assign(scalers->offset.begin(),
                          scalers->offset.begin() + plane_count);
        pp.points.resize(static_cast<std::size_t>(plane_count) * array_size);

        if (!is_32) {
            std::vector<std::uint16_t> raw;
            if (!decode_planes<std::uint16_t>(body, body_end,
                                              plane_count, array_size, raw)) {
                return std::unexpected(
                    "POOL planar 16-bit stream is truncated or malformed");
            }
            constexpr double recip = 1.0 / 65535.0;
            for (std::uint32_t i = 0; i < array_size; ++i) {
                for (std::uint32_t p = 0; p < plane_count; ++p) {
                    const double r = static_cast<double>(raw[i * plane_count + p]);
                    pp.points[i * plane_count + p] =
                        r * pp.scales[p] * recip + pp.offsets[p];
                }
            }
        } else {
            std::vector<std::uint32_t> raw;
            if (!decode_planes<std::uint32_t>(body, body_end,
                                              plane_count, array_size, raw)) {
                return std::unexpected(
                    "PO32 planar 32-bit stream is truncated or malformed");
            }
            constexpr double recip = 1.0 / 4294967295.0;
            for (std::uint32_t i = 0; i < array_size; ++i) {
                for (std::uint32_t p = 0; p < plane_count; ++p) {
                    const double r = static_cast<double>(raw[i * plane_count + p]);
                    pp.points[i * plane_count + p] =
                        r * pp.scales[p] * recip + pp.offsets[p];
                }
            }
        }
        return pp;
    };

    if (scales16.size() < pools16.size() || scales32.size() < pools32.size()) {
        return std::unexpected(std::format(
            "POOL/SCAL mismatch: {} POOL + {} SCAL, {} PO32 + {} SC32",
            pools16.size(), scales16.size(),
            pools32.size(), scales32.size()));
    }

    for (std::size_t i = 0; i < pools16.size(); ++i) {
        auto pp = decode_one(*pools16[i], *scales16[i], false);
        if (!pp) return std::unexpected(std::move(pp.error()));
        result.push_back(std::move(*pp));
    }
    for (std::size_t i = 0; i < pools32.size(); ++i) {
        auto pp = decode_one(*pools32[i], *scales32[i], true);
        if (!pp) return std::unexpected(std::move(pp.error()));
        result.push_back(std::move(*pp));
    }
    return result;
}

}  // namespace xps::io_dsf
