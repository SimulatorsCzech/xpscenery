# modules/io_filesystem — std::filesystem helpers + checked file IO.

add_library(xps_io_filesystem STATIC
    src/paths.cpp
    src/file_io.cpp
)
add_library(xps::io_filesystem ALIAS xps_io_filesystem)

target_include_directories(xps_io_filesystem
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(xps_io_filesystem
    PUBLIC  xps::core xps::core_types
)

target_compile_definitions(xps_io_filesystem PUBLIC XPS_STATIC=1)

set_target_properties(xps_io_filesystem PROPERTIES FOLDER "xpscenery/modules")
