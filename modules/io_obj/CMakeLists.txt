# modules/io_obj — X-Plane .obj (OBJ8) text format reader.

add_library(xps_io_obj STATIC
    src/obj_info.cpp
)
add_library(xps::io_obj ALIAS xps_io_obj)

target_include_directories(xps_io_obj
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(xps_io_obj
    PUBLIC
        xps::core
        xps::io_filesystem
)

target_compile_definitions(xps_io_obj PUBLIC XPS_STATIC=1)

set_target_properties(xps_io_obj PROPERTIES FOLDER "xpscenery/modules")
