#include "xpscenery/io_dsf/dsf_cmds.hpp"
#include "xpscenery/io_dsf/dsf_atoms.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <bit>
#include <cstring>
#include <format>

namespace xps::io_dsf {

namespace {

constexpr std::uint32_t make_id4(char a, char b, char c, char d) noexcept {
    return (static_cast<std::uint32_t>(a) << 24) |
           (static_cast<std::uint32_t>(b) << 16) |
           (static_cast<std::uint32_t>(c) <<  8) |
            static_cast<std::uint32_t>(d);
}
constexpr std::uint32_t kCmdsId = make_id4('C','M','D','S');

enum : std::uint8_t {
    Cmd_Reserved                    = 0,
    Cmd_PoolSelect                  = 1,
    Cmd_JunctionOffsetSelect        = 2,
    Cmd_SetDefinition8              = 3,
    Cmd_SetDefinition16             = 4,
    Cmd_SetDefinition32             = 5,
    Cmd_SetRoadSubtype8             = 6,
    Cmd_Object                      = 7,
    Cmd_ObjectRange                 = 8,
    Cmd_NetworkChain                = 9,
    Cmd_NetworkChainRange           = 10,
    Cmd_NetworkChain32              = 11,
    Cmd_Polygon                     = 12,
    Cmd_PolygonRange                = 13,
    Cmd_NestedPolygon               = 14,
    Cmd_NestedPolygonRange          = 15,
    Cmd_TerrainPatch                = 16,
    Cmd_TerrainPatchFlags           = 17,
    Cmd_TerrainPatchFlagsLOD        = 18,
    Cmd_Triangle                    = 23,
    Cmd_TriangleCrossPool           = 24,
    Cmd_TriangleRange               = 25,
    Cmd_TriangleStrip               = 26,
    Cmd_TriangleStripCrossPool      = 27,
    Cmd_TriangleStripRange          = 28,
    Cmd_TriangleFan                 = 29,
    Cmd_TriangleFanCrossPool        = 30,
    Cmd_TriangleFanRange            = 31,
    Cmd_Comment8                    = 32,
    Cmd_Comment16                   = 33,
    Cmd_Comment32                   = 34,
};

struct Cursor {
    const std::byte* p;
    const std::byte* end;

