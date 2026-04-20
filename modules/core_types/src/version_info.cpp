#include "xpscenery/core_types/version_info.hpp"

namespace xps::core_types {

namespace {

#ifndef XPS_GIT_SHA
#  define XPS_GIT_SHA "unknown"
#endif
#ifndef XPS_VERSION_STRING
#  define XPS_VERSION_STRING "0.1.0-alpha.0"
#endif

#if defined(_MSC_VER)
#  define XPS_CXX_COMPILER_STR "MSVC " XPS_STR(_MSC_VER)
#  define XPS_STR_HELPER(x) #x
#  define XPS_STR(x) XPS_STR_HELPER(x)
#elif defined(__clang__)
#  define XPS_CXX_COMPILER_STR "Clang " __clang_version__
#elif defined(__GNUC__)
#  define XPS_CXX_COMPILER_STR "GCC"
#else
#  define XPS_CXX_COMPILER_STR "unknown"
#endif

#ifdef NDEBUG
#  define XPS_BUILD_TYPE_STR "Release"
#else
#  define XPS_BUILD_TYPE_STR "Debug"
#endif

constexpr VersionInfo kVersion{
    .semver       = XPS_VERSION_STRING,
    .git_sha      = XPS_GIT_SHA,
    .build_type   = XPS_BUILD_TYPE_STR,
    .cxx_compiler = XPS_CXX_COMPILER_STR,
    .cplusplus    = __cplusplus,
};

}  // namespace

const VersionInfo& current_version() noexcept { return kVersion; }

}  // namespace xps::core_types
