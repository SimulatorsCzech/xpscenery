#include <catch2/catch_test_macros.hpp>

#include "xpscenery/io_dsf/dsf_raster.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

void push_u32_le(std::vector<std::byte>& o, std::uint32_t v) {
    for (int i = 0; i < 4; ++i)
        o.push_back(static_cast<std::byte>((v >> (8 * i)) & 0xFF));
}
void push_u16_le(std::vector<std::byte>& o, std::uint16_t v) {
    for (int i = 0; i < 2; ++i)
        o.push_back(static_cast<std::byte>((v >> (8 * i)) & 0xFF));
}
void push_i32_le(std::vector<std::byte>& o, std::int32_t v) {
    for (int i = 0; i < 4; ++i)
        o.push_back(static_cast<std::byte>((v >> (8 * i)) & 0xFF));
}
void push_f32_le(std::vector<std::byte>& o, float f) {
    std::uint32_t u;
    std::memcpy(&u, &f, 4);
    push_u32_le(o, u);
}

constexpr std::uint32_t make_id4(char a, char b, char c, char d) noexcept {
    return (static_cast<std::uint32_t>(a) << 24) |
           (static_cast<std::uint32_t>(b) << 16) |
           (static_cast<std::uint32_t>(c) <<  8) |
            static_cast<std::uint32_t>(d);
}

std::filesystem::path write_tmp(std::string_view name,
                                std::span<const std::byte> data)
{
    auto dir = std::filesystem::temp_directory_path() / "xpscenery_tests";
    std::filesystem::create_directories(dir);
    auto p = dir / std::filesystem::path{std::string{name}};
    std::filesystem::remove(p);
    auto r = xps::io_filesystem::write_binary_atomic(p, data);
    REQUIRE(r.has_value());
    return p;
}

}  // namespace

TEST_CASE("read_rasters parses a single DEMI header",
          "[io_dsf][raster]") {
    // Build DEMI payload: v1, bpp=4, flags=0x04 (post+float), 3601x3601
    std::vector<std::byte> demi_payload;
    demi_payload.push_back(std::byte{1});         // version
    demi_payload.push_back(std::byte{4});         // bpp
    push_u16_le(demi_payload, 0x0004);            // flags = post + float(0)
    push_u32_le(demi_payload, 3601);              // width
    push_u32_le(demi_payload, 3601);              // height
    push_f32_le(demi_payload, 1.0f);              // scale
    push_f32_le(demi_payload, 0.0f);              // offset

    // Wrap into DEMI atom.
    const auto demi_id = make_id4('D','E','M','I');
    std::vector<std::byte> demi_atom;
    push_u32_le(demi_atom, demi_id);
    push_u32_le(demi_atom,
                static_cast<std::uint32_t>(8 + demi_payload.size()));
    demi_atom.insert(demi_atom.end(),
                     demi_payload.begin(), demi_payload.end());

    // Wrap into DEMS atom-of-atoms.
    const auto dems_id = make_id4('D','E','M','S');
    std::vector<std::byte> dems_atom;
    push_u32_le(dems_atom, dems_id);
    push_u32_le(dems_atom,
                static_cast<std::uint32_t>(8 + demi_atom.size()));
    dems_atom.insert(dems_atom.end(), demi_atom.begin(), demi_atom.end());

    // Full DSF file: cookie + version + DEMS + MD5 footer.
    std::vector<std::byte> dsf;
    for (char c : std::string_view{"XPLNEDSF"}) dsf.push_back(static_cast<std::byte>(c));
    push_i32_le(dsf, 1);
    dsf.insert(dsf.end(), dems_atom.begin(), dems_atom.end());
    for (int i = 0; i < 16; ++i) dsf.push_back(std::byte{0});

    auto p = write_tmp("dsf_raster.dsf", dsf);
    auto rasters = xps::io_dsf::read_rasters(p);
    REQUIRE(rasters.has_value());
    REQUIRE(rasters->size() == 1);

    const auto& r = (*rasters)[0];
    REQUIRE(r.version == 1);
    REQUIRE(r.bytes_per_pixel == 4);
    REQUIRE(r.width  == 3601);
    REQUIRE(r.height == 3601);
    REQUIRE(r.is_post());
    REQUIRE(r.format() == xps::io_dsf::RasterFormat::floating);
    REQUIRE(r.scale == 1.0f);
    REQUIRE(r.offset == 0.0f);
}

TEST_CASE("read_rasters returns empty when DEMS is absent",
          "[io_dsf][raster]") {
    std::vector<std::byte> dsf;
    for (char c : std::string_view{"XPLNEDSF"}) dsf.push_back(static_cast<std::byte>(c));
    push_i32_le(dsf, 1);
    for (int i = 0; i < 16; ++i) dsf.push_back(std::byte{0});

    auto p = write_tmp("dsf_no_dems.dsf", dsf);
    auto rasters = xps::io_dsf::read_rasters(p);
    REQUIRE(rasters.has_value());
    REQUIRE(rasters->empty());
}
