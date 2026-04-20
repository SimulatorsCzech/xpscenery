# ------------------------------------------------------------------
# modules/geodesy — WGS84 ellipsoid geodesy.
#
# Vincenty inverse formula for precise (sub-mm) distances + azimuth.
# No external deps; pure C++23 + <cmath>.
# (A GeographicLib-backed alternative may be added later as a feature.)
# ------------------------------------------------------------------

add_library(xps_geodesy STATIC
    src/vincenty.cpp
)
add_library(xps::geodesy ALIAS xps_geodesy)

target_include_directories(xps_geodesy
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(xps_geodesy
    PUBLIC
        xps::core
        xps::core_types
)

target_compile_definitions(xps_geodesy PUBLIC XPS_STATIC=1)

set_target_properties(xps_geodesy PROPERTIES FOLDER "xpscenery/modules")
