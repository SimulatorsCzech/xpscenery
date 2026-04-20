# Licensing summary

**Project code:** MIT License (see [`LICENSE`](../LICENSE))

## Third-party license matrix

| Dependency | License | Linking mode | Effect on distributed binary |
|---|---|---|---|
| Qt 6 | LGPL-3.0 | dynamic (`.dll`) | must allow users to substitute Qt |
| CGAL | GPL-3.0 | static (header-only) | **distributed binary is GPL-3.0** |
| GDAL | MIT/X11-style | dynamic | no restriction |
| Boost | Boost Software License | header-only | permissive |
| GMP | LGPL-3.0 | dynamic | users may substitute |
| MPFR | LGPL-3.0 | dynamic | users may substitute |
| fmt | MIT | static | permissive |
| spdlog | MIT | static | permissive |
| nlohmann-json | MIT | header-only | permissive |
| Catch2 | BSL-1.0 | static | permissive (test-only) |

## Key consequence

Because CGAL is **GPL-3.0** and is statically compiled into the core library, any binary we distribute that contains CGAL code is automatically a GPL-3.0 combined work. Downstream users must therefore receive source + GPL-3.0 rights.

**Options to avoid GPL contagion** (not currently needed):
1. Purchase a commercial CGAL license from GeometryFactory (~1500 €/year).
2. Replace CGAL with a permissive alternative (Clipper2 + earcut.hpp + custom CDT) — massive engineering cost.

Decision: remain GPL-3.0 for the distributed binary, keep own source MIT (clean, minimal, reusable).

## Attribution

The installer will bundle a `THIRD_PARTY_NOTICES.txt` containing the full verbatim license texts for every linked library, as required by each respective license.
