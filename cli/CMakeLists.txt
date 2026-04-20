# ------------------------------------------------------------------
# xpscenery-cli — command-line runner
# ------------------------------------------------------------------

add_executable(xpscenery-cli
    src/main.cpp
)

target_link_libraries(xpscenery-cli
    PRIVATE
        xps::core
)

set_target_properties(xpscenery-cli PROPERTIES
    FOLDER "cli"
    OUTPUT_NAME "xpscenery-cli"
)

install(TARGETS xpscenery-cli
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
