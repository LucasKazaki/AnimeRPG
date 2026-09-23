# ART-006A: National Mall authoritative reference ledger

**Loop:** `astral-art-hourly-20260922`  
**Parent backlog item:** ART-006, National Mall authored material family  
**Status:** bounded source/reference packet; no production-art or runtime claim  
**Base:** art PR #28 exact head `087c673f716800198272915777681c61a748a490`

## Goal

Close the geographic/reference-ledger prerequisite for future National Mall environment and material work without waiting on Astral texture rendering. Establish a small, machine-validated source of truth for the first world blockout and material decisions using authoritative federal/institutional pages only.

## Allowed paths

- `Content/Reference/NationalMall/reference-ledger.json`
- `Content/Reference/NationalMall/README.md`
- `Docs/Art/ART-BIBLE-v0.1.md`
- `Docs/QA/ART-006A-NATIONAL-MALL-REFERENCE-LEDGER-2026-09-23.md`
- `Scripts/verify_national_mall_reference_ledger.py`
- `Scripts/test_national_mall_reference_ledger.py`
- `Tasks/ART-006A-NATIONAL-MALL-REFERENCE-LEDGER-2026-09-23.md`

Do not modify `Engine/`, gameplay, renderer/editor/importer code, CMake, workflows, dependencies, `Docs/Agents/art-hourly/*`, or another worker's open paths. The shared art-hourly continuation records are intentionally left untouched because PR #33 currently owns them.

## Acceptance

1. Ledger contains at least eight authoritative entries covering axis/scale, materials, landscape, vegetation and context boundaries.
2. Every entry uses HTTPS and an allowed authoritative host (`nps.gov`, `aoc.gov`, or `si.edu`).
3. No external images, scans, map tiles or downloadable media are embedded.
4. Numeric facts retain source units and provenance labels; no silent Unreal/Unity coordinate convention is introduced.
5. Art bible gains a concise source-control/simplification rule pointing to this ledger.
6. Independent standard-library verifier rejects false runtime status, boolean schema confusion, unapproved hosts, embedded media, missing facts, invalid measurement values/units/provenance and incomplete coverage.
7. Focused test suite and `py_compile` pass in the sandbox.

## Evidence boundary

This packet does **not** establish surveyed geometry, photogrammetry rights, finished landmark models, runtime import, Astral lighting/material behavior, performance, visual art approval, or parity with UE5, Unity, Genshin Impact or Zenless Zone Zero.
