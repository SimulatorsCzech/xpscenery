#include <catch2/catch_test_macros.hpp>

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
    void push_i32_le(std::vector<std::byte> &o, std::int32_t v)
    {
        push_u32_le(o, static_cast<std::uint32_t>(v));
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

    // Build a DSF with an arbitrary body and a CORRECT MD5 footer.
    std::vector<std::byte> build_dsf(std::span<const std::byte> body)
    {
        std::vector<std::byte> d;
        for (char c : std::string_view{"XPLNEDSF"})
            d.push_back(static_cast<std::byte>(c));
        push_i32_le(d, 1);
        d.insert(d.end(), body.begin(), body.end());
        // Compute MD5 over everything so far and append.
        auto digest = xps::io_dsf::Md5::of(std::span<const std::byte>{d.data(), d.size()});
        for (auto b : digest)
            d.push_back(static_cast<std::byte>(b));
        return d;
    }

} // namespace

TEST_CASE("rewrite_dsf_identity reproduces input byte-for-byte when MD5 is correct",
          "[io_dsf][writer]")
{
    // Minimal body: a fake DEFN atom with zero payload.
    std::vector<std::byte> body;
    for (char c : std::string_view{"NFED"}) // DEFN reversed (LE id)
        body.push_back(static_cast<std::byte>(c));
    push_u32_le(body, 8); // atom size (header only)

    auto src = build_dsf(body);
    auto src_path = write_tmp("dsf_rewrite_src.dsf", src);
    auto dst_path = std::filesystem::temp_directory_path() /
                    "xpscenery_tests" / "dsf_rewrite_dst.dsf";
    std::filesystem::remove(dst_path);

    auto rep = xps::io_dsf::rewrite_dsf_identity(src_path, dst_path);
    REQUIRE(rep.has_value());
    REQUIRE(rep->md5_unchanged);
    REQUIRE(rep->source_size == rep->output_size);

    auto dst = xps::io_filesystem::read_binary(dst_path);
    REQUIRE(dst.has_value());
    REQUIRE(dst->size() == src.size());
    REQUIRE(std::memcmp(dst->data(), src.data(), src.size()) == 0);
}

TEST_CASE("rewrite_dsf_identity repairs a corrupted MD5 footer",
          "[io_dsf][writer]")
{
    std::vector<std::byte> body;
    for (int i = 0; i < 32; ++i) body.push_back(static_cast<std::byte>(i));
    auto src = build_dsf(body);
    // Clobber the footer.
    for (std::size_t i = src.size() - 16; i < src.size(); ++i)
        src[i] = std::byte{0xFF};

    auto src_path = write_tmp("dsf_rewrite_bad.dsf", src);
    auto dst_path = std::filesystem::temp_directory_path() /
                    "xpscenery_tests" / "dsf_rewrite_fixed.dsf";
    std::filesystem::remove(dst_path);

    auto rep = xps::io_dsf::rewrite_dsf_identity(src_path, dst_path);
    REQUIRE(rep.has_value());
    REQUIRE_FALSE(rep->md5_unchanged);

    // Re-verify that the written file has a consistent footer.
    auto dst = xps::io_filesystem::read_binary(dst_path);
    REQUIRE(dst.has_value());
    std::span<const std::byte> dst_body{dst->data(), dst->size() - 16};
    auto recomputed = xps::io_dsf::Md5::of(dst_body);
    for (std::size_t i = 0; i < 16; ++i)
    {
        REQUIRE(static_cast<std::uint8_t>((*dst)[dst->size() - 16 + i]) ==
                recomputed[i]);
    }
}

TEST_CASE("rewrite_dsf_identity rejects non-DSF input", "[io_dsf][writer]")
{
    std::vector<std::byte> junk;
    for (int i = 0; i < 64; ++i) junk.push_back(std::byte{0x00});
    auto p = write_tmp("dsf_rewrite_junk.bin", junk);
    auto dst = std::filesystem::temp_directory_path() /
               "xpscenery_tests" / "dsf_rewrite_junk_out.dsf";
    std::filesystem::remove(dst);
    auto rep = xps::io_dsf::rewrite_dsf_identity(p, dst);
    REQUIRE_FALSE(rep.has_value());
}
