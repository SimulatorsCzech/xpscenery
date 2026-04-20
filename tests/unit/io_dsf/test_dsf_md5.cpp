#include <catch2/catch_test_macros.hpp>

#include "xpscenery/io_dsf/dsf_md5.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <cstddef>
#include <cstring>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using xps::io_dsf::Md5;
using xps::io_dsf::Md5Digest;
using xps::io_dsf::to_hex;

namespace {

std::span<const std::byte> as_bytes(std::string_view s) {
    return {reinterpret_cast<const std::byte*>(s.data()), s.size()};
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

TEST_CASE("MD5 of empty string matches RFC 1321", "[io_dsf][md5]") {
    auto d = Md5::of({});
    REQUIRE(to_hex(d) == "d41d8cd98f00b204e9800998ecf8427e");
}

TEST_CASE("MD5 of 'abc' matches RFC 1321", "[io_dsf][md5]") {
    auto d = Md5::of(as_bytes("abc"));
    REQUIRE(to_hex(d) == "900150983cd24fb0d6963f7d28e17f72");
}

TEST_CASE("MD5 of 'message digest' matches RFC 1321", "[io_dsf][md5]") {
    auto d = Md5::of(as_bytes("message digest"));
    REQUIRE(to_hex(d) == "f96b697d7cb7938d525a2f31aaf161d0");
}

TEST_CASE("MD5 of a long string (80 chars) matches RFC 1321",
          "[io_dsf][md5]") {
    auto d = Md5::of(as_bytes(
        "12345678901234567890123456789012345678901234567890"
        "123456789012345678901234567890"));
    REQUIRE(to_hex(d) == "57edf4a22be3c955ac49da2e2107b67a");
}

TEST_CASE("MD5 incremental update equals one-shot", "[io_dsf][md5]") {
    std::string payload(1000, 'x');
    // Three chunks that straddle the 64-byte block boundary.
    Md5 h;
    h.update(as_bytes({payload.data(),       13}));
    h.update(as_bytes({payload.data() + 13,  700}));
    h.update(as_bytes({payload.data() + 713, 287}));
    const auto chunked = h.finalize();
    const auto oneshot = Md5::of(as_bytes(payload));
    REQUIRE(chunked == oneshot);
}

TEST_CASE("verify_md5_footer matches a valid file", "[io_dsf][md5]") {
    // Body = 1024 bytes of 'A', footer = its MD5 (16 bytes).
    std::vector<std::byte> buf(1024, static_cast<std::byte>('A'));
    const auto d = Md5::of(buf);
    for (auto b : d) buf.push_back(static_cast<std::byte>(b));

    auto p = write_tmp("dsf_md5_ok.bin", buf);
    auto r = xps::io_dsf::verify_md5_footer(p);
    REQUIRE(r.has_value());
    REQUIRE(r->ok);
    REQUIRE(r->computed == r->stored);
}

TEST_CASE("verify_md5_footer rejects corrupted footer", "[io_dsf][md5]") {
    std::vector<std::byte> buf(256, static_cast<std::byte>('B'));
    const auto d = Md5::of(buf);
    for (auto b : d) buf.push_back(static_cast<std::byte>(b));
    buf.back() ^= std::byte{0xFF};  // flip a single bit in the footer

    auto p = write_tmp("dsf_md5_bad.bin", buf);
    auto r = xps::io_dsf::verify_md5_footer(p);
    REQUIRE(r.has_value());
    REQUIRE_FALSE(r->ok);
}
