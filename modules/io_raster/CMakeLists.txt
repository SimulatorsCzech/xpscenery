# ------------------------------------------------------------------
# modules/io_raster — minimal raster format detection.
#
# Phase 1B scope: byte-level magic detection for GeoTIFF (TIFF II*/MM*)
# and BigTIFF. Deep parsing (IFD entries, tile/strip, pyramids, GDAL
# integration) is Phase 2 material.
# ------------------------------------------------------------------

add_library(xps_io_raster STATIC
    src/tiff_detect.cpp
    src/geotiff_ifd.cpp
)
add_library(xps::io_raster ALIAS xps_io_raster)

target_include_directories(xps_io_raster
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(xps_io_raster
    PUBLIC
        xps::core
        xps::io_filesystem
)

target_compile_definitions(xps_io_raster PUBLIC XPS_STATIC=1)

set_target_properties(xps_io_raster PROPERTIES FOLDER "xpscenery/modules")
