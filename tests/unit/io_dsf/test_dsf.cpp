#include <catch2/catch_test_macros.hpp>

#include "xpscenery/io_dsf/dsf_atoms.hpp"
#include "xpscenery/io_dsf/dsf_header.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <vector>

namespace {

// Build a synthetic DSF byte stream:
//   "XPLNEDSF" + version(1) + atoms... + 16 MD5 bytes
std::vector<std::byte> synth_dsf(std::initializer_list<std::pair<std::uint32_t,
                                                                 std::uint32_t>> atoms) {
    // atoms: (raw_id, payload_size)
    std::vector<std::byte> out;
    const char cookie[] = {'X','P','L','N','E','D','S','F'};
    for (char c : cookie) out.push_back(static_cast<std::byte>(c));

    std::int32_t ver = 1;
    for (std::size_t i = 0; i < sizeof(ver); ++i) {
        out.push_back(static_cast<std::byte>((ver >> (8*i)) & 0xFF));
    }

    for (auto [id, payload] : atoms) {
        const std::uint32_t size_incl = 8 + payload;
        for (std::size_t i = 0; i < 4; ++i) {
            out.push_back(static_cast<std::byte>((id >> (8*i)) & 0xFF));
        }
        for (std::size_t i = 0; i < 4; ++i) {
            out.push_back(static_cast<std::byte>((size_incl >> (8*i)) & 0xFF));
        }
        out.insert(out.end(), payload, std::byte{0});
    }

    // MD5 footer = 16 bytes of zeros (fine, we do not verify it)
    for (int i = 0; i < 16; ++i) out.push_back(std::byte{0});
    return out;
}

std::filesystem::path write_tmp(std::string_view name,
                                std::span<const std::byte> data) {
    auto dir = std::filesystem::temp_directory_path() / "xpscenery_tests";
    std::filesystem::create_directories(dir);
    auto p = dir / std::filesystem::path{std::string{name}};
    std::filesystem::remove(p);
    auto r = xps::io_filesystem::write_binary_atomic(p, data);
    REQUIRE(r.has_value());
    return p;
}

// Multi-char literal 'HEAD' on LE = 0x48454144 (H=0x48,E=0x45,A=0x41,D=0x44).
// In file order (LE): bytes 0x44 0x41 0x45 0x48 = 'D','A','E','H'.
// Our tag_from_id reverses to yield human "HEAD" again.
constexpr std::uint32_t k_head = 'H' << 24 | 'E' << 16 | 'A' << 8 | 'D';
constexpr std::uint32_t k_defn = 'D' << 24 | 'E' << 16 | 'F' << 8 | 'N';
constexpr std::uint32_t k_geod = 'G' << 24 | 'E' << 16 | 'O' << 8 | 'D';

}  // namespace

TEST_CASE("looks_like_dsf recognises the magic", "[io_dsf][header]") {
    auto data = synth_dsf({});
    auto p = write_tmp("ok.dsf", data);
    REQUIRE(xps::io_dsf::looks_like_dsf(p));

    // Not a DSF: plain text
    const std::string junk = "not a dsf file at all";
    std::vector<std::byte> jb;
    for (char c : junk) jb.push_back(static_cast<std::byte>(c));
    auto p2 = write_tmp("junk.bin", jb);
    REQUIRE_FALSE(xps::io_dsf::looks_like_dsf(p2));
}

TEST_CASE("read_header accepts version 1 and rejects others",
          "[io_dsf][header]") {
    auto ok = synth_dsf({});
    auto p_ok = write_tmp("v1.dsf", ok);
    auto h = xps::io_dsf::read_header(p_ok);
    REQUIRE(h.has_value());
    REQUIRE(h->version == 1);
    REQUIRE(h->atoms_offset == 12);
    REQUIRE(h->atoms_end == h->file_size - 16);

    // Patch version to 2, expect error
    auto bad = ok;
    bad[8] = std::byte{2};
    auto p_bad = write_tmp("v2.dsf", bad);
    auto h2 = xps::io_dsf::read_header(p_bad);
    REQUIRE_FALSE(h2.has_value());
}

TEST_CASE("read_header rejects truncated files", "[io_dsf][header]") {
    // File with only the cookie, no version, no footer
    std::vector<std::byte> tiny;
    const char cookie[] = {'X','P','L','N','E','D','S','F'};
    for (char c : cookie) tiny.push_back(static_cast<std::byte>(c));
    auto p = write_tmp("tiny.dsf", tiny);
    auto h = xps::io_dsf::read_header(p);
    REQUIRE_FALSE(h.has_value());
}

TEST_CASE("read_top_level_atoms walks the atom tree",
          "[io_dsf][atoms]") {
    auto data = synth_dsf({{k_head, 4}, {k_defn, 12}, {k_geod, 0}});
    auto p = write_tmp("three_atoms.dsf", data);
    auto atoms = xps::io_dsf::read_top_level_atoms(p);
    REQUIRE(atoms.has_value());
    REQUIRE(atoms->size() == 3);

    REQUIRE((*atoms)[0].tag == "HEAD");
    REQUIRE((*atoms)[0].size == 12);       // 8 header + 4 payload
    REQUIRE((*atoms)[0].offset == 12);

    REQUIRE((*atoms)[1].tag == "DEFN");
    REQUIRE((*atoms)[1].size == 20);       // 8 + 12
    REQUIRE((*atoms)[1].offset == 12 + 12);

    REQUIRE((*atoms)[2].tag == "GEOD");
    REQUIRE((*atoms)[2].size == 8);        // header only
    REQUIRE((*atoms)[2].offset == 12 + 12 + 20);
}

TEST_CASE("read_top_level_atoms rejects a bogus atom size",
          "[io_dsf][atoms]") {
    // Build a valid-ish file, then corrupt the first atom's size
    auto data = synth_dsf({{k_head, 4}});
    // size field is at offset 12+4 = 16 (LE 4 bytes). Set to 3 (< header=8).
    data[16] = std::byte{3};
    data[17] = std::byte{0};
    data[18] = std::byte{0};
    data[19] = std::byte{0};
    auto p = write_tmp("bad_size.dsf", data);
    auto atoms = xps::io_dsf::read_top_level_atoms(p);
    REQUIRE_FALSE(atoms.has_value());
}
