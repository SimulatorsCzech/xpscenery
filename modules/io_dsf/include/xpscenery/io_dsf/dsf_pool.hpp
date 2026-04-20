#pragma once
// xpscenery — io_dsf/dsf_pool.hpp
//
// Decoder for DSF "planar numeric" point pools under GEOD:
//   POOL / SCAL — 16-bit point pools (Planar Numeric Table)
//   PO32 / SC32 — 32-bit point pools
//
// POOL atom payload (after 8-byte atom header):
//   int32  plane_size     (number of records in the table)
//   uint8  plane_count    (values per record, e.g. 5 for terrain: lon,lat,z,nx,ny)
//   for each plane:
//     uint8  encode_mode  (0=raw, 1=differenced, 2=RLE, 3=RLE+differenced)
//     <encoded stream>    (see RLE/flat rules)
//
// SCAL atom payload: plane_count × (float32 scale, float32 offset).
//
// Decoded value for point i, plane p:
//   raw16      = decoded uint16
//   normalized = raw16 / 65535.0
//   value      = normalized * scale[p] + offset[p]
// (for 32-bit pools the divisor is 4294967295.0).

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace xps::io_dsf {

struct PointPool {
    std::uint32_t plane_count = 0;   ///< planes per record
    std::uint32_t array_size  = 0;   ///< records in the table
    bool          is_32bit    = false;

    /// Interleaved doubles: size = plane_count * array_size.
    /// Indexed as `points[record * plane_count + plane]`.
    std::vector<double> points;

    /// Per-plane scale/offset used to produce `points`.
    std::vector<double> scales;
    std::vector<double> offsets;
};

/// Enumerate every POOL + PO32 atom under GEOD and decode each
/// against its matching SCAL / SC32 sibling. Returns empty vector
/// if there is no GEOD atom.
[[nodiscard]] std::expected<std::vector<PointPool>, std::string>
read_point_pools(const std::filesystem::path& dsf_path);

}  // namespace xps::io_dsf
