// Decomposing DSF writer tests — v0.3.0 atom-level round-trip.
#include <catch2/catch_test_macros.hpp>

#include "xpscenery/io_dsf/dsf_atoms.hpp"
#include "xpscenery/io_dsf/dsf_md5.hpp"
#include "xpscenery/io_dsf/dsf_writer.hpp"
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

    void push_u32_le(std::vector<std::byte> &o, std::uint32_t v)
    {
        o.push_back(static_cast<std::byte>(v & 0xFF));
        o.push_back(static_cast<std::byte>((v >> 8) & 0xFF));
        o.push_back(static_cast<std::byte>((v >> 16) & 0xFF));
        o.push_back(static_cast<std::byte>((v >> 24) & 0xFF));
    }

    // Atom IDs in DSF files are stored as little-endian bytes; X-Plane
    // writes them as 4-char literals like 'HEAD' which on LE hosts
    // serialises to bytes 'D','A','E','H'. This helper takes the
    // human-readable tag and produces the file-order byte sequence.
    void push_atom_header(std::vector<std::byte> &o,
                          std::string_view tag,
                          std::uint32_t size_incl_header)
    {
        REQUIRE(tag.size() == 4);
        o.push_back(static_cast<std::byte>(tag[3]));
        o.push_back(static_cast<std::byte>(tag[2]));
        o.push_back(static_cast<std::byte>(tag[1]));
        o.push_back(static_cast<std::byte>(tag[0]));
        push_u32_le(o, size_incl_header);
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

    // Build a full DSF with an arbitrary body (everything between the
    // 12-byte header and the 16-byte MD5 footer) and a correct footer.
    std::vector<std::byte> build_dsf(std::span<const std::byte> body)
    {
        std::vector<std::byte> d;
        for (char c : std::string_view{"XPLNEDSF"})
            d.push_back(static_cast<std::byte>(c));
        push_u32_le(d, 1u); // version
        d.insert(d.end(), body.begin(), body.end());
        auto digest = xps::io_dsf::Md5::of(
            std::span<const std::byte>{d.data(), d.size()});
        for (auto b : digest)
            d.push_back(static_cast<std::byte>(b));
        return d;
    }

    // Build a body with three named atoms of given payload sizes.
    std::vector<std::byte> build_three_atom_body()
    {
        std::vector<std::byte> body;
        // HEAD atom: 8 header + 4 payload = 12 bytes
        push_atom_header(body, "HEAD", 12);
        for (int i = 0; i < 4; ++i)
            body.push_back(static_cast<std::byte>(0xA0 + i));
        // DEFN atom: 8 header + 0 payload = 8 bytes
        push_atom_header(body, "DEFN", 8);
        // GEOD atom: 8 header + 16 payload = 24 bytes
        push_atom_header(body, "GEOD", 24);
        for (int i = 0; i < 16; ++i)
            body.push_back(static_cast<std::byte>(i * 7 + 1));
        return body;
    }

} // namespace

TEST_CASE("read_dsf_blob decomposes the file into tagged atom blobs",
          "[io_dsf][writer][decompose]")
{
    auto body = build_three_atom_body();
    auto dsf = build_dsf(body);
    auto src = write_tmp("dsf_decompose_read.dsf", dsf);

    auto blob = xps::io_dsf::read_dsf_blob(src);
    REQUIRE(blob.has_value());
    REQUIRE(blob->version == 1);
    REQUIRE(blob->atoms.size() == 3);
    REQUIRE(blob->atoms[0].tag == "HEAD");
    REQUIRE(blob->atoms[0].bytes.size() == 12);
    REQUIRE(blob->atoms[1].tag == "DEFN");
    REQUIRE(blob->atoms[1].bytes.size() == 8);
    REQUIRE(blob->atoms[2].tag == "GEOD");
    REQUIRE(blob->atoms[2].bytes.size() == 24);
}

TEST_CASE("rewrite_dsf_decomposed is bit-identical on a well-formed DSF",
          "[io_dsf][writer][decompose]")
{
    auto body = build_three_atom_body();
    auto dsf = build_dsf(body);
    auto src = write_tmp("dsf_decompose_roundtrip_src.dsf", dsf);
    auto dst = std::filesystem::temp_directory_path() /
               "xpscenery_tests" / "dsf_decompose_roundtrip_dst.dsf";
    std::filesystem::remove(dst);

    auto rep = xps::io_dsf::rewrite_dsf_decomposed(src, dst);
    REQUIRE(rep.has_value());
    REQUIRE(rep->md5_unchanged);
    REQUIRE(rep->source_size == rep->output_size);

    auto out = xps::io_filesystem::read_binary(dst);
    REQUIRE(out.has_value());
    REQUIRE(out->size() == dsf.size());
    REQUIRE(std::memcmp(out->data(), dsf.data(), dsf.size()) == 0);
}

TEST_CASE("write_dsf_blob emits a valid DSF after atom mutation",
          "[io_dsf][writer][decompose]")
{
    auto body = build_three_atom_body();
    auto dsf = build_dsf(body);
    auto src = write_tmp("dsf_decompose_mutate_src.dsf", dsf);

    auto blob = xps::io_dsf::read_dsf_blob(src);
    REQUIRE(blob.has_value());

    // Drop the middle (empty DEFN) atom to exercise a real structural
    // change. We now have two atoms, total body smaller.
    blob->atoms.erase(blob->atoms.begin() + 1);

    auto dst = std::filesystem::temp_directory_path() /
               "xpscenery_tests" / "dsf_decompose_mutate_dst.dsf";
    std::filesystem::remove(dst);

    auto rep = xps::io_dsf::write_dsf_blob(dst, *blob);
    REQUIRE(rep.has_value());

    // Re-open and verify the top-level atom walk + MD5 both succeed.
    auto atoms = xps::io_dsf::read_top_level_atoms(dst);
    REQUIRE(atoms.has_value());
    REQUIRE(atoms->size() == 2);
    REQUIRE((*atoms)[0].tag == "HEAD");
    REQUIRE((*atoms)[1].tag == "GEOD");

    auto v = xps::io_dsf::verify_md5_footer(dst);
    REQUIRE(v.has_value());
    REQUIRE(v->ok);
}

TEST_CASE("write_dsf_blob rejects an atom whose header size disagrees with its blob",
          "[io_dsf][writer][decompose]")
{
    xps::io_dsf::DsfBlob blob;
    blob.version = 1;
    xps::io_dsf::AtomBlob a;
    a.tag = "HEAD";
    // Declare size 99 in the header but provide only 12 bytes.
    a.bytes.resize(12);
    a.bytes[0] = static_cast<std::byte>('D');
    a.bytes[1] = static_cast<std::byte>('A');
    a.bytes[2] = static_cast<std::byte>('E');
    a.bytes[3] = static_cast<std::byte>('H');
    a.bytes[4] = static_cast<std::byte>(99); // size LSB
    a.bytes[5] = std::byte{0};
    a.bytes[6] = std::byte{0};
    a.bytes[7] = std::byte{0};
    blob.atoms.push_back(std::move(a));

    auto dst = std::filesystem::temp_directory_path() /
               "xpscenery_tests" / "dsf_decompose_bad.dsf";
    std::filesystem::remove(dst);
    auto rep = xps::io_dsf::write_dsf_blob(dst, blob);
    REQUIRE_FALSE(rep.has_value());
}
