# ------------------------------------------------------------------
# xpscenery — test suite
# ------------------------------------------------------------------

find_package(Catch2 3 CONFIG REQUIRED)

add_executable(xps_tests
    unit/test_version.cpp
    unit/test_smoke.cpp
    unit/core_types/test_lat_lon.cpp
    unit/core_types/test_tile_coord.cpp
    unit/core_types/test_bounding_box.cpp
    unit/io_filesystem/test_paths.cpp
    unit/io_filesystem/test_file_io.cpp
    unit/io_config/test_tile_config.cpp
)

target_link_libraries(xps_tests
    PRIVATE
        xps::core
        xps::core_types
        xps::io_logging
        xps::io_filesystem
        xps::io_config
        Catch2::Catch2WithMain
)

set_target_properties(xps_tests PROPERTIES
    FOLDER "tests"
)

include(Catch)
catch_discover_tests(xps_tests)
