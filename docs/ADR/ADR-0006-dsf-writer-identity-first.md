# ADR-0006: DSF reader first, writer is identity rewriter for v0.x

**Status**: Accepted (2026-04-22)
**Context**: Phase 1 (DSF support)

## Context

`xptools260` ships a full DSF writer that encodes `DEFN`, `GEOD`,
`CMDS`, and `DEMS` atom trees from in-memory scene state. A full port
of that writer is a multi-week task (pool compaction, plane differencing,
RLE planning, MD5 recomputation, test vectors vs. X-Plane 12 RenderFarm
baseline output).

For v0.x we need **round-trip capability** — the ability to prove that
our decoders understood the file — but we do not yet need to emit DSFs
from scratch.

## Decision

1. For v0.x the only writer we ship is
   `io_dsf::rewrite_dsf_identity(src, dst)`:
   * reads the source file,
   * validates the cookie and version,
   * copies everything between the header and the MD5 footer
     **byte-for-byte**,
   * recomputes the MD5 footer over the new (identical) payload,
   * writes atomically via `io_filesystem::write_binary_atomic`.

   This gives us:
   * a guarantee that every byte we can read we can also re-emit;
   * a cheap repair tool for corrupted MD5 footers;
   * a foundation for differential writers (read → mutate atoms →
     re-emit).

2. The `CMDS` decoder in v0.x returns **aggregate statistics**, not
   a geometry stream. Emitting decoded triangles/objects as an
   in-memory scene graph is deferred to v0.5.0 (same milestone as
   the XESCore port) because both need the same `Scene` abstraction.

3. The `GeoTIFF` IFD parser is limited to classic (32-bit) TIFF.
   BigTIFF walks return a clear error. Full BigTIFF support is
   scheduled alongside the raster-pipeline work in Phase 2.

## Consequences

* We can ship a credible `dsf-rewrite` + `dsf-stats` pair to validate
  all existing scenery packs; if either fails, we found a real bug in
  our readers.
* A v0.3.0 user who wants an "atom-level" writer must wait for the
  same release as the mesh port — that is the correct order because
  the atom-level writer's acceptance test is "produces the same DSF
  as RenderFarm on the Linux baseline."
* Identity-rewrite tests are cheap (synthetic DSF + re-verify MD5)
  and catch buffer-overrun regressions in `read_binary` / MD5 paths
  without needing real scenery.

## Alternatives considered

* **Fully port `DSFLib.cpp` now.** Rejected: blocks every other
  feature for ~2 weeks, and we do not need emission until XESCore
  is ready.
* **Skip writing entirely.** Rejected: we lose the round-trip test
  and users have no way to repair a corrupted DSF footer from our CLI.
