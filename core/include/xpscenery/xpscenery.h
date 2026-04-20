// ---------------------------------------------------------------------------
// xpscenery — public C API
// Stable ABI surface for FFI (Qt UI, future Python/Rust bindings).
// All functions are extern "C" with POD types only.
// ---------------------------------------------------------------------------
#pragma once

#if defined(_WIN32)
  #if defined(XPS_CORE_BUILD)
    #define XPS_API __declspec(dllexport)
  #else
    #define XPS_API __declspec(dllimport)
  #endif
#else
  #define XPS_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Version
// ---------------------------------------------------------------------------

typedef struct {
    int major;
    int minor;
    int patch;
    const char* git_sha;  // short commit SHA, NULL if unavailable
} xps_version_t;

XPS_API const xps_version_t* xps_version(void);

XPS_API const char* xps_build_info(void);

// ---------------------------------------------------------------------------
// Library init / shutdown
// ---------------------------------------------------------------------------

typedef enum {
    XPS_OK              = 0,
    XPS_E_GENERIC       = 1,
    XPS_E_INVALID_ARG   = 2,
    XPS_E_IO            = 3,
    XPS_E_OUT_OF_MEMORY = 4,
    XPS_E_NOT_IMPLEMENTED = 99
} xps_status_t;

XPS_API xps_status_t xps_init(void);
XPS_API void         xps_shutdown(void);

// Return last error message (thread-local, valid until next API call)
XPS_API const char* xps_last_error(void);

// ---------------------------------------------------------------------------
// TODO — populate as Phase 1 port progresses:
//   xps_dsf_open / read / write
//   xps_mesh_build
//   xps_shp_import
//   xps_tile_process
// ---------------------------------------------------------------------------

#ifdef __cplusplus
} // extern "C"
#endif
