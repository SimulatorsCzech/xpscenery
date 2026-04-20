#include "xpscenery/io_osm/osm_detect.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstring>
#include <format>
#include <fstream>
#include <string_view>

namespace xps::io_osm
{

    namespace
    {

        constexpr std::size_t kProbeBytes = 512;

        /// OSM PBF starts with a big-endian uint32 length-of-BlobHeader,
        /// followed by the BlobHeader protobuf. The first field of BlobHeader
        /// is `type` (string, tag=1, wire=2). The canonical value for the very
        /// first BlobHeader is "OSMHeader" — so we look for either that
        /// literal or for a plausible size prefix followed by the protobuf tag.
        bool looks_like_pbf(const std::uint8_t *p, std::size_t n,
                            std::uint32_t &len_out) noexcept
        {
            if (n < 8)
                return false;

            // Big-endian u32 length of BlobHeader. Real-world values: 10–100.
            std::uint32_t be;
            std::memcpy(&be, p, sizeof(be));
            const std::uint32_t len =
                (std::endian::native == std::endian::little) ? std::byteswap(be) : be;

            if (len == 0 || len > 64u * 1024u)
            {
                return false; // absurd BlobHeader length
            }
            if (4u + len > n)
            {
                // Can't see inside the BlobHeader; fall back to the string search.
                const std::string_view hay(reinterpret_cast<const char *>(p),
                                           std::min(n, kProbeBytes));
                if (hay.find("OSMHeader") != std::string_view::npos)
                {
                    len_out = len;
                    return true;
                }
                return false;
            }

            // Inside the BlobHeader: field 1 (type), wire type 2 (length-delim).
            // Proto tag byte = (1 << 3) | 2 = 0x0A.
            if (p[4] != 0x0A)
                return false;
            if (5u >= n)
                return false;

            const std::uint8_t name_len = p[5];
            if (name_len == 0 || static_cast<std::size_t>(6 + name_len) > n)
            {
                return false;
            }

            const std::string_view name(reinterpret_cast<const char *>(p + 6), name_len);
            if (name == "OSMHeader" || name == "OSMData")
            {
                len_out = len;
                return true;
            }
            return false;
        }

        bool looks_like_osm_xml(const std::uint8_t *p, std::size_t n) noexcept
        {
            const std::string_view hay(reinterpret_cast<const char *>(p),
                                       std::min(n, kProbeBytes));
            // Must contain an XML prolog or <osm tag close to the start.
            const auto xml_decl = hay.find("<?xml");
            const auto osm_tag = hay.find("<osm");
            if (xml_decl == std::string_view::npos && osm_tag == std::string_view::npos)
            {
                return false;
            }
            return osm_tag != std::string_view::npos;
        }

    } // namespace

    OsmInfo detect_osm_bytes(const std::uint8_t *data, std::size_t size) noexcept
    {
        OsmInfo info;
        if (data == nullptr || size == 0)
        {
            return info;
        }

        std::uint32_t len = 0;
        if (looks_like_pbf(data, size, len))
        {
            info.kind = OsmKind::pbf;
            info.first_blob_header_len = len;
            return info;
        }

        if (looks_like_osm_xml(data, size))
        {
            info.kind = OsmKind::xml;
            return info;
        }

        return info;
    }

    std::expected<OsmInfo, std::string>
    detect_osm(const std::filesystem::path &path)
    {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec))
        {
            return std::unexpected(std::format("file not found: {}", path.string()));
        }

        std::ifstream in(path, std::ios::binary);
        if (!in)
        {
            return std::unexpected(std::format("cannot open: {}", path.string()));
        }

        std::array<std::uint8_t, kProbeBytes> probe{};
        in.read(reinterpret_cast<char *>(probe.data()),
                static_cast<std::streamsize>(probe.size()));
        const auto got = static_cast<std::size_t>(in.gcount());

        return detect_osm_bytes(probe.data(), got);
    }

} // namespace xps::io_osm
