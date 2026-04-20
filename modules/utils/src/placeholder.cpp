// xps_utils — placeholder translation unit for Phase 1 scaffold.
//
// Actual modules (FileUtils, XMLObject, ArgumentParser, ...) will be
// ported from xptools260/src/Utils into this directory in separate
// commits. See modules/utils/CMakeLists.txt for the port roadmap.

#include "xpscenery/utils/version.hpp"

namespace xps::utils {

const char* module_name() noexcept {
    return "xps_utils";
}

int module_revision() noexcept {
    return 0;  // bump when non-trivial files land
}

}  // namespace xps::utils
