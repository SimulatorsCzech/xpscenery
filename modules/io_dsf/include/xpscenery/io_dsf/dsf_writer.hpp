#pragma once
// xpscenery — io_dsf/dsf_writer.hpp
//
// Round-trip DSF rewriter:
//   1. Read the source file into memory.
//   2. Validate cookie + version.
//   3. Drop the existing 16-byte MD5 footer.
//   4. Optionally: zero-copy passthrough of the atom region (identity mode).
//   5. Re-compute MD5 over (header + atoms) and append as new footer.
//   6. Atomically write to the destination path.
//
// This is deliberately byte-faithful — it never reorders or repacks
// atoms. It is the basis for a future atom-level writer (v0.3.0+).

#include <cstddef>
#include <cstdint>
#include <array>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace xps::io_dsf
{

    struct RewriteReport
    {
        std::uint64_t source_size = 0;
        std::uint64_t output_size = 0;
        bool          md5_unchanged = false; ///< true iff recomputed digest == stored digest
        std::array<std::uint8_t, 16> stored{};
        std::array<std::uint8_t, 16> computed{};
    };

    /// Rewrite `src` into `dst`. In identity mode (default) the payload
    /// between the header and the MD5 footer is copied verbatim; only
    /// the footer is recomputed. Returns an error if `src` lacks a valid
    /// cookie / version or is too short.
    [[nodiscard]] std::expected<RewriteReport, std::string>
    rewrite_dsf_identity(const std::filesystem::path &src,
                         const std::filesystem::path &dst);

} // namespace xps::io_dsf
