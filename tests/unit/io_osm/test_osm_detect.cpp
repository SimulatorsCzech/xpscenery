#include <catch2/catch_test_macros.hpp>

#include "xpscenery/io_osm/osm_detect.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

using namespace xps::io_osm;

TEST_CASE("detect_osm_bytes: OSM PBF with OSMHeader blob", "[io_osm][pbf]") {
    // BlobHeader length = 13 (big-endian u32 prefix) — chosen so the
    // first tag byte (proto field 1, wire type 2) + string length byte
    // + "OSMHeader" (9) = 11 bytes, plus a couple of padding bytes.
    std::vector<std::uint8_t> buf{
        0x00, 0x00, 0x00, 0x0D,  // length prefix = 13
        0x0A,                    // field 1, wire 2
        0x09,                    // string length
        'O','S','M','H','e','a','d','e','r',
        0x00, 0x00,              // padding to reach BlobHeader length
    };
    const auto info = detect_osm_bytes(buf.data(), buf.size());
    REQUIRE(info.kind == OsmKind::pbf);
    REQUIRE(info.first_blob_header_len == 13u);
}

TEST_CASE("detect_osm_bytes: OSM XML file", "[io_osm][xml]") {
    constexpr std::string_view xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<osm version=\"0.6\" generator=\"xpscenery-tests\">\n"
        "</osm>";
    const auto info = detect_osm_bytes(
        reinterpret_cast<const std::uint8_t*>(xml.data()), xml.size());
    REQUIRE(info.kind == OsmKind::xml);
    REQUIRE(info.first_blob_header_len == 0u);
}

TEST_CASE("detect_osm_bytes: random binary is not OSM", "[io_osm]") {
    constexpr std::array<std::uint8_t, 12> junk{
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0x12, 0x34, 0x56, 0x78,
    };
    const auto info = detect_osm_bytes(junk.data(), junk.size());
    REQUIRE(info.kind == OsmKind::none);
}

TEST_CASE("detect_osm_bytes: XML without <osm root is not OSM", "[io_osm][xml]") {
    constexpr std::string_view xml =
        "<?xml version=\"1.0\"?><root><item/></root>";
    const auto info = detect_osm_bytes(
        reinterpret_cast<const std::uint8_t*>(xml.data()), xml.size());
    REQUIRE(info.kind == OsmKind::none);
}
