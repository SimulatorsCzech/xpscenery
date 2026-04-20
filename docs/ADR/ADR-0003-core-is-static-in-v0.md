# ADR-0003 — Core is a static library in v0.x

**Status:** Accepted
**Date:** 2026-04-20

## Context

During Phase 0b the `core/` library was declared as `STATIC` (see
`core/CMakeLists.txt`). The public header `core/include/xpscenery/xpscenery.h`
originally used Windows import/export attributes unconditionally:

```cpp
#if defined(_WIN32)
  #if defined(XPS_CORE_BUILD)
    #define XPS_API __declspec(dllexport)
  #else
    #define XPS_API __declspec(dllimport)
  #endif
#endif
```

That caused MSVC linker errors when `cli/` linked against the static lib
(`LNK2019: unresolved external symbol __imp_xps_version` etc.), because
consumers saw `dllimport` and generated `__imp_*` thunk calls that the
static archive did not provide.

## Decision

- Core ships as a **static library (`STATIC`) for all of v0.x**.
- The `XPS_API` macro becomes a no-op when `XPS_STATIC` is defined.
- CMake target `xps_core` sets `XPS_STATIC=1` as a `PUBLIC` compile
  definition, so every consumer of `xps::core` inherits it and gets the
  empty macro expansion automatically.
- Keep the `XPS_CORE_BUILD` export path in the header for the future,
  but guard it behind `#elif defined(_WIN32)` after the `XPS_STATIC`
  check.

## Consequences

**Pros**

- Simpler linkage, no DLL hell in early development.
- No need to stabilise the C++ ABI yet — we can break it each sprint.
- Smaller attack surface for symbol visibility bugs.

**Cons**

- CLI and UI executables bundle the core code directly → bigger binaries.
- No "plugin" architecture via DLL boundaries in v0.x (not a goal anyway).
- When we eventually flip to `SHARED` for plugin hosting, we will need
  an explicit ABI review and export annotations on every public symbol.

## Future

- In v1.0 or v2.0, reconsider a **`SHARED` plugin-host** variant
  exposing the C API as `xpscenery.dll`. Until then, `STATIC` wins
  on simplicity.
- If we need cross-DSO shared state earlier, the C API surface in
  `core/src/capi/xps_capi.cpp` already isolates the ABI shape.
