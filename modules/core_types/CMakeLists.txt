# ------------------------------------------------------------------
# modules/core_types — pure domain types (header-only + thin .cpp).
# No external dependencies beyond the C++23 standard library.
# ------------------------------------------------------------------

add_library(xps_core_types STATIC
    src/lat_lon.cpp
    src/tile_coord.cpp
    src/bounding_box.cpp
    src/version_info.cpp
)
add_library(xps::core_types ALIAS xps_core_types)

target_include_directories(xps_core_types
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(xps_core_types
    PUBLIC
        xps::core
)

target_compile_definitions(xps_core_types PUBLIC XPS_STATIC=1)

set_target_properties(xps_core_types PROPERTIES FOLDER "xpscenery/modules")
