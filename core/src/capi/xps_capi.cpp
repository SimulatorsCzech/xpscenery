// ---------------------------------------------------------------------------
// xpscenery — C API implementation (skeleton)
// ---------------------------------------------------------------------------
#include "xpscenery/xpscenery.h"

#include <string>
#include <thread>

// Stringify helper (internal)
#define XPS_STRINGIFY_IMPL(x) #x
#define XPS_STRINGIFY(x)      XPS_STRINGIFY_IMPL(x)

namespace {

// Thread-local last-error message.
thread_local std::string g_last_error;

const char* kGitSha =
#ifdef XPS_GIT_SHA
    XPS_STRINGIFY(XPS_GIT_SHA);
#else
    nullptr;
#endif

constexpr xps_version_t kVersion{
    XPS_VERSION_MAJOR,
    XPS_VERSION_MINOR,
    XPS_VERSION_PATCH,
    nullptr  // populated at runtime below if git SHA known
};

} // namespace

extern "C" {

const xps_version_t* xps_version(void) {
    // Note: static means the `git_sha` pointer is reused across calls,
    //       which is fine because it's pointing at string literal storage.
    static xps_version_t v = kVersion;
    v.git_sha = kGitSha;
    return &v;
}

const char* xps_build_info(void) {
    return
        "xpscenery " XPS_STRINGIFY(XPS_VERSION_MAJOR) "."
                    XPS_STRINGIFY(XPS_VERSION_MINOR) "."
                    XPS_STRINGIFY(XPS_VERSION_PATCH)
        " (C++" XPS_STRINGIFY(__cplusplus) ")";
}

xps_status_t xps_init(void) {
    g_last_error.clear();
    // TODO: initialize subsystems (GDAL driver registration, logging, etc.)
    return XPS_OK;
}

void xps_shutdown(void) {
    // TODO: flush logs, release resources
}

const char* xps_last_error(void) {
    return g_last_error.empty() ? "" : g_last_error.c_str();
}

} // extern "C"
