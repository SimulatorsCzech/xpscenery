#include "xpscenery/io_dsf/dsf_header.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <array>
#include <bit>
#include <cstring>
#include <format>

namespace xps::io_dsf
{

    namespace
    {

        constexpr std::array<char, 8> kDsfCookie = {'X', 'P', 'L', 'N', 'E', 'D', 'S', 'F'};
        constexpr std::uint64_t kHeaderBytes = 12; // cookie(8) + version(4)
        constexpr std::uint64_t kFooterBytes = 16; // MD5 signature
        constexpr std::int32_t kDsfVersion = 1;

    } // namespace

    std::expected<DsfHeaderInfo, std::string>
    read_header(const std::filesystem::path &dsf_path)
    {
        auto bytes = xps::io_filesystem::read_binary(dsf_path);
        if (!bytes)
            return std::unexpected(std::move(bytes.error()));

        const auto file_size = static_cast<std::uint64_t>(bytes->size());
        if (file_size < kHeaderBytes + kFooterBytes)
        {
            return std::unexpected(std::format(
                "DSF file too small ({} bytes, need at least {}): {}",
                file_size, kHeaderBytes + kFooterBytes, dsf_path.string()));
        }

        // Cookie
        for (std::size_t i = 0; i < kDsfCookie.size(); ++i)
        {
            if (std::to_integer<char>((*bytes)[i]) != kDsfCookie[i])
            {
                return std::unexpected(std::format(
                    "not a DSF file (bad cookie) in {}", dsf_path.string()));
            }
        }

        // Version (little-endian int32)
        std::int32_t version = 0;
        std::memcpy(&version, bytes->data() + 8, sizeof(version));
        if constexpr (std::endian::native == std::endian::big)
        {
            version = static_cast<std::int32_t>(
                std::byteswap(static_cast<std::uint32_t>(version)));
        }
        if (version != kDsfVersion)
        {
            return std::unexpected(std::format(
                "unsupported DSF version {} (expected {}) in {}",
                version, kDsfVersion, dsf_path.string()));
        }

        DsfHeaderInfo info;
        info.version = version;
        info.file_size = file_size;
        info.atoms_offset = kHeaderBytes;
        info.atoms_end = file_size - kFooterBytes;
        return info;
    }

    bool looks_like_dsf(const std::filesystem::path &path) noexcept
    {
        std::error_code ec;
        if (!std::filesystem::is_regular_file(path, ec) || ec)
            return false;
        auto sz = std::filesystem::file_size(path, ec);
        if (ec || sz < kDsfCookie.size())
            return false;

        // Read just the first 8 bytes via our file_io (simple path).
        auto bytes = xps::io_filesystem::read_binary(path);
        if (!bytes || bytes->size() < kDsfCookie.size())
            return false;
        for (std::size_t i = 0; i < kDsfCookie.size(); ++i)
        {
            if (std::to_integer<char>((*bytes)[i]) != kDsfCookie[i])
                return false;
        }
        return true;
    }

} // namespace xps::io_dsf
