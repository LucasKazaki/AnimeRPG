# ART-010: Starter Modular Architecture v1

**Loop:** `astral-art-hourly-20260922`
**Owner:** art worker
**Status:** proposed source fixture, not runtime acceptance

## Goal

Advance the generic starter library with a small original snapping-focused modular architecture kit. This is an importer-facing source asset, not an editor/runtime implementation.

## Reference rationale

Unreal Engine 5.8 documents grid-snapped actor placement and grid-based blockout/modeling workflows. Unity documents Scene-view grid alignment and incremental snapping. ART-010 adopts those workflow lessons only. It does not copy engine content or claim parity.

## Source contract

- glTF 2.0, right-handed, metres, +Y up, +Z forward, -X right
- six unique functional modules
- proposed 0.25 m translation grid and 90 degree rotation increment
- local snap pivot at the module minimum corner on its base plane
- indexed TRIANGLES with POSITION, NORMAL, TANGENT, TEXCOORD_0
- one neutral opaque material using linear baseColorFactor semantics
- deterministic generator and independent verifier
- source -> validated -> imported -> runtime_verified -> art_approved states remain distinct

## Stop condition

Source packet may be considered source-validated when fresh generation, exact regeneration check, independent verifier, focused regressions, and Python compile all pass. Do not merge under this worker's authority.

## Not claimed

No Blender round-trip, Khronos validator run, Astral import/render, editor primitive registration, collision/physics, LODs, navigation, native GPU measurements, or visual-art approval.
