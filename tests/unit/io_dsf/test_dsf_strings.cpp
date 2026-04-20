#include <catch2/catch_test_macros.hpp>

#include "xpscenery/io_dsf/dsf_atoms.hpp"
#include "xpscenery/io_dsf/dsf_header.hpp"
#include "xpscenery/io_dsf/dsf_strings.hpp"
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

void push_u32_le(std::vector<std::byte>& out, std::uint32_t v) {
    for (std::size_t i = 0; i < 4; ++i)
        out.push_back(static_cast<std::byte>((v >> (8 * i)) & 0xFF));
}
void push_i32_le(std::vector<std::byte>& out, std::int32_t v) {
    for (std::size_t i = 0; i < 4; ++i)
        out.push_back(static_cast<std::byte>((v >> (8 * i)) & 0xFF));
}
void push_str(std::vector<std::byte>& out, std::string_view s) {
    for (char c : s) out.push_back(static_cast<std::byte>(c));
    out.push_back(std::byte{0});
}

// Atom id ints are multi-char literals. On little-endian the file bytes
// are id-byte-0..3 = LSB..MSB of the int. Encode the integer here.
constexpr std::uint32_t k_head = ('H' << 24) | ('E' << 16) | ('A' << 8) | 'D';
constexpr std::uint32_t k_prop = ('P' << 24) | ('R' << 16) | ('O' << 8) | 'P';
constexpr std::uint32_t k_tert = ('T' << 24) | ('E' << 16) | ('R' << 8) | 'T';
constexpr std::uint32_t k_defn = ('D' << 24) | ('E' << 16) | ('F' << 8) | 'N';

/// Build a minimal DSF file containing:
///   HEAD { PROP { "sim/west\0-180\0sim/east\0180\0" } }
///   DEFN { TERT { "terrain/foo.ter\0" } }
std::vector<std::byte> synth_dsf_with_props() {
    std::vector<std::byte> prop_payload;
    push_str(prop_payload, "sim/west");
    push_str(prop_payload, "-180");
    push_str(prop_payload, "sim/east");
    push_str(prop_payload, "180");

    std::vector<std::byte> prop_atom;
    push_u32_le(prop_atom, k_prop);
    push_u32_le(prop_atom, static_cast<std::uint32_t>(8 + prop_payload.size()));
    prop_atom.insert(prop_atom.end(), prop_payload.begin(), prop_payload.end());

    std::vector<std::byte> head_atom;
    push_u32_le(head_atom, k_head);
    push_u32_le(head_atom, static_cast<std::uint32_t>(8 + prop_atom.size()));
    head_atom.insert(head_atom.end(), prop_atom.begin(), prop_atom.end());

    std::vector<std::byte> tert_payload;
    push_str(tert_payload, "terrain/foo.ter");
    std::vector<std::byte> tert_atom;
    push_u32_le(tert_atom, k_tert);
    push_u32_le(tert_atom, static_cast<std::uint32_t>(8 + tert_payload.size()));
    tert_atom.insert(tert_atom.end(), tert_payload.begin(), tert_payload.end());

    std::vector<std::byte> defn_atom;
    push_u32_le(defn_atom, k_defn);
    push_u32_le(defn_atom, static_cast<std::uint32_t>(8 + tert_atom.size()));
    defn_atom.insert(defn_atom.end(), tert_atom.begin(), tert_atom.end());

    std::vector<std::byte> out;
    for (char c : std::string_view{"XPLNEDSF"}) out.push_back(static_cast<std::byte>(c));
    push_i32_le(out, 1);  // version
    out.insert(out.end(), head_atom.begin(), head_atom.end());
    out.insert(out.end(), defn_atom.begin(), defn_atom.end());
    for (int i = 0; i < 16; ++i) out.push_back(std::byte{0});  // MD5 footer
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

}  // namespace

TEST_CASE("read_properties decodes HEAD/PROP key-value pairs",
          "[io_dsf][strings]") {
    auto data = synth_dsf_with_props();
    auto p = write_tmp("dsf_props.dsf", data);

    auto props = xps::io_dsf::read_properties(p);
    REQUIRE(props.has_value());
    REQUIRE(props->size() == 2);
    REQUIRE((*props)[0].first  == "sim/west");
    REQUIRE((*props)[0].second == "-180");
    REQUIRE((*props)[1].first  == "sim/east");
    REQUIRE((*props)[1].second == "180");
}

TEST_CASE("read_child_atoms descends into DEFN",
          "[io_dsf][strings]") {
    auto data = synth_dsf_with_props();
    auto p = write_tmp("dsf_defn.dsf", data);

    auto atoms = xps::io_dsf::read_top_level_atoms(p);
    REQUIRE(atoms.has_value());

    const xps::io_dsf::TopLevelAtom* defn = nullptr;
    for (const auto& a : *atoms) {
        if (a.tag == "DEFN") { defn = &a; break; }
    }
    REQUIRE(defn != nullptr);

    auto children = xps::io_dsf::read_child_atoms(p, *defn);
    REQUIRE(children.has_value());
    REQUIRE(children->size() == 1);
    REQUIRE((*children)[0].tag == "TERT");

    auto strings = xps::io_dsf::read_string_table(p, (*children)[0]);
    REQUIRE(strings.has_value());
    REQUIRE(strings->size() == 1);
    REQUIRE((*strings)[0] == "terrain/foo.ter");
}

TEST_CASE("read_properties returns empty list when HEAD is missing",
          "[io_dsf][strings]") {
    // Synthesize a DSF with only a DEFN atom — no HEAD.
    std::vector<std::byte> defn_atom;
    push_u32_le(defn_atom, k_defn);
    push_u32_le(defn_atom, 8u);

    std::vector<std::byte> out;
    for (char c : std::string_view{"XPLNEDSF"}) out.push_back(static_cast<std::byte>(c));
    push_i32_le(out, 1);
    out.insert(out.end(), defn_atom.begin(), defn_atom.end());
    for (int i = 0; i < 16; ++i) out.push_back(std::byte{0});

    auto p = write_tmp("no_head.dsf", out);
    auto props = xps::io_dsf::read_properties(p);
    REQUIRE(props.has_value());
    REQUIRE(props->empty());
}

TEST_CASE("read_defn_strings pulls terrain types", "[io_dsf][strings][defn]") {
    auto data = synth_dsf_with_props();
    auto p = write_tmp("dsf_defn_tert.dsf", data);

    auto tert = xps::io_dsf::read_defn_strings(
        p, xps::io_dsf::DefnKind::terrain_types);
    REQUIRE(tert.has_value());
    REQUIRE(tert->size() == 1);
    REQUIRE((*tert)[0] == "terrain/foo.ter");

    auto objt = xps::io_dsf::read_defn_strings(
        p, xps::io_dsf::DefnKind::object_defs);
    REQUIRE(objt.has_value());
    REQUIRE(objt->empty());  // synth file has no OBJT child
}
