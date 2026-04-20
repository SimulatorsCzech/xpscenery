#include "xpscenery/io_raster/tiff_detect.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>

namespace xps::io_raster {

namespace {

constexpr std::size_t kProbeBytes = 16;

std::uint16_t read_u16(const std::uint8_t* p, bool little) noexcept {
    std::uint16_t raw;
    std::memcpy(&raw, p, sizeof(raw));
    if constexpr (std::endian::native == std::endian::little) {
        return little ? raw : std::byteswap(raw);
    } else {
        return little ? std::byteswap(raw) : raw;
    }
}

std::uint32_t read_u32(const std::uint8_t* p, bool little) noexcept {
    std::uint32_t raw;
    std::memcpy(&raw, p, sizeof(raw));
    if constexpr (std::endian::native == std::endian::little) {
        return little ? raw : std::byteswap(raw);
    } else {
        return little ? std::byteswap(raw) : raw;
    }
}

std::uint64_t read_u64(const std::uint8_t* p, bool little) noexcept {
    std::uint64_t raw;
    std::memcpy(&raw, p, sizeof(raw));
    if constexpr (std::endian::native == std::endian::little) {
        return little ? raw : std::byteswap(raw);
    } else {
        return little ? std::byteswap(raw) : raw;
    }
}

}  // namespace

TiffInfo detect_tiff_bytes(const std::uint8_t* data, std::size_t size) noexcept {
    TiffInfo info;
    if (data == nullptr || size < 8) {
        return info;
    }

    // Byte-order marker: "II" = little-endian, "MM" = big-endian.
    const bool little = (data[0] == 'I' && data[1] == 'I');
    const bool big    = (data[0] == 'M' && data[1] == 'M');
    if (!little && !big) {
        return info;
    }
    info.little_endian = little;

    const std::uint16_t version = read_u16(data + 2, little);
    if (version == 42u) {
        info.kind = little ? TiffKind::tiff_le : TiffKind::tiff_be;
        info.first_ifd_offset = read_u32(data + 4, little);
        return info;
    }
    if (version == 43u) {
        info.kind = little ? TiffKind::bigtiff_le : TiffKind::bigtiff_be;
        info.is_bigtiff = true;
        if (size < 16) {
            return info;  // signature valid, offset unavailable
        }
        // BigTIFF header: bytesize (u16) + constant (u16) + first IFD (u64).
        const std::uint16_t bytesize = read_u16(data + 4, little);
        const std::uint16_t constant = read_u16(data + 6, little);
        if (bytesize != 8u || constant != 0u) {
            // Malformed BigTIFF signature — still report as BigTIFF, but no offset.
            return info;
        }
        info.first_ifd_offset = read_u64(data + 8, little);
        return info;
    }

    return info;  // Known byte order, unknown version — not a TIFF we accept.
}

std::expected<TiffInfo, std::string>
detect_tiff(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return std::unexpected(std::format("file not found: {}", path.string()));
    }

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::unexpected(std::format("cannot open: {}", path.string()));
    }

    std::array<std::uint8_t, kProbeBytes> probe{};
    in.read(reinterpret_cast<char*>(probe.data()),
            static_cast<std::streamsize>(probe.size()));
    const auto got = static_cast<std::size_t>(in.gcount());

    return detect_tiff_bytes(probe.data(), got);
}

}  // namespace xps::io_raster
