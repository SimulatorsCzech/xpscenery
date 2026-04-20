# modules/io_config — JSON tile configuration (schema-validated).

find_package(nlohmann_json CONFIG REQUIRED)

add_library(xps_io_config STATIC
    src/tile_config.cpp
)
add_library(xps::io_config ALIAS xps_io_config)

target_include_directories(xps_io_config
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(xps_io_config
    PUBLIC  xps::core xps::core_types xps::io_filesystem nlohmann_json::nlohmann_json
)

target_compile_definitions(xps_io_config PUBLIC XPS_STATIC=1)

set_target_properties(xps_io_config PROPERTIES FOLDER "xpscenery/modules")
