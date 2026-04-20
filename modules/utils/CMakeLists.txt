# ------------------------------------------------------------------
# xps_utils — port of xptools260/src/Utils (~74k LOC)
#
# Phase 1, step 1: scaffold only. Actual source files will be
# incrementally brought over from xptools260/src/Utils in
# separate PRs. Target depends only on STL + fmt + spdlog.
# ------------------------------------------------------------------

add_library(xps_utils STATIC
    src/placeholder.cpp
)
add_library(xps::utils ALIAS xps_utils)

find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

target_include_directories(xps_utils
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(xps_utils
    PUBLIC
        xps::core
    PRIVATE
        fmt::fmt
        spdlog::spdlog
)

target_compile_definitions(xps_utils
    PUBLIC
        XPS_STATIC=1
)

set_target_properties(xps_utils PROPERTIES
    FOLDER "xpscenery/modules"
)

# The module will grow through sub-groups mirroring xptools260/src/Utils:
#   FileUtils, XMLObject, ArgumentParser, MathUtils, GISUtils,
#   CompGeomUtils, hl_types, ConfigReader.
# Each sub-group lands as a separate set of *.cpp/*.h files and a
# corresponding catch2 test in tests/unit/utils/.
