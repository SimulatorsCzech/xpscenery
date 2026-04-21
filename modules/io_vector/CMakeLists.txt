# ------------------------------------------------------------------
# modules/io_vector — minimal vector format (Shapefile) support.
#
# Phase 2 scope: ESRI Shapefile (.shp) header reader (100-byte header)
# per ESRI whitepaper. Parses file length (words), version, shape type,
# and the overall X/Y/Z/M bounding box. Deep record parsing (per-shape
# geometry) is future work.
# ------------------------------------------------------------------

add_library(xps_io_vector STATIC
    src/shp_header.cpp
)
add_library(xps::io_vector ALIAS xps_io_vector)

target_include_directories(xps_io_vector
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(xps_io_vector
    PUBLIC
        xps::core
        xps::io_filesystem
)

target_compile_definitions(xps_io_vector PUBLIC XPS_STATIC=1)

set_target_properties(xps_io_vector PROPERTIES FOLDER "xpscenery/modules")
