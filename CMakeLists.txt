# ------------------------------------------------------------------
# xpscenery — top-level CMake
# Requires: CMake 4.2+, MSVC 19.40+ (VS 2022 17.12 or VS 2026 Preview)
# ------------------------------------------------------------------

cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

# ------------------------------------------------------------------
# Project
# ------------------------------------------------------------------
project(xpscenery
    VERSION       0.1.0
    DESCRIPTION   "Modern Windows GIS + scenery authoring tool for X-Plane 12"
    HOMEPAGE_URL  "https://github.com/YOUR_USERNAME/xpscenery"
    LANGUAGES     C CXX
)

# ------------------------------------------------------------------
# Global settings
# ------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for clangd / IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Group folders in Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Allow multi-config (Debug/Release) — important on Windows
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

# ------------------------------------------------------------------
# Options
# ------------------------------------------------------------------
option(XPS_BUILD_CORE  "Build libxpscenery core library"   ON)
option(XPS_BUILD_CLI   "Build xpscenery-cli executable"    ON)
option(XPS_BUILD_UI    "Build Qt 6 desktop application"    OFF)
option(XPS_BUILD_TESTS "Build unit tests & regression"     ON)

option(XPS_WARNINGS_AS_ERRORS "Treat warnings as errors"    ON)
option(XPS_ENABLE_ASAN        "Enable AddressSanitizer"     OFF)
option(XPS_ENABLE_UBSAN       "Enable UndefinedBehaviorSan" OFF)
option(XPS_ENABLE_LTO         "Enable link-time optimize"   OFF)

message(STATUS "xpscenery options:")
message(STATUS "  CORE   = ${XPS_BUILD_CORE}")
message(STATUS "  CLI    = ${XPS_BUILD_CLI}")
message(STATUS "  UI     = ${XPS_BUILD_UI}")
message(STATUS "  TESTS  = ${XPS_BUILD_TESTS}")

# ------------------------------------------------------------------
# Warning flags (MSVC-focused, Clang-cl compatible)
# ------------------------------------------------------------------
if(MSVC)
    add_compile_options(
        /W4              # level 4 warnings
        /permissive-     # strict standard conformance
        /Zc:__cplusplus  # correct __cplusplus macro
        /Zc:preprocessor # standards-conformant preprocessor
        /utf-8           # source + exec charset UTF-8
        /MP              # multi-core build
        /EHsc            # standard exception handling
    )
    if(XPS_WARNINGS_AS_ERRORS)
        add_compile_options(/WX)
    endif()
    # Suppress deprecated CRT warnings — use std:: alternatives where possible
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(XPS_WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()
endif()

# ------------------------------------------------------------------
# Sanitizers
# ------------------------------------------------------------------
if(XPS_ENABLE_ASAN)
    if(MSVC)
        add_compile_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()

# ------------------------------------------------------------------
# Link-time optimization
# ------------------------------------------------------------------
if(XPS_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _ipo_ok OUTPUT _ipo_err)
    if(_ipo_ok)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    else()
        message(WARNING "LTO requested but not supported: ${_ipo_err}")
    endif()
endif()

# ------------------------------------------------------------------
# Install dirs
# ------------------------------------------------------------------
include(GNUInstallDirs)

# ------------------------------------------------------------------
# Subprojects
# ------------------------------------------------------------------
if(XPS_BUILD_CORE)
    add_subdirectory(core)
endif()

if(XPS_BUILD_CLI)
    add_subdirectory(cli)
endif()

if(XPS_BUILD_UI)
    add_subdirectory(ui)
endif()

if(XPS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ------------------------------------------------------------------
# Summary
# ------------------------------------------------------------------
message(STATUS "------------------------------------------------------------")
message(STATUS "xpscenery ${PROJECT_VERSION}")
message(STATUS "  build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "  generator  : ${CMAKE_GENERATOR}")
message(STATUS "  compiler   : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++        : ${CMAKE_CXX_STANDARD}")
message(STATUS "------------------------------------------------------------")
