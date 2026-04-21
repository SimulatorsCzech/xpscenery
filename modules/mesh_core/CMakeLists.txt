# ------------------------------------------------------------------
# modules/mesh_core — TIN (triangle-mesh) foundation.
#
# Fáze 3A: hand-rolled Bowyer-Watson Delaunay triangulace + mesh I/O
# (OBJ, PLY ASCII). Žádné externí závislosti — vše v čistém C++23.
# Fáze 3B: volitelný CGAL backend pro robustní predikáty, constrained
# Delaunay, Delaunay refinement, atd.
# ------------------------------------------------------------------

add_library(xps_mesh_core STATIC
    src/delaunay.cpp
    src/mesh_io.cpp
)
add_library(xps::mesh_core ALIAS xps_mesh_core)

target_include_directories(xps_mesh_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(xps_mesh_core
    PUBLIC
        xps::core
)

target_compile_definitions(xps_mesh_core PUBLIC XPS_STATIC=1)

set_target_properties(xps_mesh_core PROPERTIES FOLDER "xpscenery/modules")
