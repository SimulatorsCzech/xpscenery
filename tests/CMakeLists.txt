# ------------------------------------------------------------------
# xpscenery — test suite
# ------------------------------------------------------------------

find_package(Catch2 3 CONFIG REQUIRED)

add_executable(xps_tests
    unit/test_version.cpp
    unit/test_smoke.cpp
)

target_link_libraries(xps_tests
    PRIVATE
        xps::core
        Catch2::Catch2WithMain
)

set_target_properties(xps_tests PROPERTIES
    FOLDER "tests"
)

include(Catch)
catch_discover_tests(xps_tests)
