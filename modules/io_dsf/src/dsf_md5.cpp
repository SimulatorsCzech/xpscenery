#include "xpscenery/io_dsf/dsf_md5.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <bit>
#include <cstring>
#include <format>

namespace xps::io_dsf {

namespace {

// MD5 per-round constants (RFC 1321).
constexpr std::uint32_t kT[64] = {
    0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE,
    0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
    0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE,
    0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,
    0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA,
    0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
    0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED,
    0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,
    0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C,
    0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
    0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05,
    0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,
    0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039,
    0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
    0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1,
    0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391,
};

constexpr std::uint32_t kS[64] = {
    7,12,17,22, 7,12,17,22, 7,12,17,22, 7,12,17,22,
    5, 9,14,20, 5, 9,14,20, 5, 9,14,20, 5, 9,14,20,
    4,11,16,23, 4,11,16,23, 4,11,16,23, 4,11,16,23,
    6,10,15,21, 6,10,15,21, 6,10,15,21, 6,10,15,21,
};

constexpr std::uint32_t rotl(std::uint32_t x, std::uint32_t n) noexcept {
    return (x << n) | (x >> (32 - n));
}

void transform(std::uint32_t state[4], const std::uint8_t block[64]) noexcept {
    std::uint32_t M[16];
    for (int i = 0; i < 16; ++i) {
        std::memcpy(&M[i], block + i * 4, 4);
        if constexpr (std::endian::native == std::endian::big) {
            M[i] = std::byteswap(M[i]);
        }
    }

    std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    for (int i = 0; i < 64; ++i) {
        std::uint32_t f, g;
        if      (i < 16) { f = (b & c) | (~b & d);          g = i; }
        else if (i < 32) { f = (d & b) | (~d & c);          g = (5 * i + 1) & 15; }
        else if (i < 48) { f = b ^ c ^ d;                   g = (3 * i + 5) & 15; }
        else             { f = c ^ (b | ~d);                g = (7 * i)     & 15; }

        const std::uint32_t temp = d;
        d = c;
        c = b;
        b = b + rotl(a + f + kT[i] + M[g], kS[i]);
        a = temp;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

}  // namespace

void Md5::reset() noexcept {
    state_[0] = 0x67452301;
    state_[1] = 0xEFCDAB89;
    state_[2] = 0x98BADCFE;
    state_[3] = 0x10325476;
    count_bits_ = 0;
    std::memset(buffer_, 0, sizeof(buffer_));
}

void Md5::update(std::span<const std::byte> data) noexcept {
    const auto* p = reinterpret_cast<const std::uint8_t*>(data.data());
    std::size_t len = data.size();

    const std::size_t have = (count_bits_ / 8) % 64;
    count_bits_ += static_cast<std::uint64_t>(len) * 8;

    std::size_t i = 0;
    if (have) {
        const std::size_t need = 64 - have;
        if (len < need) {
            std::memcpy(buffer_ + have, p, len);
            return;
        }
        std::memcpy(buffer_ + have, p, need);
        transform(state_, buffer_);
        i = need;
    }
    for (; i + 64 <= len; i += 64) {
        transform(state_, p + i);
    }
    if (i < len) {
        std::memcpy(buffer_, p + i, len - i);
    }
}

Md5Digest Md5::finalize() noexcept {
    const std::uint64_t bit_count = count_bits_;
    std::uint8_t pad[72]{};
    pad[0] = 0x80;

    const std::size_t have = (bit_count / 8) % 64;
    const std::size_t pad_len = (have < 56) ? (56 - have) : (120 - have);

    update({reinterpret_cast<const std::byte*>(pad), pad_len});

    // Append 64-bit little-endian bit count.
    std::uint8_t len_le[8];
    for (int i = 0; i < 8; ++i) {
        len_le[i] = static_cast<std::uint8_t>((bit_count >> (8 * i)) & 0xFF);
    }
    update({reinterpret_cast<const std::byte*>(len_le), 8});

    Md5Digest out{};
    for (int i = 0; i < 4; ++i) {
        std::uint32_t s = state_[i];
        if constexpr (std::endian::native == std::endian::big) {
            s = std::byteswap(s);
        }
        std::memcpy(out.data() + i * 4, &s, 4);
    }
    reset();
    return out;
}

Md5Digest Md5::of(std::span<const std::byte> data) noexcept {
    Md5 h;
    h.update(data);
    return h.finalize();
}

std::string to_hex(const Md5Digest& d) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string s;
    s.resize(32);
    for (std::size_t i = 0; i < d.size(); ++i) {
        s[2*i    ] = kHex[(d[i] >> 4) & 0xF];
        s[2*i + 1] = kHex[ d[i]       & 0xF];
    }
    return s;
}

std::expected<Md5VerifyResult, std::string>
verify_md5_footer(const std::filesystem::path& dsf_path)
{
    auto bytes = xps::io_filesystem::read_binary(dsf_path);
    if (!bytes) return std::unexpected(std::move(bytes.error()));
    if (bytes->size() < 16) {
        return std::unexpected(std::format(
            "DSF file '{}' is shorter than MD5 footer (16 bytes)",
            dsf_path.string()));
    }

    const auto body_size = bytes->size() - 16;
    const Md5Digest computed =
        Md5::of(std::span<const std::byte>{bytes->data(), body_size});

    Md5VerifyResult r;
    r.computed = computed;
    std::memcpy(r.stored.data(), bytes->data() + body_size, 16);
    r.ok = (r.computed == r.stored);
    return r;
}

}  // namespace xps::io_dsf
