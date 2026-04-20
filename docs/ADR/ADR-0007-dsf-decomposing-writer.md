# ADR-0007 — Decomposing DSF writer as v0.3.0 foundation

**Status**: Accepted (2026-04-22)
**Supersedes part of**: [ADR-0006](ADR-0006-dsf-writer-identity-first.md)
**Related**: ADR-0006 (identity writer)

## Context

ADR-0006 delivered an *identity* DSF rewriter: copy the whole file body
verbatim, recompute the MD5 footer, write atomically. That is enough to
repair corrupted footers but not enough to fulfill the v0.3.0 promise
of a **bit-identical round-trip writer** that can *modify* DSFs.

X-Plane's DSF layout is a tree of `[4-byte id | 4-byte size | payload]`
atoms bracketed by a 12-byte header (`XPLNEDSF` + int32 version) and a
16-byte MD5 footer. Any mutation of a single atom's payload requires the
containing atom's size field to be updated, and of course the file's
MD5 to be recomputed — but the other atoms' bytes can flow through
untouched.

Building a **fully decoded** atom-level writer (CMDS re-emission,
POOL/SCAL re-encoding, DEFN table rebuild, …) is a v0.5.0-scale effort
tied to the XESCore port (see ADR-0006 §5). We need an intermediate
step that unlocks meaningful DSF mutation *today*.

## Decision

Ship a **decomposing writer** — `read_dsf_blob` + `write_dsf_blob` —
that splits a DSF into opaque top-level atom blobs (tag + original
bytes including the 8-byte header) and can re-emit them in any order,
with any subset, with any replaced payload, provided each `AtomBlob`'s
header size field is consistent with its byte count.

```
DsfBlob {
    int32 version
    vector<AtomBlob> atoms
}
AtomBlob {
    string tag            // "HEAD", "DEFN", "GEOD", ...
    uint32 raw_id
    vector<byte> bytes    // full atom incl. 8-byte header
}
```

Consumers of this API do **not** need to understand atom contents.
They can:

1. Read a DSF into a `DsfBlob`.
2. Swap out, drop or append atoms by manipulating `blob.atoms`.
3. Call `write_dsf_blob(dst, blob)` — the writer validates the header
   size field of each blob, concatenates them, and appends a fresh MD5.

Round-trip `read_dsf_blob → write_dsf_blob` on an unmodified blob is
guaranteed **bit-identical** (unit test `rewrite_dsf_decomposed is
bit-identical on a well-formed DSF`).

The decomposing path is exposed to users via
`xpscenery-cli dsf-rewrite <src> <dst> --mode decomposed`.

## Consequences

### Positive

- Unlocks DSF mutation (replace a raster atom, strip a DEMS atom,
  insert a new PROP atom) in v0.3.0 without a geometry decoder.
- Existing atom-reading code (`read_top_level_atoms`) is reused
  zero-copy.
- The "one atom = opaque bytes" abstraction matches how atoms are
  already cached during reading, so there is no duplicate parsing.
- Gives v0.5.0 XESCore port a clean insertion point: CMDS/POOL/SCAL
  re-emitters can produce replacement `AtomBlob`s for specific tags
  while all other atoms stream through unchanged.

### Negative / Limitations

- `write_dsf_blob` **does not** recompute per-atom size fields: the
  caller is responsible for keeping the 4-byte size header in sync
  with `bytes.size()`. The writer validates this invariant and errors
  out if violated, so bugs are caught at write time rather than when
  X-Plane loads the file.
- Nested container atoms (atoms that contain atoms) are treated as
  opaque blobs. Mutating a nested atom today means replacing its
  entire parent's bytes. A recursive blob API is deferred.
- No attempt is made to re-normalise POOL/SCAL alignment or CMDS
  opcode ordering. Decomposed output equals the input byte-for-byte
  *only* when no mutation occurred.

### Neutral

- Same error-model (`std::expected<T, std::string>`) and same
  atomic-write machinery (`io_filesystem::write_binary_atomic`) as
  the identity path; no new dependencies.

## Alternatives Considered

**(A) Keep identity-only until v0.5.0.** Rejected — blocks every
downstream mutation feature for 3+ months.

**(B) Full atom-of-atoms recursive blob tree.** Deferred — adds
complexity without payoff: DSF files rarely nest beyond 2 levels in
the top-level DEFN/GEOD/CMDS tree, and the recursive case can be
added backward-compatibly when needed.

**(C) Decode every atom.** That's the v0.5.0 plan with XESCore. Too
much surface area to deliver safely in a single turn.

## References

- [modules/io_dsf/include/xpscenery/io_dsf/dsf_writer.hpp](../../modules/io_dsf/include/xpscenery/io_dsf/dsf_writer.hpp)
- [modules/io_dsf/src/dsf_writer.cpp](../../modules/io_dsf/src/dsf_writer.cpp)
- [tests/unit/io_dsf/test_dsf_decomposed.cpp](../../tests/unit/io_dsf/test_dsf_decomposed.cpp)
- ADR-0006 — identity writer rationale (what this ADR extends)
