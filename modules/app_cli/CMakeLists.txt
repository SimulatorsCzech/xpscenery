# modules/app_cli — CLI11 + subcommand dispatch.

find_package(CLI11 CONFIG REQUIRED)

add_library(xps_app_cli STATIC
    src/cli_app.cpp
    src/cmd_version.cpp
    src/cmd_inspect.cpp
    src/cmd_validate.cpp
)
add_library(xps::app_cli ALIAS xps_app_cli)

target_include_directories(xps_app_cli
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(xps_app_cli
    PUBLIC
        xps::core
        xps::core_types
        xps::io_logging
        xps::io_filesystem
        xps::io_config
        xps::io_dsf
        CLI11::CLI11
)

target_compile_definitions(xps_app_cli PUBLIC XPS_STATIC=1)

set_target_properties(xps_app_cli PROPERTIES FOLDER "xpscenery/modules")
