# ------------------------------------------------------------------
# modules/io_osm — minimal OpenStreetMap file format detection.
#
# Phase 1B scope: byte-level probe to differentiate OSM PBF
# (protobuf framed) from OSM XML (<?xml + <osm ...>) from unknown.
# Deep parsing (node/way/relation, zlib-decompressed blobs) is Phase 2.
# ------------------------------------------------------------------

add_library(xps_io_osm STATIC
    src/osm_detect.cpp
)
add_library(xps::io_osm ALIAS xps_io_osm)

target_include_directories(xps_io_osm
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(xps_io_osm
    PUBLIC
        xps::core
        xps::io_filesystem
)

target_compile_definitions(xps_io_osm PUBLIC XPS_STATIC=1)

set_target_properties(xps_io_osm PROPERTIES FOLDER "xpscenery/modules")
