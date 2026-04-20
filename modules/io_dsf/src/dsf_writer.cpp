#include "xpscenery/io_dsf/dsf_writer.hpp"
#include "xpscenery/io_dsf/dsf_header.hpp"
#include "xpscenery/io_dsf/dsf_md5.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <cstring>
#include <format>

namespace xps::io_dsf
{

    std::expected<RewriteReport, std::string>
    rewrite_dsf_identity(const std::filesystem::path &src,
                         const std::filesystem::path &dst)
    {
        auto hdr = read_header(src);
        if (!hdr) return std::unexpected(std::move(hdr.error()));

        auto bytes = xps::io_filesystem::read_binary(src);
        if (!bytes) return std::unexpected(std::move(bytes.error()));

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
        rep.computed       = computed;
        rep.md5_unchanged  = (computed == rep.stored);

        // Assemble output = original[0 .. size-16) + computed digest.
        std::vector<std::byte> out;
        out.reserve(bytes->size());
        out.insert(out.end(), body.begin(), body.end());
        for (auto b : computed)
            out.push_back(static_cast<std::byte>(b));

        rep.output_size = out.size();

        auto wr = xps::io_filesystem::write_binary_atomic(dst, out);
        if (!wr) return std::unexpected(std::move(wr.error()));

        return rep;
    }

} // namespace xps::io_dsf
