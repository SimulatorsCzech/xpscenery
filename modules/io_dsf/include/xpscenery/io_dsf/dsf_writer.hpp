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
        bool md5_unchanged = false; ///< true iff recomputed digest == stored digest
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

    // -----------------------------------------------------------------
    // Decomposing (atom-level) writer — v0.3.0 foundation.
    //
    // Unlike the identity path, this splits the file into individually
    // addressable top-level atom blobs. Each atom keeps its original
    // 8-byte header + payload bytes, so callers can replace, drop or
    // reorder atoms and re-emit a fully-valid DSF (correct MD5, correct
    // global size).
    //
    // The decomposed round-trip (read → write without mutation) is
    // guaranteed bit-identical on any input accepted by read_dsf_blob.
    // -----------------------------------------------------------------

    struct AtomBlob
    {
        std::string tag;              ///< 4-char human-readable, e.g. "HEAD"
        std::uint32_t raw_id = 0;     ///< original 4-byte atom id
        std::vector<std::byte> bytes; ///< full atom incl. 8-byte header
    };

    struct DsfBlob
    {
        std::int32_t version = 1;
        std::vector<AtomBlob> atoms;
    };

    /// Decompose a DSF file into header-version + per-atom byte blobs.
    [[nodiscard]] std::expected<DsfBlob, std::string>
    read_dsf_blob(const std::filesystem::path &src);

    /// Recompose a DSF from a DsfBlob and write it atomically. Computes
    /// and appends a fresh 16-byte MD5 footer over the serialised body.
    [[nodiscard]] std::expected<RewriteReport, std::string>
    write_dsf_blob(const std::filesystem::path &dst, const DsfBlob &blob);

    /// Convenience: read_dsf_blob → write_dsf_blob. On any valid input
    /// this is bit-identical to `rewrite_dsf_identity` but exercises the
    /// decompose→recompose path, which is what downstream mutators use.
    [[nodiscard]] std::expected<RewriteReport, std::string>
    rewrite_dsf_decomposed(const std::filesystem::path &src,
                           const std::filesystem::path &dst);

} // namespace xps::io_dsf
