#pragma once
// xpscenery — io_dsf/dsf_cmds.hpp
//
// Decoder for the CMDS atom in X-Plane DSF files. CMDS is a
// variable-length command stream with 35 opcodes (1 byte each) that
// drive geometry emission against the POOL + definition tables.
//
// This header exposes a *stats* walk that counts every opcode
// encountered and surfaces aggregate counters per primitive kind
// (objects placed, polygons, terrain patches, network chains,
// triangles). A full geometric decoder would additionally track
// pool-select / definition-select state and emit primitives; that
// is out of scope for v0.3 read-only summarisation.

#include <array>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>

namespace xps::io_dsf {

/// Opcodes per DSFDefs.h `dsf_Cmd_*` (0..34 inclusive).
inline constexpr std::size_t kCmdOpcodeCount = 35;

struct CmdStats {
    std::array<std::uint64_t, kCmdOpcodeCount> opcode_counts{};

    // High-level aggregates.
    std::uint64_t pool_selects      = 0;
    std::uint64_t def_selects       = 0;
    std::uint64_t objects_placed    = 0;
    std::uint64_t network_chains    = 0;
    std::uint64_t polygons          = 0;
    std::uint64_t terrain_patches   = 0;
    std::uint64_t triangles_emitted = 0;

    /// Total vertices consumed by triangle commands (cross-pool and
    /// same-pool together).
    std::uint64_t triangle_vertices = 0;

    std::uint64_t bytes_consumed    = 0;
};

[[nodiscard]] std::expected<CmdStats, std::string>
read_cmd_stats(const std::filesystem::path& dsf_path);

}  // namespace xps::io_dsf
