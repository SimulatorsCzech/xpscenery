// xpscenery — io_vector/shp_header.cpp  (Phase 2 minimal)
#include "xpscenery/io_vector/shp_header.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <bit>
#include <cstring>
#include <format>
#include <fstream>

namespace xps::io_vector
{

    std::string shape_type_name(std::int32_t raw) noexcept
    {
        switch (static_cast<ShapeType>(raw))
        {
        case ShapeType::null_shape: return "null";
        case ShapeType::point:      return "point";
        case ShapeType::polyline:   return "polyline";
        case ShapeType::polygon:    return "polygon";
        case ShapeType::multipoint: return "multipoint";
        case ShapeType::point_z:    return "point (Z)";
        case ShapeType::polyline_z: return "polyline (Z)";
        case ShapeType::polygon_z:  return "polygon (Z)";
        case ShapeType::multipoint_z: return "multipoint (Z)";
        case ShapeType::point_m:    return "point (M)";
        case ShapeType::polyline_m: return "polyline (M)";
        case ShapeType::polygon_m:  return "polygon (M)";
        case ShapeType::multipoint_m: return "multipoint (M)";
        case ShapeType::multipatch: return "multipatch";
        }
        return std::format("unknown({})", raw);
    }

    namespace
    {
        // Little-endian load (source bytes already in LE storage).
        template <typename T>
        T load_le(const std::byte *p) noexcept
        {
            T v{};
            std::memcpy(&v, p, sizeof(T));
            if constexpr (std::endian::native == std::endian::big
                          && sizeof(T) > 1)
            {
                auto *b = reinterpret_cast<unsigned char *>(&v);
                std::reverse(b, b + sizeof(T));
            }
            return v;
        }

        std::int32_t load_be_i32(const std::byte *p) noexcept
        {
            std::uint32_t u = 0;
            const auto *b = reinterpret_cast<const unsigned char *>(p);
            u |= static_cast<std::uint32_t>(b[0]) << 24;
            u |= static_cast<std::uint32_t>(b[1]) << 16;
            u |= static_cast<std::uint32_t>(b[2]) << 8;
            u |= static_cast<std::uint32_t>(b[3]);
            return static_cast<std::int32_t>(u);
        }
    } // namespace

    std::expected<ShpHeader, std::string>
    read_shp_header(const std::filesystem::path &path)
    {
        // Read just the first 100 bytes — shapefile header is fixed-size.
        std::ifstream f(path, std::ios::binary);
        if (!f)
        {
            return std::unexpected(std::format(
                "io_vector: cannot open '{}'", path.string()));
        }
        std::byte buf[100];
        f.read(reinterpret_cast<char *>(buf), 100);
        if (f.gcount() < 100)
        {
            return std::unexpected(std::format(
                "io_vector: '{}' too short for .shp header ({} bytes)",
                path.string(), f.gcount()));
        }

        ShpHeader h;
        h.file_code = load_be_i32(buf + 0);
        if (h.file_code != 9994)
        {
            return std::unexpected(std::format(
                "io_vector: '{}' is not a Shapefile (file_code {}≠9994)",
                path.string(), h.file_code));
        }

        const std::int32_t len_words = load_be_i32(buf + 24);
        h.file_length_bytes = static_cast<std::uint64_t>(
            static_cast<std::uint32_t>(len_words)) * 2ull;

        h.version        = load_le<std::int32_t>(buf + 28);
        h.shape_type_raw = load_le<std::int32_t>(buf + 32);

        h.x_min = load_le<double>(buf + 36);
        h.y_min = load_le<double>(buf + 44);
        h.x_max = load_le<double>(buf + 52);
        h.y_max = load_le<double>(buf + 60);
        h.z_min = load_le<double>(buf + 68);
        h.z_max = load_le<double>(buf + 76);
        h.m_min = load_le<double>(buf + 84);
        h.m_max = load_le<double>(buf + 92);

        return h;
    }

} // namespace xps::io_vector
