# ------------------------------------------------------------------
# xpscenery-ui — Qt 6 desktop application (Fáze 2A MVP, Qt Widgets)
# ------------------------------------------------------------------
find_package(Qt6 6.9 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup(REQUIRES 6.9)

set(XPS_UI_SOURCES
    src/main.cpp
    src/main_window.cpp
    src/main_window.hpp
    src/dsf_inspector_view.cpp
    src/dsf_inspector_view.hpp
    src/raster_viewer_view.cpp
    src/raster_viewer_view.hpp
    src/obj_viewer_view.cpp
    src/obj_viewer_view.hpp
    src/project_view.cpp
    src/project_view.hpp
)

qt_add_executable(xpscenery-ui ${XPS_UI_SOURCES})

set_target_properties(xpscenery-ui PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)

target_include_directories(xpscenery-ui PRIVATE src)

target_link_libraries(xpscenery-ui
    PRIVATE
        xps::core
        xps::core_types
        xps::io_logging
        xps::io_filesystem
        xps::io_config
        xps::io_dsf
        xps::io_raster
        xps::io_obj
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
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
    qt_generate_deploy_app_script(
        TARGET xpscenery-ui
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()
