#pragma once

// xps_utils public API — Phase 1 scaffold.
// The module is currently empty; see modules/utils/CMakeLists.txt for the
// roadmap of sub-modules to be ported from xptools260/src/Utils.

namespace xps::utils {

/// Returns the module name string, useful for logging.
const char* module_name() noexcept;

/// Returns a monotonically increasing revision of the ported module set.
/// 0 = empty scaffold, later increments are documented in CHANGELOG.md.
int module_revision() noexcept;

}  // namespace xps::utils
