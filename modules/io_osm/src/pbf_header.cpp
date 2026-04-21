// xpscenery — io_osm/pbf_header.cpp  (Phase 2 minimal)
#include "xpscenery/io_osm/pbf_header.hpp"

#include <cstring>
#include <format>
#include <fstream>

namespace xps::io_osm
{

    namespace
    {
        // Varint decoder (returns value + bytes consumed; or 0 count on
        // malformed). Max 10 bytes for 64-bit.
        struct Varint
        {
            std::uint64_t value = 0;
            std::size_t   bytes = 0;
        };
        Varint read_varint(const std::uint8_t *p, std::size_t avail) noexcept
        {
            Varint out;
            std::uint64_t v = 0;
            unsigned shift = 0;
            for (std::size_t i = 0; i < avail && i < 10; ++i)
            {
                const std::uint8_t b = p[i];
                v |= static_cast<std::uint64_t>(b & 0x7F) << shift;
                if ((b & 0x80) == 0)
                {
                    out.value = v;
                    out.bytes = i + 1;
                    return out;
                }
                shift += 7;
            }
            return out; // bytes == 0 → error
        }
    } // namespace

    std::expected<PbfHeaderInfo, std::string>
    read_pbf_header(const std::filesystem::path &path)
    {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f)
            return std::unexpected(std::format(
                "pbf_header: cannot open '{}'", path.string()));
        const std::uint64_t fsize =
            static_cast<std::uint64_t>(f.tellg());
        f.seekg(0);

        // 1. BE uint32 header length.
        std::uint8_t lenb[4];
        f.read(reinterpret_cast<char *>(lenb), 4);
        if (f.gcount() < 4)
            return std::unexpected("pbf_header: file too short for header length");
        const std::uint32_t header_len =
            (static_cast<std::uint32_t>(lenb[0]) << 24) |
            (static_cast<std::uint32_t>(lenb[1]) << 16) |
            (static_cast<std::uint32_t>(lenb[2]) << 8)  |
            (static_cast<std::uint32_t>(lenb[3]));
        // Sanity: BlobHeader is tiny — refuse absurd lengths.
        if (header_len == 0 || header_len > 64 * 1024)
            return std::unexpected(std::format(
                "pbf_header: nepřijatelná délka prvního BlobHeader ({})",
                header_len));

        std::string buf;
        buf.resize(header_len);
        f.read(buf.data(), header_len);
        if (static_cast<std::uint32_t>(f.gcount()) < header_len)
            return std::unexpected("pbf_header: krátké čtení BlobHeader");

        PbfHeaderInfo out;
        out.file_size = fsize;
        out.first_blob_header_len = header_len;

        // 2. Parse protobuf BlobHeader: hledáme pole 1 (string) a pole 3 (int32).
        const auto *p   = reinterpret_cast<const std::uint8_t *>(buf.data());
        std::size_t pos = 0;
        const std::size_t end = buf.size();
        while (pos < end)
        {
            const auto key = read_varint(p + pos, end - pos);
            if (key.bytes == 0)
                return std::unexpected("pbf_header: malformed varint (tag)");
            pos += key.bytes;
            const std::uint32_t field = static_cast<std::uint32_t>(key.value >> 3);
            const std::uint32_t wire  = static_cast<std::uint32_t>(key.value & 0x7);
            if (wire == 2) // length-delimited
            {
                const auto len = read_varint(p + pos, end - pos);
                if (len.bytes == 0)
                    return std::unexpected("pbf_header: malformed varint (length)");
                pos += len.bytes;
                if (pos + len.value > end)
                    return std::unexpected("pbf_header: length exceeds buffer");
                if (field == 1)
                {
                    out.first_blob_type.assign(
                        reinterpret_cast<const char *>(p + pos),
                        static_cast<std::size_t>(len.value));
                }
                pos += static_cast<std::size_t>(len.value);
            }
            else if (wire == 0) // varint
            {
                const auto v = read_varint(p + pos, end - pos);
                if (v.bytes == 0)
                    return std::unexpected("pbf_header: malformed varint (value)");
                pos += v.bytes;
                if (field == 3)
                    out.first_blob_datasize = static_cast<std::int32_t>(v.value);
            }
            else if (wire == 1) pos += 8;   // fixed64
            else if (wire == 5) pos += 4;   // fixed32
            else return std::unexpected(std::format(
                "pbf_header: nepodporovaný wire type {}", wire));
        }
        if (out.first_blob_type.empty())
            return std::unexpected("pbf_header: BlobHeader bez povinného pole type");

        return out;
    }

} // namespace xps::io_osm