    bool  has(std::size_t n) const noexcept { return (end - p) >= static_cast<std::ptrdiff_t>(n); }
    void  skip(std::size_t n) noexcept       { p += n; }
    std::uint8_t  u8()  noexcept { std::uint8_t v; std::memcpy(&v, p, 1); p += 1; return v; }
    std::uint16_t u16() noexcept {
        std::uint16_t v; std::memcpy(&v, p, 2); p += 2;
        if constexpr (std::endian::native == std::endian::big) v = std::byteswap(v);
        return v;
    }
    std::uint32_t u32() noexcept {
        std::uint32_t v; std::memcpy(&v, p, 4); p += 4;
        if constexpr (std::endian::native == std::endian::big) v = std::byteswap(v);
        return v;
    }
};

[[nodiscard]] bool walk_cmds(std::span<const std::byte> bytes,
                              CmdStats& out, std::string& err)
{
    Cursor c{bytes.data(), bytes.data() + bytes.size()};

    auto need = [&](std::size_t n, const char* ctx) -> bool {
        if (!c.has(n)) {
            err = std::format("CMDS stream truncated at offset {} while reading {}",
                              static_cast<std::size_t>(c.p - bytes.data()), ctx);
            return false;
        }
        return true;
    };

    while (c.p < c.end) {
        if (!need(1, "opcode")) return false;
        const std::uint8_t opcode = c.u8();
        if (opcode < kCmdOpcodeCount) {
            ++out.opcode_counts[opcode];
        }

        switch (opcode) {
        case Cmd_Reserved:
            err = "CMDS stream contains reserved opcode 0";
            return false;

        case Cmd_PoolSelect:
            if (!need(2, "PoolSelect index")) return false;
            c.u16();
            ++out.pool_selects;
            break;

        case Cmd_JunctionOffsetSelect:
        case Cmd_SetDefinition32:
        case Cmd_ObjectRange:
        case Cmd_NetworkChainRange:
            if (!need(4, "u32 arg")) return false;
            c.skip(4);
            if (opcode == Cmd_SetDefinition32) ++out.def_selects;
            if (opcode == Cmd_ObjectRange)     ++out.objects_placed;
            if (opcode == Cmd_NetworkChainRange) ++out.network_chains;
            break;

        case Cmd_SetDefinition8:
        case Cmd_SetRoadSubtype8:
            if (!need(1, "u8 arg")) return false;
            c.skip(1);
            if (opcode == Cmd_SetDefinition8) ++out.def_selects;
            break;

        case Cmd_SetDefinition16:
        case Cmd_Object:
            if (!need(2, "u16 arg")) return false;
            c.skip(2);
            if (opcode == Cmd_SetDefinition16) ++out.def_selects;
            if (opcode == Cmd_Object)           ++out.objects_placed;
            break;

        case Cmd_NetworkChain: {
            if (!need(1, "NetworkChain count")) return false;
            const std::uint8_t n = c.u8();
            if (!need(2u * n, "NetworkChain indices")) return false;
            c.skip(2u * n);
            ++out.network_chains;
            break;
        }
        case Cmd_NetworkChain32: {
            if (!need(1, "NetworkChain32 count")) return false;
            const std::uint8_t n = c.u8();
            if (!need(4u * n, "NetworkChain32 indices")) return false;
            c.skip(4u * n);
            ++out.network_chains;
            break;
        }

        case Cmd_Polygon: {
            if (!need(3, "Polygon param+count")) return false;
            c.u16();                                  // param
            const std::uint8_t n = c.u8();
            if (!need(2u * n, "Polygon indices")) return false;
            c.skip(2u * n);
            ++out.polygons;
            break;
        }
        case Cmd_PolygonRange:
            if (!need(6, "PolygonRange args")) return false;
            c.skip(6);
            ++out.polygons;
            break;
        case Cmd_NestedPolygon: {
            if (!need(3, "NestedPolygon param+count")) return false;
            c.u16();                                  // param
            std::uint8_t rings = c.u8();
            while (rings--) {
                if (!need(1, "NestedPolygon ring count")) return false;
                const std::uint8_t n = c.u8();
                if (!need(2u * n, "NestedPolygon ring indices")) return false;
                c.skip(2u * n);
            }
            ++out.polygons;
            break;
        }
        case Cmd_NestedPolygonRange: {
            if (!need(5, "NestedPolygonRange head")) return false;
            c.u16();                                  // param
            const std::uint8_t n = c.u8();
            c.u16();                                  // start
            if (!need(2u * n, "NestedPolygonRange ring offsets")) return false;
            c.skip(2u * n);
            ++out.polygons;
            break;
        }

        case Cmd_TerrainPatchFlagsLOD:
            if (!need(8, "TerrainPatchFlagsLOD near/far")) return false;
            c.skip(8);
            [[fallthrough]];
        case Cmd_TerrainPatchFlags:
            if (!need(1, "TerrainPatchFlags")) return false;
            c.skip(1);
            [[fallthrough]];
        case Cmd_TerrainPatch:
            ++out.terrain_patches;
            break;

        case Cmd_Triangle:
        case Cmd_TriangleStrip:
        case Cmd_TriangleFan: {
            if (!need(1, "Triangle* count")) return false;
            const std::uint8_t n = c.u8();
            if (!need(2u * n, "Triangle* indices")) return false;
            c.skip(2u * n);
            out.triangles_emitted += 1;
            out.triangle_vertices += n;
            break;
        }
        case Cmd_TriangleCrossPool:
        case Cmd_TriangleStripCrossPool:
        case Cmd_TriangleFanCrossPool: {
            if (!need(1, "Triangle*CrossPool count")) return false;
            const std::uint8_t n = c.u8();
            if (!need(4u * n, "Triangle*CrossPool tuples")) return false;
            c.skip(4u * n);
            out.triangles_emitted += 1;
            out.triangle_vertices += n;
            break;
        }
        case Cmd_TriangleRange:
        case Cmd_TriangleStripRange:
        case Cmd_TriangleFanRange: {
            if (!need(4, "Triangle*Range args")) return false;
            const std::uint16_t first = c.u16();
            const std::uint16_t last  = c.u16();
            out.triangles_emitted += 1;
            out.triangle_vertices += (last > first) ? (last - first) : 0;
            break;
        }

        case Cmd_Comment8: {
            if (!need(1, "Comment8 length")) return false;
            const std::uint8_t n = c.u8();
            if (!need(n, "Comment8 body")) return false;
            c.skip(n);
            break;
        }
        case Cmd_Comment16: {
            if (!need(2, "Comment16 length")) return false;
            const std::uint16_t n = c.u16();
            if (!need(n, "Comment16 body")) return false;
            c.skip(n);
            break;
        }
        case Cmd_Comment32: {
            if (!need(4, "Comment32 length")) return false;
            const std::uint32_t n = c.u32();
            if (!need(n, "Comment32 body")) return false;
            c.skip(n);
            break;
        }

        default:
            err = std::format("Unknown CMDS opcode {} at offset {}",
                              static_cast<int>(opcode),
                              static_cast<std::size_t>(c.p - bytes.data() - 1));
            return false;
        }
    }
    out.bytes_consumed = static_cast<std::uint64_t>(c.p - bytes.data());
    return true;
}

}  // namespace

std::expected<CmdStats, std::string>
read_cmd_stats(const std::filesystem::path& dsf_path)
{
    auto atoms = read_top_level_atoms(dsf_path);
    if (!atoms) return std::unexpected(std::move(atoms.error()));

    const TopLevelAtom* cmds = nullptr;
    for (const auto& a : *atoms) if (a.raw_id == kCmdsId) { cmds = &a; break; }

    CmdStats stats{};
    if (!cmds) return stats;

    auto bytes = xps::io_filesystem::read_binary(dsf_path);
    if (!bytes) return std::unexpected(std::move(bytes.error()));

    std::span<const std::byte> payload{
        bytes->data() + cmds->offset + 8,
        static_cast<std::size_t>(cmds->size) - 8
    };
    std::string err;
    if (!walk_cmds(payload, stats, err)) {
        return std::unexpected(std::move(err));
    }
    return stats;
}

}  // namespace xps::io_dsf
