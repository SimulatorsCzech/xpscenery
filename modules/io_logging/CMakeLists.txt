# modules/io_logging — spdlog wrapper (async, JSON + console, rotating).

find_package(spdlog CONFIG REQUIRED)
find_package(fmt    CONFIG REQUIRED)

add_library(xps_io_logging STATIC
    src/logger.cpp
)
add_library(xps::io_logging ALIAS xps_io_logging)

target_include_directories(xps_io_logging
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(xps_io_logging
    PUBLIC  xps::core xps::core_types spdlog::spdlog fmt::fmt
)

target_compile_definitions(xps_io_logging PUBLIC
    XPS_STATIC=1
    SPDLOG_FMT_EXTERNAL=1   # use system fmt instead of bundled
)

set_target_properties(xps_io_logging PROPERTIES FOLDER "xpscenery/modules")
