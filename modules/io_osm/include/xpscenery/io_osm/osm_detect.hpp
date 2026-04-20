#pragma once
// xpscenery — io_osm/osm_detect.hpp
//
// Classify a file as an OpenStreetMap PBF, an OSM XML document, or
// something else. This is a byte-level probe — no protobuf decoding
// and no XML parsing happens here; full decoding will live in later
// modules (will likely depend on osmpbf / pugixml via vcpkg).

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>

namespace xps::io_osm {

enum class OsmKind {
    none,     ///< Not recognised as an OSM payload.
    pbf,      ///< OpenStreetMap Protocolbuffer Binary Format (.osm.pbf).
    xml,      ///< OpenStreetMap XML (.osm / .osm.xml).
};

struct OsmInfo {
    OsmKind kind = OsmKind::none;
    /// For PBF: length of the first BlobHeader (big-endian uint32 prefix).
    /// For XML: 0 (not applicable).
    std::uint32_t first_blob_header_len = 0;
};

/// Classify the file at `path` by reading up to the first few hundred
/// bytes. Errors reported only for I/O failures.
[[nodiscard]] std::expected<OsmInfo, std::string>
detect_osm(const std::filesystem::path& path);

/// In-memory variant. Never reads past `size` bytes.
[[nodiscard]] OsmInfo detect_osm_bytes(const std::uint8_t* data,
                                       std::size_t size) noexcept;

}  // namespace xps::io_osm
