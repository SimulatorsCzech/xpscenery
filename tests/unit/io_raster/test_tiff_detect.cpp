#include <catch2/catch_test_macros.hpp>

#include "xpscenery/io_raster/tiff_detect.hpp"

#include <array>
#include <cstdint>

using namespace xps::io_raster;

TEST_CASE("detect_tiff_bytes: classic little-endian TIFF", "[io_raster][tiff]")
{
    // II*, version 42, first IFD offset = 0x00000008
    constexpr std::array<std::uint8_t, 12> hdr{
        'I',
        'I',
        0x2A,
        0x00,
        0x08,
        0x00,
        0x00,
        0x00,
        0,
        0,
        0,
        0,
    };
    const auto info = detect_tiff_bytes(hdr.data(), hdr.size());
    REQUIRE(info.kind == TiffKind::tiff_le);
    REQUIRE(info.little_endian);
    REQUIRE_FALSE(info.is_bigtiff);
    REQUIRE(info.first_ifd_offset == 8u);
}

TEST_CASE("detect_tiff_bytes: classic big-endian TIFF", "[io_raster][tiff]")
{
    constexpr std::array<std::uint8_t, 8> hdr{
        'M',
        'M',
        0x00,
        0x2A,
        0x00,
        0x00,
        0x00,
        0x10,
    };
    const auto info = detect_tiff_bytes(hdr.data(), hdr.size());
    REQUIRE(info.kind == TiffKind::tiff_be);
    REQUIRE_FALSE(info.little_endian);
    REQUIRE(info.first_ifd_offset == 16u);
}

TEST_CASE("detect_tiff_bytes: BigTIFF little-endian", "[io_raster][tiff]")
{
    constexpr std::array<std::uint8_t, 16> hdr{
        'I',
        'I',
        0x2B,
        0x00, // version 43 = BigTIFF
        0x08,
        0x00, // bytesize = 8
        0x00,
        0x00, // constant = 0
        0x10,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00, // first IFD = 0x10
    };
    const auto info = detect_tiff_bytes(hdr.data(), hdr.size());
    REQUIRE(info.kind == TiffKind::bigtiff_le);
    REQUIRE(info.is_bigtiff);
    REQUIRE(info.first_ifd_offset == 0x10u);
}

TEST_CASE("detect_tiff_bytes: not a TIFF", "[io_raster][tiff]")
{
    constexpr std::array<std::uint8_t, 8> hdr{
        'X',
        'P',
        'L',
        'N',
        'E',
        'D',
        'S',
        'F',
    };
    const auto info = detect_tiff_bytes(hdr.data(), hdr.size());
    REQUIRE(info.kind == TiffKind::none);
    REQUIRE(info.first_ifd_offset == 0u);
}

TEST_CASE("detect_tiff_bytes: too short", "[io_raster][tiff]")
{
    constexpr std::array<std::uint8_t, 4> hdr{'I', 'I', 0x2A, 0x00};
    const auto info = detect_tiff_bytes(hdr.data(), hdr.size());
    REQUIRE(info.kind == TiffKind::none);
}
