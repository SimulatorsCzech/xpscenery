#include <catch2/catch_test_macros.hpp>

#include "xpscenery/io_dsf/dsf_cmds.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace
{

    void push_u16_le(std::vector<std::byte> &o, std::uint16_t v)
    {
        o.push_back(static_cast<std::byte>(v & 0xFF));
        o.push_back(static_cast<std::byte>((v >> 8) & 0xFF));
    }
    void push_u32_le(std::vector<std::byte> &o, std::uint32_t v)
    {
        o.push_back(static_cast<std::byte>(v & 0xFF));
        o.push_back(static_cast<std::byte>((v >> 8) & 0xFF));
        o.push_back(static_cast<std::byte>((v >> 16) & 0xFF));
        o.push_back(static_cast<std::byte>((v >> 24) & 0xFF));
    }
    void push_i32_le(std::vector<std::byte> &o, std::int32_t v)
    {
        push_u32_le(o, static_cast<std::uint32_t>(v));
    }
    constexpr std::uint32_t make_id4(char a, char b, char c, char d) noexcept
    {
        return (static_cast<std::uint32_t>(a) << 24) |
               (static_cast<std::uint32_t>(b) << 16) |
               (static_cast<std::uint32_t>(c) << 8) |
               static_cast<std::uint32_t>(d);
    }

    std::vector<std::byte> wrap_cmds(std::span<const std::byte> payload)
    {
        std::vector<std::byte> atom;
        push_u32_le(atom, make_id4('C', 'M', 'D', 'S'));
        push_u32_le(atom, static_cast<std::uint32_t>(8 + payload.size()));
        atom.insert(atom.end(), payload.begin(), payload.end());
        return atom;
    }

    std::vector<std::byte> synth_dsf(std::span<const std::byte> body)
    {
        std::vector<std::byte> out;
        for (char c : std::string_view{"XPLNEDSF"})
            out.push_back(static_cast<std::byte>(c));
        push_i32_le(out, 1);
        out.insert(out.end(), body.begin(), body.end());
        for (int i = 0; i < 16; ++i)
            out.push_back(std::byte{0});
        return out;
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

} // namespace

TEST_CASE("read_cmd_stats returns zeros for file without CMDS",
          "[io_dsf][cmds]")
{
    auto dsf = synth_dsf({});
    auto p = write_tmp("dsf_no_cmds.dsf", dsf);
    auto st = xps::io_dsf::read_cmd_stats(p);
    REQUIRE(st.has_value());
    REQUIRE(st->bytes_consumed == 0);
    REQUIRE(st->objects_placed == 0);
}

TEST_CASE("read_cmd_stats counts a small varied command stream",
          "[io_dsf][cmds]")
{
    std::vector<std::byte> body;
    // PoolSelect 0
    body.push_back(std::byte{1});
    push_u16_le(body, 0);
    // SetDefinition16 = 4 → one def select
    body.push_back(std::byte{4});
    push_u16_le(body, 7);
    // Object (7) → 2 bytes index
    body.push_back(std::byte{7});
    push_u16_le(body, 42);
    // Object again
    body.push_back(std::byte{7});
    push_u16_le(body, 43);
    // Polygon (12): u16 param + u8 count + count × u16
    body.push_back(std::byte{12});
    push_u16_le(body, 9);
    body.push_back(std::byte{3});
    push_u16_le(body, 0);
    push_u16_le(body, 1);
    push_u16_le(body, 2);
    // TerrainPatch (16)
    body.push_back(std::byte{16});
    // Triangle (23) with 6 indices
    body.push_back(std::byte{23});
    body.push_back(std::byte{6});
    for (int i = 0; i < 6; ++i)
        push_u16_le(body, static_cast<std::uint16_t>(i));
    // Comment8 (32) with 4-byte payload
    body.push_back(std::byte{32});
    body.push_back(std::byte{4});
    for (int i = 0; i < 4; ++i)
        body.push_back(std::byte{0xAA});

    auto cmds = wrap_cmds(body);
    auto dsf = synth_dsf(cmds);
    auto p = write_tmp("dsf_cmds.dsf", dsf);

    auto st = xps::io_dsf::read_cmd_stats(p);
    REQUIRE(st.has_value());
    REQUIRE(st->pool_selects == 1);
    REQUIRE(st->def_selects == 1);
    REQUIRE(st->objects_placed == 2);
    REQUIRE(st->polygons == 1);
    REQUIRE(st->terrain_patches == 1);
    REQUIRE(st->triangles_emitted == 1);
    REQUIRE(st->triangle_vertices == 6);
    REQUIRE(st->opcode_counts[1] == 1);  // pool select
    REQUIRE(st->opcode_counts[7] == 2);  // object
    REQUIRE(st->opcode_counts[32] == 1); // comment8
    REQUIRE(st->bytes_consumed == body.size());
}

TEST_CASE("read_cmd_stats rejects a truncated Triangle command",
          "[io_dsf][cmds]")
{
    std::vector<std::byte> body;
    body.push_back(std::byte{23}); // Triangle
    body.push_back(std::byte{4});  // claims 4 vertices
    push_u16_le(body, 0);
    push_u16_le(body, 1);
    // missing 4 bytes
    auto dsf = synth_dsf(wrap_cmds(body));
    auto p = write_tmp("dsf_cmds_trunc.dsf", dsf);
    auto st = xps::io_dsf::read_cmd_stats(p);
    REQUIRE_FALSE(st.has_value());
}

TEST_CASE("read_cmd_stats handles TerrainPatchFlagsLOD fall-through",
          "[io_dsf][cmds]")
{
    std::vector<std::byte> body;
    // TerrainPatchFlagsLOD (18): 8 bytes LOD range + 1 flag byte
    body.push_back(std::byte{18});
    for (int i = 0; i < 9; ++i)
        body.push_back(std::byte{0});
    // TerrainPatchFlags (17): 1 flag byte
    body.push_back(std::byte{17});
    body.push_back(std::byte{0});
    // TerrainPatch (16): no args
    body.push_back(std::byte{16});
    auto dsf = synth_dsf(wrap_cmds(body));
    auto p = write_tmp("dsf_cmds_patches.dsf", dsf);
    auto st = xps::io_dsf::read_cmd_stats(p);
    REQUIRE(st.has_value());
    REQUIRE(st->terrain_patches == 3);
    REQUIRE(st->opcode_counts[18] == 1);
    REQUIRE(st->opcode_counts[17] == 1);
    REQUIRE(st->opcode_counts[16] == 1);
}
