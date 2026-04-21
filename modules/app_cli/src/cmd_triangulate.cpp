// xpscenery-cli triangulate — 2D Delaunay triangulace bodů ze souboru.
//
// Vstupní formát: prostý text, jeden bod na řádek, `x y [z]` oddělené
// mezerou. Řádky začínající # jsou komentáře. Výstup: OBJ nebo PLY
// soubor s triangulovanou sítí.
//
// Typické použití:
//   xpscenery-cli triangulate pts.txt --out tri.obj
//   xpscenery-cli triangulate pts.txt --out tri.ply --format ply

#include "commands.hpp"

#include "xpscenery/mesh_core/delaunay.hpp"
#include "xpscenery/mesh_core/mesh_io.hpp"
#include "xpscenery/mesh_core/point.hpp"

#include <CLI/CLI.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <print>
#include <sstream>
#include <string>
#include <vector>

namespace xps::app_cli::detail
{

    void register_triangulate(CLI::App &root)
    {
        auto *cmd = root.add_subcommand(
            "triangulate",
            "Delaunay triangulace 2D bodů (Bowyer-Watson, pure C++). "
            "Vstup: text 'x y [z]' na řádek; výstup: OBJ nebo PLY.");

        static std::string src_arg;
        static std::string out_arg;
        static std::string format_arg = "obj";
        static bool as_json = false;

        cmd->add_option("src", src_arg, "Input points file (text)")->required();
        cmd->add_option("--out", out_arg, "Output mesh file")->required();
        cmd->add_option("--format", format_arg, "obj | ply")
            ->check(CLI::IsMember({"obj", "ply"}));
        cmd->add_flag("--json", as_json, "Emit machine-readable JSON summary");

        cmd->callback([]
                      {
            std::ifstream in(src_arg);
            if (!in) {
                std::fprintf(stderr, "triangulate: cannot open '%s'\n", src_arg.c_str());
                std::exit(1);
            }
            std::vector<xps::mesh_core::Point3> pts;
            std::string line;
            std::size_t line_no = 0;
            while (std::getline(in, line))
            {
                ++line_no;
                // Strip trailing \r.
                if (!line.empty() && line.back() == '\r') line.pop_back();
                // Trim leading whitespace for comment detection.
                std::size_t i = 0;
                while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
                if (i >= line.size() || line[i] == '#') continue;
                std::istringstream ss(line.substr(i));
                double x = 0, y = 0, z = 0;
                if (!(ss >> x >> y)) {
                    std::fprintf(stderr,
                                 "triangulate: syntax error na řádku %zu\n",
                                 line_no);
                    std::exit(2);
                }
                ss >> z; // volitelné
                pts.push_back({x, y, z});
            }

            auto r = xps::mesh_core::delaunay_triangulate_2d(
                std::span<const xps::mesh_core::Point3>{pts});
            if (!r) {
                std::fprintf(stderr, "triangulate: %s\n", r.error().c_str());
                std::exit(3);
            }
            const auto &mesh = *r;

            std::expected<void, std::string> wres;
            namespace fs = std::filesystem;
            if (format_arg == "ply")
                wres = xps::mesh_core::write_ply_ascii(fs::path{out_arg}, mesh);
            else
                wres = xps::mesh_core::write_obj(fs::path{out_arg}, mesh);
            if (!wres) {
                std::fprintf(stderr, "triangulate: %s\n", wres.error().c_str());
                std::exit(4);
            }

            if (as_json) {
                std::println(
                    R"({{"source":"{}","output":"{}","format":"{}",)"
                    R"("vertices":{},"triangles":{}}})",
                    src_arg, out_arg, format_arg,
                    mesh.vertex_count(), mesh.triangle_count());
            } else {
                std::println("triangulate: {} → {} ({} vrcholů, {} trojúhelníků, {})",
                             src_arg, out_arg,
                             mesh.vertex_count(), mesh.triangle_count(),
                             format_arg);
            }
        });
    }

} // namespace xps::app_cli::detail
