#include "commands.hpp"

#include "xpscenery/io_filesystem/paths.hpp"
#include "xpscenery/io_raster/geotiff_ifd.hpp"
#include "xpscenery/io_raster/tiff_detect.hpp"

#include <CLI/CLI.hpp>

#include <cstdio>
#include <filesystem>
#include <print>
#include <string>

namespace xps::app_cli::detail
{

    void register_raster_info(CLI::App &root)
    {
        auto *cmd = root.add_subcommand(
            "raster-info",
            "Dump TIFF / GeoTIFF first-IFD summary (width, height, "
            "bits/sample, pixel scale, tiepoints, GeoKey count).");

        static std::string path_arg;
        cmd->add_option("path", path_arg, "Path to a .tif/.tiff file")->required();

        cmd->callback([]
                      {
            namespace fs = std::filesystem;
            auto resolved = xps::io_filesystem::require_existing_file(fs::path{path_arg});
            if (!resolved) {
                std::fprintf(stderr, "raster-info: %s\n", resolved.error().c_str());
                std::exit(1);
            }
            auto det = xps::io_raster::detect_tiff(*resolved);
            if (!det) {
                std::fprintf(stderr, "raster-info: %s\n", det.error().c_str());
                std::exit(2);
            }
            if (det->kind == xps::io_raster::TiffKind::none) {
                std::fprintf(stderr, "raster-info: %s is not a TIFF file\n",
                             resolved->string().c_str());
                std::exit(3);
            }
            std::println("TIFF  : {}", resolved->string());
            std::println("  byte-order    : {}",
                         det->little_endian ? "little-endian (II)" : "big-endian (MM)");
            std::println("  kind          : {}",
                         det->is_bigtiff ? "BigTIFF" : "classic TIFF");
            std::println("  first-IFD     : {}", det->first_ifd_offset);

            auto ifd = xps::io_raster::read_geotiff_ifd(*resolved);
            if (!ifd) {
                std::fprintf(stderr, "raster-info: %s\n", ifd.error().c_str());
                std::exit(4);
            }
            std::println("  size          : {} x {}", ifd->width, ifd->height);
            std::println("  bits/sample   : {}", ifd->bits_per_sample);
            std::println("  samples/pixel : {}", ifd->samples_per_pixel);
            std::println("  photometric   : {}", ifd->photometric);
            std::println("  compression   : {}", ifd->compression);
            std::println("  sample-format : {}", ifd->sample_format);
            std::println("  strips        : {}", ifd->strip_count);
            if (ifd->have_pixel_scale) {
                std::println("  pixel-scale   : {} x {} (z={})",
                             ifd->pixel_scale_x, ifd->pixel_scale_y,
                             ifd->pixel_scale_z);
            }
            if (!ifd->tiepoints.empty()) {
                std::println("  tiepoints ({}):", ifd->tiepoints.size());
                for (std::size_t i = 0; i < ifd->tiepoints.size(); ++i) {
                    const auto& tp = ifd->tiepoints[i];
                    std::println("    [{}] raster=({}, {}, {}) world=({}, {}, {})",
                                 i, tp.i, tp.j, tp.k, tp.x, tp.y, tp.z);
                }
            }
            if (ifd->has_geokeys()) {
                std::println("  geo-keys      : {} (rev {}.{})",
                             ifd->geo_keys.size(),
                             ifd->geo_key_revision,
                             ifd->geo_key_minor_revision);
            } else if (!ifd->is_georeferenced()) {
                std::println("  georeferenced : no");
            } });
    }

} // namespace xps::app_cli::detail
