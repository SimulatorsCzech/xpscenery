#pragma once
// xpscenery — core_types/version_info.hpp

#include <string_view>

namespace xps::core_types {

struct VersionInfo {
    std::string_view semver;      ///< "0.1.0-alpha.0"
    std::string_view git_sha;     ///< short SHA or "unknown"
    std::string_view build_type;  ///< "Release" / "Debug"
    std::string_view cxx_compiler;///< "MSVC 19.50" etc.
    long             cplusplus;   ///< value of __cplusplus macro
};

/// Returns a snapshot of build-time information. Lifetime = static.
[[nodiscard]] const VersionInfo& current_version() noexcept;

}  // namespace xps::core_types
