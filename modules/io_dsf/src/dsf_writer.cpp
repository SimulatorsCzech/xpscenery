#include "xpscenery/io_dsf/dsf_writer.hpp"
#include "xpscenery/io_dsf/dsf_atoms.hpp"
#include "xpscenery/io_dsf/dsf_header.hpp"
#include "xpscenery/io_dsf/dsf_md5.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <bit>
#include <cstring>
#include <format>

namespace xps::io_dsf
{

    namespace
    {
        constexpr std::array<std::byte, 8> kCookie{
            std::byte{'X'}, std::byte{'P'}, std::byte{'L'}, std::byte{'N'},
            std::byte{'E'}, std::byte{'D'}, std::byte{'S'}, std::byte{'F'},
        };

        void append_u32_le(std::vector<std::byte> &out, std::uint32_t v)
        {
            if constexpr (std::endian::native == std::endian::big)
                v = std::byteswap(v);
            const auto *p = reinterpret_cast<const std::byte *>(&v);
            out.insert(out.end(), p, p + 4);
        }

        void append_i32_le(std::vector<std::byte> &out, std::int32_t v)
        {
            append_u32_le(out, static_cast<std::uint32_t>(v));
        }
    } // namespace

    std::expected<RewriteReport, std::string>
    rewrite_dsf_identity(const std::filesystem::path &src,
                         const std::filesystem::path &dst)
    {
        auto hdr = read_header(src);
        if (!hdr)
            return std::unexpected(std::move(hdr.error()));

        auto bytes = xps::io_filesystem::read_binary(src);
        if (!bytes)
            return std::unexpected(std::move(bytes.error()));

        if (bytes->size() < 12 + 16)
            return std::unexpected(std::format(
                "DSF too small for identity rewrite: {} bytes", bytes->size()));

        RewriteReport rep;
        rep.source_size = bytes->size();

        // Stored footer
        std::memcpy(rep.stored.data(),
                    bytes->data() + bytes->size() - 16, 16);

        // Compute MD5 of everything except the existing 16-byte footer.
        std::span<const std::byte> body{bytes->data(),
                                        bytes->size() - 16};
        const auto computed = Md5::of(body);
        rep.computed = computed;
        rep.md5_unchanged = (computed == rep.stored);

        // Assemble output = original[0 .. size-16) + computed digest.
        std::vector<std::byte> out;
        out.reserve(bytes->size());
        out.insert(out.end(), body.begin(), body.end());
        for (auto b : computed)
            out.push_back(static_cast<std::byte>(b));

        rep.output_size = out.size();

        auto wr = xps::io_filesystem::write_binary_atomic(dst, out);
        if (!wr)
            return std::unexpected(std::move(wr.error()));

        return rep;
    }

    // ---------------------------------------------------------------------
    // Decomposing writer
    // ---------------------------------------------------------------------

    std::expected<DsfBlob, std::string>
    read_dsf_blob(const std::filesystem::path &src)
    {
        auto hdr = read_header(src);
        if (!hdr)
            return std::unexpected(std::move(hdr.error()));

        auto atoms = read_top_level_atoms(src);
        if (!atoms)
            return std::unexpected(std::move(atoms.error()));

        auto bytes = xps::io_filesystem::read_binary(src);
        if (!bytes)
            return std::unexpected(std::move(bytes.error()));

        DsfBlob blob;
        blob.version = hdr->version;
        blob.atoms.reserve(atoms->size());
        for (const auto &a : *atoms)
        {
            if (a.offset + a.size > bytes->size())
                return std::unexpected(std::format(
                    "Atom '{}' at offset {} (size {}) exceeds file size {}",
                    a.tag, a.offset, a.size, bytes->size()));
            AtomBlob b;
            b.tag = a.tag;
            b.raw_id = a.raw_id;
            b.bytes.assign(bytes->begin() + static_cast<std::ptrdiff_t>(a.offset),
                           bytes->begin() + static_cast<std::ptrdiff_t>(a.offset + a.size));
            blob.atoms.push_back(std::move(b));
        }
        return blob;
    }

    std::expected<RewriteReport, std::string>
    write_dsf_blob(const std::filesystem::path &dst, const DsfBlob &blob)
    {
        // Validate each atom blob: must have >= 8 bytes and the embedded
        // size field must match blob.bytes.size(). This catches callers
        // that mutate payload without updating the header.
        std::uint64_t body_bytes = 12; // cookie + version
        for (std::size_t i = 0; i < blob.atoms.size(); ++i)
        {
            const auto &a = blob.atoms[i];
            if (a.bytes.size() < 8)
                return std::unexpected(std::format(
                    "Atom #{} ('{}') blob is {} bytes, < 8-byte header",
                    i, a.tag, a.bytes.size()));
            std::uint32_t declared = 0;
            std::memcpy(&declared, a.bytes.data() + 4, 4);
            if constexpr (std::endian::native == std::endian::big)
                declared = std::byteswap(declared);
            if (declared != a.bytes.size())
                return std::unexpected(std::format(
                    "Atom #{} ('{}') header declares size {}, blob has {} bytes",
                    i, a.tag, declared, a.bytes.size()));
            body_bytes += a.bytes.size();
        }

        std::vector<std::byte> out;
        out.reserve(static_cast<std::size_t>(body_bytes + 16));

        // Header: cookie + int32 version
        out.insert(out.end(), kCookie.begin(), kCookie.end());
        append_i32_le(out, blob.version);

        // Atoms (each already contains its 8-byte header)
        for (const auto &a : blob.atoms)
            out.insert(out.end(), a.bytes.begin(), a.bytes.end());

        // Footer MD5 over header + atoms
        std::span<const std::byte> body{out.data(), out.size()};
        const auto digest = Md5::of(body);
        for (auto b : digest)
            out.push_back(static_cast<std::byte>(b));

        RewriteReport rep;
        rep.source_size = body_bytes + 16; // what we *would* have read
        rep.output_size = out.size();
        rep.computed = digest;
        rep.stored = digest;
        rep.md5_unchanged = true;

        auto wr = xps::io_filesystem::write_binary_atomic(dst, out);
        if (!wr)
            return std::unexpected(std::move(wr.error()));
        return rep;
    }

    std::expected<RewriteReport, std::string>
    rewrite_dsf_decomposed(const std::filesystem::path &src,
                           const std::filesystem::path &dst)
    {
        auto blob = read_dsf_blob(src);
        if (!blob)
            return std::unexpected(std::move(blob.error()));

        // Capture stored footer *before* rewriting so the report can
        // reflect whether the original file's MD5 matched its body.
        auto src_bytes = xps::io_filesystem::read_binary(src);
        if (!src_bytes)
            return std::unexpected(std::move(src_bytes.error()));
        if (src_bytes->size() < 12 + 16)
            return std::unexpected(std::format(
                "DSF too small for decomposed rewrite: {} bytes",
                src_bytes->size()));

        std::array<std::uint8_t, 16> stored{};
        std::memcpy(stored.data(),
                    src_bytes->data() + src_bytes->size() - 16, 16);
        std::span<const std::byte> body{src_bytes->data(),
                                         src_bytes->size() - 16};
        const auto computed = Md5::of(body);

        auto wr = write_dsf_blob(dst, *blob);
        if (!wr)
            return std::unexpected(std::move(wr.error()));

        // Overwrite the blob-level report with the *source* context.
        wr->source_size = src_bytes->size();
        wr->stored = stored;
        wr->computed = computed;
        wr->md5_unchanged = (computed == stored);
        return wr;
    }

} // namespace xps::io_dsf
