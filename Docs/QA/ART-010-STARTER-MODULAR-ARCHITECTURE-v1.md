# ART-010 Starter Modular Architecture v1 QA

## Purpose

Verify that a small generic modular kit has deterministic, inspectable source geometry suitable for a future Astral importer/editor handoff without pretending runtime support exists.

## Acceptance checks

- six unique named modules
- 0.25 m grid-aligned authored part bounds
- exact local bounds and base-corner pivots
- real indexed triangles
- finite positions, unit normals/tangents, tangent-normal orthogonality
- normalized face UVs
- outward winding
- exact source/glTF hashes in a deterministic manifest
- fail-closed schema and runtime-status fields
- no third-party asset payloads
- no runtime/art-approved claims

## Evidence boundary

Passing this QA promotes only the prepared packet to source-validated source data. Astral import/runtime/editor/performance and independent visual review remain separate future gates.
