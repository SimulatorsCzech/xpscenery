#include <catch2/catch_test_macros.hpp>

#include "xpscenery/io_filesystem/file_io.hpp"
#include "xpscenery/io_raster/geotiff_ifd.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string_view>
#include <vector>

namespace
{

    void push_u16(std::vector<std::byte> &o, std::uint16_t v)
    {
        o.push_back(static_cast<std::byte>(v & 0xFF));
        o.push_back(static_cast<std::byte>((v >> 8) & 0xFF));
    }
    void push_u32(std::vector<std::byte> &o, std::uint32_t v)
    {
        o.push_back(static_cast<std::byte>(v & 0xFF));
        o.push_back(static_cast<std::byte>((v >> 8) & 0xFF));
        o.push_back(static_cast<std::byte>((v >> 16) & 0xFF));
        o.push_back(static_cast<std::byte>((v >> 24) & 0xFF));
    }
    void push_f64(std::vector<std::byte> &o, double d)
    {
        std::uint64_t u;
        std::memcpy(&u, &d, 8);
        for (int i = 0; i < 8; ++i)
            o.push_back(static_cast<std::byte>((u >> (i * 8)) & 0xFF));
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

    struct IfdEntryIn
    {
        std::uint16_t tag;
        std::uint16_t type;
        std::uint32_t count;
        std::uint32_t value_or_offset;
    };

    // Build a tiny classic TIFF (little-endian) with a single IFD.
    std::vector<std::byte>
    build_tiff(const std::vector<IfdEntryIn> &entries,
               const std::vector<std::byte> &out_of_band_payload,
               std::uint32_t oob_offset)
    {
        std::vector<std::byte> f;
        // Header: II*\0  + first_ifd_offset
        f.push_back(std::byte{'I'});
        f.push_back(std::byte{'I'});
        push_u16(f, 42);
        push_u32(f, 8); // first IFD immediately after header

        // IFD
        push_u16(f, static_cast<std::uint16_t>(entries.size()));
        for (const auto &e : entries)
        {
            push_u16(f, e.tag);
            push_u16(f, e.type);
            push_u32(f, e.count);
            push_u32(f, e.value_or_offset);
        }
        push_u32(f, 0); // next IFD = 0

        // Pad to oob_offset, then out-of-band payload.
        while (f.size() < oob_offset)
            f.push_back(std::byte{0});
        f.insert(f.end(), out_of_band_payload.begin(), out_of_band_payload.end());
        return f;
    }

} // namespace

TEST_CASE("read_geotiff_ifd parses basic ImageWidth/Length from classic TIFF",
          "[io_raster][geotiff]")
{
    std::vector<IfdEntryIn> entries;
    entries.push_back({256, 3, 1, 1024});   // ImageWidth  (SHORT, inline)
    entries.push_back({257, 4, 1, 512});    // ImageLength (LONG,  inline)
    entries.push_back({258, 3, 1, 16});     // BitsPerSample
    entries.push_back({262, 3, 1, 1});      // Photometric = BlackIsZero
    entries.push_back({273, 4, 8, 0x1000}); // StripOffsets (count)

    auto tiff = build_tiff(entries, {}, 0);
    auto p = write_tmp("tiff_basic.tif", tiff);

    auto info = xps::io_raster::read_geotiff_ifd(p);
    REQUIRE(info.has_value());
    REQUIRE(info->width == 1024);
    REQUIRE(info->height == 512);
    REQUIRE(info->bits_per_sample == 16);
    REQUIRE(info->photometric == 1);
    REQUIRE(info->strip_count == 8);
    REQUIRE_FALSE(info->is_georeferenced());
    REQUIRE_FALSE(info->has_geokeys());
}

TEST_CASE("read_geotiff_ifd captures ModelPixelScale + ModelTiepoint",
          "[io_raster][geotiff]")
{
    // Out-of-band doubles for 33550 (3 doubles, 24 B) at offset 0x80.
    std::vector<std::byte> oob;
    push_f64(oob, 0.00028); // pixel scale x  ~1 arcsec
    push_f64(oob, 0.00028); // pixel scale y
    push_f64(oob, 0.0);     // pixel scale z
    // ModelTiepointTag (33922) — 6 doubles (48 B), placed right after.
    push_f64(oob, 0.0);  // I
    push_f64(oob, 0.0);  // J
    push_f64(oob, 0.0);  // K
    push_f64(oob, 15.0); // X (lon)
    push_f64(oob, 50.0); // Y (lat)
    push_f64(oob, 0.0);  // Z

    const std::uint32_t oob_offset = 0x80;
    std::vector<IfdEntryIn> entries;
    entries.push_back({256, 4, 1, 100});
    entries.push_back({257, 4, 1, 100});
    entries.push_back({33550, 12, 3, oob_offset});
    entries.push_back({33922, 12, 6, oob_offset + 24});

    auto tiff = build_tiff(entries, oob, oob_offset);
    auto p = write_tmp("tiff_geo.tif", tiff);

    auto info = xps::io_raster::read_geotiff_ifd(p);
    REQUIRE(info.has_value());
    REQUIRE(info->have_pixel_scale);
    REQUIRE(info->pixel_scale_x == 0.00028);
    REQUIRE(info->pixel_scale_y == 0.00028);
    REQUIRE(info->tiepoints.size() == 1);
    REQUIRE(info->tiepoints[0].x == 15.0);
    REQUIRE(info->tiepoints[0].y == 50.0);
    REQUIRE(info->is_georeferenced());
}

TEST_CASE("read_geotiff_ifd rejects non-TIFF input",
          "[io_raster][geotiff]")
{
    std::vector<std::byte> junk;
    for (int i = 0; i < 64; ++i)
        junk.push_back(std::byte{0x11});
    auto p = write_tmp("tiff_junk.bin", junk);
    auto info = xps::io_raster::read_geotiff_ifd(p);
    REQUIRE_FALSE(info.has_value());
}

// ---------------------------------------------------------------------
// BigTIFF (64-bit) coverage
// ---------------------------------------------------------------------
namespace
{

