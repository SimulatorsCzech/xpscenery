#include "xpscenery/io_dsf/dsf_raster.hpp"
#include "xpscenery/io_dsf/dsf_strings.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <bit>
#include <cstring>
#include <format>

namespace xps::io_dsf {

namespace {

constexpr std::uint32_t make_id4(char a, char b, char c, char d) noexcept {
    return (static_cast<std::uint32_t>(a) << 24) |
           (static_cast<std::uint32_t>(b) << 16) |
           (static_cast<std::uint32_t>(c) <<  8) |
            static_cast<std::uint32_t>(d);
}
constexpr std::uint32_t kDemsId = make_id4('D','E','M','S');
constexpr std::uint32_t kDemiId = make_id4('D','E','M','I');

template <typename T>
T read_le(const std::byte* p) noexcept {
    T v{};
    std::memcpy(&v, p, sizeof(T));
    if constexpr (std::endian::native == std::endian::big) {
        if constexpr (sizeof(T) == 4) {
            std::uint32_t u;
            std::memcpy(&u, &v, 4);
            u = std::byteswap(u);
            std::memcpy(&v, &u, 4);
        } else if constexpr (sizeof(T) == 2) {
            std::uint16_t u;
            std::memcpy(&u, &v, 2);
            u = std::byteswap(u);
            std::memcpy(&v, &u, 2);
        }
    }
    return v;
}

}  // namespace

std::expected<std::vector<RasterHeader>, std::string>
read_rasters(const std::filesystem::path& dsf_path)
{
    auto atoms = read_top_level_atoms(dsf_path);
    if (!atoms) return std::unexpected(std::move(atoms.error()));

    const TopLevelAtom* dems = nullptr;
    for (const auto& a : *atoms) {
        if (a.raw_id == kDemsId) { dems = &a; break; }
    }
    if (dems == nullptr) {
        return std::vector<RasterHeader>{};
    }

    auto children = read_child_atoms(dsf_path, *dems);
    if (!children) return std::unexpected(std::move(children.error()));

    auto bytes = xps::io_filesystem::read_binary(dsf_path);
    if (!bytes) return std::unexpected(std::move(bytes.error()));

    // Optional: raster names from DEFN → DEMN.
    auto layer_names = read_defn_strings(dsf_path, DefnKind::raster_names);
    std::vector<std::string> names;
    if (layer_names) names = std::move(*layer_names);

    std::vector<RasterHeader> out;
    std::size_t demi_index = 0;
    for (const auto& c : *children) {
        if (c.raw_id != kDemiId) continue;

        constexpr std::uint32_t kDemiPayloadSize = 20;  // v1 layout
        if (c.size < 8 + kDemiPayloadSize) {
            return std::unexpected(std::format(
                "DSF DEMI atom at offset {} has size {} < 28",
                c.offset, c.size));
        }
        const auto* p = bytes->data() + c.offset + 8;
        RasterHeader h;
        h.version         = static_cast<std::uint8_t>(p[0]);
        h.bytes_per_pixel = static_cast<std::uint8_t>(p[1]);
        h.flags           = read_le<std::uint16_t>(p + 2);
        h.width           = read_le<std::uint32_t>(p + 4);
        h.height          = read_le<std::uint32_t>(p + 8);
        h.scale           = read_le<float>(p + 12);
        h.offset          = read_le<float>(p + 16);
        if (demi_index < names.size()) {
            h.name = names[demi_index];
        }
        out.push_back(std::move(h));
        ++demi_index;
    }
    return out;
}

}  // namespace xps::io_dsf
