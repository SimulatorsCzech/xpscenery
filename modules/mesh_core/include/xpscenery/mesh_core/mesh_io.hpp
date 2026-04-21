#pragma once
// xpscenery — mesh_core/mesh_io.hpp
//
// Wavefront .obj a Stanford .ply exporter. Ne import — ten přijde až
// bude potřeba (mesh_core primárně generuje, nečte cizí meshe).

#include "triangle_mesh.hpp"

#include <expected>
#include <filesystem>
#include <string>

namespace xps::mesh_core
{

    /// Wavefront .obj (ASCII): `v x y z` pro každý vrchol, pak `f a b c`
    /// s 1-indexováním (dle specifikace OBJ).
    [[nodiscard]] std::expected<void, std::string>
    write_obj(const std::filesystem::path &path, const TriangleMesh &mesh);

    /// Stanford .ply (ASCII). Využije se hlavně pro interop s MeshLab /
    /// CloudCompare při verifikaci.
    [[nodiscard]] std::expected<void, std::string>
    write_ply_ascii(const std::filesystem::path &path, const TriangleMesh &mesh);

} // namespace xps::mesh_core