    void push_u64(std::vector<std::byte> &o, std::uint64_t v)
    {
        for (int i = 0; i < 8; ++i)
            o.push_back(static_cast<std::byte>((v >> (i * 8)) & 0xFF));
    }

    struct BigIfdEntryIn
    {
        std::uint16_t tag;
        std::uint16_t type;
        std::uint64_t count;
        std::uint64_t value_or_offset;
    };

    // Build a minimal BigTIFF (little-endian) with a single IFD.
    // Header layout (16 bytes):
    //   "II" + u16(43) + u16(8 offsetSize) + u16(0) + u64(first_ifd_offset)
    std::vector<std::byte>
    build_bigtiff(const std::vector<BigIfdEntryIn> &entries,
                  const std::vector<std::byte> &out_of_band_payload,
                  std::uint64_t oob_offset)
    {
        std::vector<std::byte> f;
        f.push_back(std::byte{'I'});
        f.push_back(std::byte{'I'});
        push_u16(f, 43); // BigTIFF magic
        push_u16(f, 8);  // bytesize of offsets
        push_u16(f, 0);  // constant 0
        push_u64(f, 16); // first IFD offset = right after header

        push_u64(f, static_cast<std::uint64_t>(entries.size()));
        for (const auto &e : entries)
        {
            push_u16(f, e.tag);
            push_u16(f, e.type);
            push_u64(f, e.count);
            push_u64(f, e.value_or_offset);
        }
        push_u64(f, 0); // next IFD = 0

        while (f.size() < oob_offset)
            f.push_back(std::byte{0});
        f.insert(f.end(), out_of_band_payload.begin(), out_of_band_payload.end());
        return f;
    }

} // namespace

TEST_CASE("read_geotiff_ifd walks a BigTIFF first IFD",
          "[io_raster][geotiff][bigtiff]")
{
    std::vector<BigIfdEntryIn> entries;
    // Inline LONG — fits in the 8-byte value slot (BigTIFF).
    entries.push_back({256, 4, 1, 4096}); // ImageWidth
    entries.push_back({257, 4, 1, 2048}); // ImageLength
    entries.push_back({258, 3, 1, 32});   // BitsPerSample (inline SHORT)
    entries.push_back({262, 3, 1, 1});    // Photometric
    // LONG8 — a BigTIFF-only field type (width 8). Should still decode as width.
    entries.push_back({273, 16, 4, 0x200}); // StripOffsets count=4

    auto bt = build_bigtiff(entries, {}, 0);
    auto p = write_tmp("bigtiff_basic.tif", bt);

    auto info = xps::io_raster::read_geotiff_ifd(p);
    REQUIRE(info.has_value());
    REQUIRE(info->is_bigtiff);
    REQUIRE(info->width == 4096);
    REQUIRE(info->height == 2048);
    REQUIRE(info->bits_per_sample == 32);
    REQUIRE(info->photometric == 1);
    REQUIRE(info->strip_count == 4);
}

TEST_CASE("read_geotiff_ifd decodes GeoTIFF keys from a BigTIFF payload",
          "[io_raster][geotiff][bigtiff]")
{
    // Place all out-of-band doubles + GeoKeyDirectory at 0x200.
    std::vector<std::byte> oob;
    // ModelPixelScaleTag — 3 doubles
    push_f64(oob, 0.0005);
    push_f64(oob, 0.0005);
    push_f64(oob, 0.0);
    // ModelTiepointTag — 6 doubles
    push_f64(oob, 0.0);
    push_f64(oob, 0.0);
    push_f64(oob, 0.0);
    push_f64(oob, 16.0); // X
    push_f64(oob, 49.0); // Y
    push_f64(oob, 0.0);
    // GeoKeyDirectoryTag — header + 2 keys = 4 + 2*4 = 12 shorts
    const auto geo_key_off = 0x200u + static_cast<std::uint32_t>(oob.size());
    push_u16(oob, 1); // KeyDirectoryVersion
    push_u16(oob, 1); // KeyRevision
    push_u16(oob, 2); // MinorRevision
    push_u16(oob, 2); // NumberOfKeys
    // key 1: GTModelTypeGeoKey (1024) = ModelTypeGeographic (2)
    push_u16(oob, 1024);
    push_u16(oob, 0);
    push_u16(oob, 1);
    push_u16(oob, 2);
    // key 2: GeographicTypeGeoKey (2048) = WGS 84 (4326)
    push_u16(oob, 2048);
    push_u16(oob, 0);
    push_u16(oob, 1);
    push_u16(oob, 4326);

    std::vector<BigIfdEntryIn> entries;
    entries.push_back({256, 4, 1, 10});
    entries.push_back({257, 4, 1, 10});
    entries.push_back({33550, 12, 3, 0x200});
    entries.push_back({33922, 12, 6, 0x200 + 24});
    entries.push_back({34735, 3, 12, geo_key_off});

    auto bt = build_bigtiff(entries, oob, 0x200);
    auto p = write_tmp("bigtiff_geo.tif", bt);

    auto info = xps::io_raster::read_geotiff_ifd(p);
    REQUIRE(info.has_value());
    REQUIRE(info->is_bigtiff);
    REQUIRE(info->have_pixel_scale);
    REQUIRE(info->pixel_scale_x == 0.0005);
    REQUIRE(info->tiepoints.size() == 1);
    REQUIRE(info->tiepoints[0].x == 16.0);
    REQUIRE(info->tiepoints[0].y == 49.0);
    REQUIRE(info->has_geokeys());
    REQUIRE(info->geo_keys.size() == 2);
    REQUIRE(info->geo_keys[0].key_id == 1024);
    REQUIRE(info->geo_keys[0].value == 2);
    REQUIRE(info->geo_keys[1].key_id == 2048);
    REQUIRE(info->geo_keys[1].value == 4326);
}
