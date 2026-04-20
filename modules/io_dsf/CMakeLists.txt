# ------------------------------------------------------------------
# modules/io_dsf — minimal reader for X-Plane DSF file format.
#
# Phase 1B scope: detect DSF magic, read header, enumerate top-level
# atoms (id + size). Deep atom parsing (terrain, objects, pools,
# commands, rasters) is Phase 2 material.
# ------------------------------------------------------------------

add_library(xps_io_dsf STATIC
    src/dsf_header.cpp
    src/dsf_atoms.cpp
    src/dsf_strings.cpp
    src/dsf_md5.cpp
    src/dsf_raster.cpp
    src/dsf_pool.cpp
    src/dsf_cmds.cpp
)
add_library(xps::io_dsf ALIAS xps_io_dsf)

target_include_directories(xps_io_dsf
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(xps_io_dsf
    PUBLIC
        xps::core
        xps::core_types
        xps::io_filesystem
)

target_compile_definitions(xps_io_dsf PUBLIC XPS_STATIC=1)

set_target_properties(xps_io_dsf PROPERTIES FOLDER "xpscenery/modules")
