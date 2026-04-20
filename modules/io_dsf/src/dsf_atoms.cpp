#include "xpscenery/io_dsf/dsf_atoms.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <bit>
#include <cstring>
#include <format>

namespace xps::io_dsf {

namespace {

std::uint32_t le_u32(const std::byte* p) noexcept {
    std::uint32_t v = 0;
    std::memcpy(&v, p, sizeof(v));
    if constexpr (std::endian::native == std::endian::big) {
        v = std::byteswap(v);
    }
    return v;
}

/// Decode a 4-byte atom id back to a human-readable tag. X-Plane
/// source writes ids as multi-char literals like 'HEAD' which on
/// little-endian hosts serialises to bytes 'D','A','E','H'. So we
/// reverse the four payload bytes to get "HEAD" for display.
std::string tag_from_id(std::uint32_t raw) noexcept {
    char t[4];
    t[0] = static_cast<char>((raw >> 24) & 0xFF);
    t[1] = static_cast<char>((raw >> 16) & 0xFF);
    t[2] = static_cast<char>((raw >>  8) & 0xFF);
    t[3] = static_cast<char>( raw        & 0xFF);
    // Replace non-printable with '?'
    for (auto& c : t) {
        const auto u = static_cast<unsigned char>(c);
        if (u < 0x20 || u > 0x7E) c = '?';
    }
    return std::string{t, 4};
}

}  // namespace

std::expected<std::vector<TopLevelAtom>, std::string>
read_top_level_atoms(const std::filesystem::path& dsf_path) {
    auto hdr = read_header(dsf_path);
    if (!hdr) return std::unexpected(std::move(hdr.error()));

    auto bytes = xps::io_filesystem::read_binary(dsf_path);
    if (!bytes) return std::unexpected(std::move(bytes.error()));

    std::vector<TopLevelAtom> atoms;
    std::uint64_t pos = hdr->atoms_offset;
    const std::uint64_t end = hdr->atoms_end;

    while (pos + 8 <= end) {
        const auto id   = le_u32(bytes->data() + pos);
        const auto size = le_u32(bytes->data() + pos + 4);
        if (size < 8) {
            return std::unexpected(std::format(
                "DSF atom at offset {} has size {} < 8 (header) in {}",
                pos, size, dsf_path.string()));
        }
        if (pos + size > end) {
            return std::unexpected(std::format(
                "DSF atom '{}' at offset {} extends beyond atom region "
                "(size {}, end {}) in {}",
                tag_from_id(id), pos, size, end, dsf_path.string()));
        }
        atoms.push_back(TopLevelAtom{
            .tag    = tag_from_id(id),
            .raw_id = id,
            .offset = pos,
            .size   = size,
        });
        pos += size;
    }
    if (pos != end) {
        return std::unexpected(std::format(
            "DSF atom region mis-aligned: pos {} != end {} in {}",
            pos, end, dsf_path.string()));
    }
    return atoms;
}

}  // namespace xps::io_dsf
