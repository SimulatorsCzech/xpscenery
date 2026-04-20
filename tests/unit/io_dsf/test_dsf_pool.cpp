#include <catch2/catch_test_macros.hpp>

#include "xpscenery/io_dsf/dsf_pool.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

void push_u16_le(std::vector<std::byte>& o, std::uint16_t v) {
    o.push_back(static_cast<std::byte>( v        & 0xFF));
    o.push_back(static_cast<std::byte>((v >>  8) & 0xFF));
}
void push_u32_le(std::vector<std::byte>& o, std::uint32_t v) {
    o.push_back(static_cast<std::byte>( v        & 0xFF));
    o.push_back(static_cast<std::byte>((v >>  8) & 0xFF));
    o.push_back(static_cast<std::byte>((v >> 16) & 0xFF));
    o.push_back(static_cast<std::byte>((v >> 24) & 0xFF));
}
void push_i32_le(std::vector<std::byte>& o, std::int32_t v) {
    push_u32_le(o, static_cast<std::uint32_t>(v));
}
void push_f32_le(std::vector<std::byte>& o, float f) {
    std::uint32_t u; std::memcpy(&u, &f, 4); push_u32_le(o, u);
}
constexpr std::uint32_t make_id4(char a, char b, char c, char d) noexcept {
    return (static_cast<std::uint32_t>(a) << 24) |
           (static_cast<std::uint32_t>(b) << 16) |
           (static_cast<std::uint32_t>(c) <<  8) |
            static_cast<std::uint32_t>(d);
}

// Build one POOL atom (16-bit) with plane_count planes, each
// encoded using mode 0 (Raw). Each plane has plane_size u16 values.
std::vector<std::byte> pool_atom_raw(std::uint32_t plane_size,
                                     std::uint8_t plane_count,
                                     const std::vector<std::uint16_t>& interleaved)
{
    // Payload header
    std::vector<std::byte> payload;
    push_u32_le(payload, plane_size);
    payload.push_back(static_cast<std::byte>(plane_count));

    // For each plane: mode byte (0 = raw) + plane_size values.
    for (std::uint8_t p = 0; p < plane_count; ++p) {
        payload.push_back(std::byte{0});
        for (std::uint32_t i = 0; i < plane_size; ++i) {
            push_u16_le(payload, interleaved[i * plane_count + p]);
        }
    }

    std::vector<std::byte> atom;
    push_u32_le(atom, make_id4('P','O','O','L'));
    push_u32_le(atom, static_cast<std::uint32_t>(8 + payload.size()));
    atom.insert(atom.end(), payload.begin(), payload.end());
    return atom;
}

std::vector<std::byte> scal_atom(const std::vector<std::pair<float,float>>& ps)
{
    std::vector<std::byte> payload;
    for (auto [s, o] : ps) { push_f32_le(payload, s); push_f32_le(payload, o); }
    std::vector<std::byte> atom;
    push_u32_le(atom, make_id4('S','C','A','L'));
    push_u32_le(atom, static_cast<std::uint32_t>(8 + payload.size()));
    atom.insert(atom.end(), payload.begin(), payload.end());
    return atom;
}

std::vector<std::byte> wrap_geod(std::span<const std::byte> children)
{
    std::vector<std::byte> atom;
    push_u32_le(atom, make_id4('G','E','O','D'));
    push_u32_le(atom, static_cast<std::uint32_t>(8 + children.size()));
    atom.insert(atom.end(), children.begin(), children.end());
    return atom;
}

