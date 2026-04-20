# ------------------------------------------------------------------
# xpscenery-ui — Qt 6 desktop application
# ------------------------------------------------------------------

find_package(Qt6 6.9 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Quick
    QuickControls2
    Qml
)
# Optional Qt modules (uncomment when needed):
# find_package(Qt6 6.9 REQUIRED COMPONENTS Quick3D Location)

# Use Qt6::Core, Qt6::Gui etc. as targets
qt_standard_project_setup(REQUIRES 6.9)

qt_add_executable(xpscenery-ui
    src/main.cpp
)

# QML module — holds all .qml files and registers them with the Qt resource system
qt_add_qml_module(xpscenery-ui
    URI XpScenery
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/MapPlaceholder.qml
        qml/StatusBar.qml
    RESOURCES
        qml/style/theme.js
)

target_link_libraries(xpscenery-ui
    PRIVATE
        xps::core
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Qml
)

set_target_properties(xpscenery-ui PROPERTIES
    FOLDER               "ui"
    WIN32_EXECUTABLE     ON
    MACOSX_BUNDLE        ON
    OUTPUT_NAME          "xpscenery"
)

install(TARGETS xpscenery-ui
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Deploy Qt DLLs alongside the exe (Windows only, release builds)
if(WIN32 AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    qt_generate_deploy_qml_app_script(
        TARGET xpscenery-ui
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()
