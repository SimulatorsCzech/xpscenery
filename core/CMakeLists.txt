# ------------------------------------------------------------------
# libxpscenery — core engine
# ------------------------------------------------------------------

add_library(xpscenery_core STATIC
    src/version.cpp
    src/capi/xps_capi.cpp
)

target_include_directories(xpscenery_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(xpscenery_core
    PRIVATE
        XPS_CORE_BUILD=1
    PUBLIC
        XPS_STATIC=1
        XPS_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        XPS_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        XPS_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Third-party deps (via vcpkg) — uncomment as ported modules need them
# find_package(Boost REQUIRED COMPONENTS graph filesystem program_options)
# find_package(CGAL  REQUIRED)
# find_package(GDAL  REQUIRED)
# find_package(fmt   CONFIG REQUIRED)
# find_package(spdlog CONFIG REQUIRED)
# find_package(nlohmann_json CONFIG REQUIRED)
#
# target_link_libraries(xpscenery_core PUBLIC
#     Boost::graph Boost::filesystem
#     CGAL::CGAL GDAL::GDAL
#     fmt::fmt spdlog::spdlog nlohmann_json::nlohmann_json
# )

# Export as alias for downstream
add_library(xps::core ALIAS xpscenery_core)

set_target_properties(xpscenery_core PROPERTIES
    FOLDER "core"
)

# ------------------------------------------------------------------
# Sub-modules (will be added during Phase 1 port from xptools260)
# ------------------------------------------------------------------
# add_subdirectory(utils)     # from xptools260/src/Utils
# add_subdirectory(dsf)       # from xptools260/src/DSF
# add_subdirectory(xescore)   # from xptools260/src/XESCore
# add_subdirectory(xestools)  # from xptools260/src/XESTools