std::vector<std::byte> synth_dsf(std::span<const std::byte> body)
{
    std::vector<std::byte> out;
    for (char c : std::string_view{"XPLNEDSF"}) out.push_back(static_cast<std::byte>(c));
    push_i32_le(out, 1);
    out.insert(out.end(), body.begin(), body.end());
    for (int i = 0; i < 16; ++i) out.push_back(std::byte{0});
    return out;
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

TEST_CASE("read_point_pools returns empty when GEOD is absent",
          "[io_dsf][pool]") {
    auto dsf = synth_dsf({});
    auto p = write_tmp("dsf_no_geod.dsf", dsf);
    auto pools = xps::io_dsf::read_point_pools(p);
    REQUIRE(pools.has_value());
    REQUIRE(pools->empty());
}

TEST_CASE("read_point_pools decodes a single 16-bit raw pool",
          "[io_dsf][pool]") {
    // plane_size = 3 records, plane_count = 2 planes, raw encoding.
    // interleaved[i*2 + p]
    std::vector<std::uint16_t> interleaved = {
        0,       0,           // record 0: (0, 0)
        32767,   16383,       // record 1
        65535,   65535        // record 2: max
    };
    auto pool = pool_atom_raw(3, 2, interleaved);
    auto scal = scal_atom({{2.0f, -1.0f}, {10.0f, 5.0f}});
    std::vector<std::byte> children;
    children.insert(children.end(), pool.begin(), pool.end());
    children.insert(children.end(), scal.begin(), scal.end());
    auto geod = wrap_geod(children);
    auto dsf  = synth_dsf(geod);

    auto p = write_tmp("dsf_pool_raw.dsf", dsf);
    auto pools = xps::io_dsf::read_point_pools(p);
    REQUIRE(pools.has_value());
    REQUIRE(pools->size() == 1);

    const auto& pp = (*pools)[0];
    REQUIRE(pp.plane_count == 2);
    REQUIRE(pp.array_size  == 3);
    REQUIRE_FALSE(pp.is_32bit);
    REQUIRE(pp.points.size() == 6);

    // plane 0: scale=2, offset=-1, value = raw/65535 * 2 - 1
    REQUIRE(pp.points[0] == -1.0);                       // 0     → -1
    REQUIRE(pp.points[2] > -0.001);                      // 32767 → ~0
    REQUIRE(pp.points[2] <  0.001);
    REQUIRE(pp.points[4] == 1.0);                        // 65535 → 1

    // plane 1: scale=10, offset=5, value = raw/65535 * 10 + 5
    REQUIRE(pp.points[1] == 5.0);
    REQUIRE(pp.points[5] == 15.0);
}

TEST_CASE("read_point_pools decodes an RLE+differenced pool",
          "[io_dsf][pool]") {
    // plane_count = 1, plane_size = 6. Encode mode 3 (RLE+differenced).
    // We'll produce cumulative sums [10, 20, 30, 40, 41, 42].
    // Deltas: [10, 10, 10, 10, 1, 1]
    // RLE stream: run of 4 × 10, then run of 2 × 1.
    //   code = 0x80 | 4, value 10 (u16 LE)
    //   code = 0x80 | 2, value  1 (u16 LE)
    std::vector<std::byte> pool_payload;
    push_u32_le(pool_payload, 6);                       // plane_size
    pool_payload.push_back(std::byte{1});               // plane_count
    pool_payload.push_back(std::byte{3});               // encoding = RLE+differenced
    pool_payload.push_back(std::byte{0x80 | 4});
    push_u16_le(pool_payload, 10);
    pool_payload.push_back(std::byte{0x80 | 2});
    push_u16_le(pool_payload, 1);

    std::vector<std::byte> pool_atom;
    push_u32_le(pool_atom, make_id4('P','O','O','L'));
    push_u32_le(pool_atom, static_cast<std::uint32_t>(8 + pool_payload.size()));
    pool_atom.insert(pool_atom.end(), pool_payload.begin(), pool_payload.end());

    // scale = 65535 (so we recover raw after recip_65535), offset = 0
    auto scal = scal_atom({{65535.0f, 0.0f}});

    std::vector<std::byte> children;
    children.insert(children.end(), pool_atom.begin(), pool_atom.end());
    children.insert(children.end(), scal.begin(),      scal.end());
    auto dsf = synth_dsf(wrap_geod(children));

    auto p = write_tmp("dsf_pool_rled.dsf", dsf);
    auto pools = xps::io_dsf::read_point_pools(p);
    REQUIRE(pools.has_value());
    REQUIRE(pools->size() == 1);

    const auto& pp = (*pools)[0];
    REQUIRE(pp.array_size == 6);
    REQUIRE(pp.points.size() == 6);
    // With scale=65535 and divisor=65535, we recover the raw accumulators.
    REQUIRE(pp.points[0] == 10.0);
    REQUIRE(pp.points[1] == 20.0);
    REQUIRE(pp.points[2] == 30.0);
    REQUIRE(pp.points[3] == 40.0);
    REQUIRE(pp.points[4] == 41.0);
    REQUIRE(pp.points[5] == 42.0);
}

TEST_CASE("read_point_pools rejects POOL without matching SCAL",
          "[io_dsf][pool]") {
    auto pool = pool_atom_raw(1, 1, {0});
    std::vector<std::byte> children;
    children.insert(children.end(), pool.begin(), pool.end());
    auto dsf = synth_dsf(wrap_geod(children));
    auto p = write_tmp("dsf_pool_no_scal.dsf", dsf);
    auto pools = xps::io_dsf::read_point_pools(p);
    REQUIRE_FALSE(pools.has_value());
}
