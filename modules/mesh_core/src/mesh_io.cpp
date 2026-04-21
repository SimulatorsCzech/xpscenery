// xpscenery — mesh_core/mesh_io.cpp
#include "xpscenery/mesh_core/mesh_io.hpp"

#include <format>
#include <fstream>

namespace xps::mesh_core
{

    std::expected<void, std::string>
    write_obj(const std::filesystem::path &path, const TriangleMesh &mesh)
    {
        std::ofstream f(path, std::ios::binary);
        if (!f)
            return std::unexpected(std::format(
                "mesh_io: cannot open '{}' for writing", path.string()));

        f << "# xpscenery mesh_core — Wavefront OBJ export\n";
        f << "# vertices: " << mesh.vertex_count()
          << " triangles: "  << mesh.triangle_count() << "\n";

        for (const auto &v : mesh.vertices())
            f << std::format("v {:.17g} {:.17g} {:.17g}\n", v.x, v.y, v.z);

        for (const auto &t : mesh.triangles())
        {
            // OBJ uses 1-based indices.
            f << std::format("f {} {} {}\n",
                             t.v0 + 1, t.v1 + 1, t.v2 + 1);
        }

        if (!f.good())
            return std::unexpected(std::format(
                "mesh_io: I/O error writing '{}'", path.string()));
        return {};
    }

    std::expected<void, std::string>
    write_ply_ascii(const std::filesystem::path &path, const TriangleMesh &mesh)
    {
        std::ofstream f(path, std::ios::binary);
        if (!f)
            return std::unexpected(std::format(
                "mesh_io: cannot open '{}' for writing", path.string()));

        f << "ply\nformat ascii 1.0\n";
        f << "comment xpscenery mesh_core export\n";
        f << "element vertex "  << mesh.vertex_count()   << "\n";
        f << "property double x\nproperty double y\nproperty double z\n";
        f << "element face "    << mesh.triangle_count() << "\n";
        f << "property list uchar int vertex_indices\n";
        f << "end_header\n";

        for (const auto &v : mesh.vertices())
            f << std::format("{:.17g} {:.17g} {:.17g}\n", v.x, v.y, v.z);
        for (const auto &t : mesh.triangles())
            f << std::format("3 {} {} {}\n", t.v0, t.v1, t.v2);

        if (!f.good())
            return std::unexpected(std::format(
                "mesh_io: I/O error writing '{}'", path.string()));
        return {};
    }

} // namespace xps::mesh_core
