#pragma once
// xpscenery -- io_dsf/dsf_md5.hpp
//
// MD5 footer verification for DSF files. X-Plane writes the last 16
// bytes of every DSF as MD5(file_bytes[0 .. end-16]). This header
// exposes both a standalone MD5 hasher (purely byte-level, no deps)
// and a high-level `verify_md5_footer()` helper that returns whether
// the signature matches.

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string>

namespace xps::io_dsf
{

    using Md5Digest = std::array<std::uint8_t, 16>;

    /// Plain RFC 1321 MD5. NOT a cryptographic primitive for auth — we use
    /// it exclusively to verify the X-Plane DSF file checksum.
    class Md5
    {
    public:
        Md5() noexcept { reset(); }

        void reset() noexcept;
        void update(std::span<const std::byte> data) noexcept;
        [[nodiscard]] Md5Digest finalize() noexcept;

        /// One-shot helper.
        [[nodiscard]] static Md5Digest of(std::span<const std::byte> data) noexcept;

    private:
        std::uint32_t state_[4]{};
        std::uint64_t count_bits_{};
        std::uint8_t buffer_[64]{};
    };

    struct Md5VerifyResult
    {
        bool ok = false;
        Md5Digest computed{};
        Md5Digest stored{};
    };

    /// Read the DSF file, compute MD5 of everything except the last 16
    /// bytes, and compare with the stored footer. An I/O failure yields
    /// an error; a digest mismatch yields `Md5VerifyResult{ok=false, …}`.
    [[nodiscard]] std::expected<Md5VerifyResult, std::string>
    verify_md5_footer(const std::filesystem::path &dsf_path);

    /// Lower-case hex string of a digest.
    [[nodiscard]] std::string to_hex(const Md5Digest &d);

} // namespace xps::io_dsf
