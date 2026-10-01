# ART-012: Protagonist Readability v1

**Loop:** `astral-art-hourly-20260922`  
**Owner:** art worker  
**Status:** proposed source art-direction contract, not runtime acceptance

## Goal

Define and independently validate one measurable protagonist-readability contract for the retained AnimeRPG identity before production character asset work. Preserve one persistent customizable protagonist across masculine/feminine presentation variants and the Shadowblade, Arc Mage, and Aegis archetypes. Use current Astral runtime capabilities only as calibration fixtures; do not change renderer, gameplay, editor, build, CI, or engine acceptance.

## Baseline

- baseline main commit: `c644eec79752b9dc101aeca6b03141cf26854c96`
- source contract: `Content/ArtDirection/ProtagonistReadabilityV1/protagonist-readability-contract.json`
- existing Astral procedural humanoid is a runtime fixture, not a production protagonist or art approval
- Genshin Impact and Zenless Zone Zero remain visual/readability quality references only; no protected assets or distinctive designs may be copied

## Acceptance for this bounded source pass

1. The source contract parses as JSON without duplicate keys.
2. It pins the ART-012 task/loop identity and the observed baseline.
3. Review fixtures require 64, 96, and 128 px apparent body heights, with 96 px primary.
4. Front, three-quarter, profile, and rear views are present with explicit yaw.
5. Three neutral-background luminance fixtures are present and bounded to [0,1].
6. Base silhouette review explicitly disables class VFX and does not permit glow/hue/UI/camera motion to carry class identity.
7. One-protagonist identity and shared rig/equipment semantics remain explicit across masculine/feminine presentation variants.
8. Shadowblade, Arc Mage, and Aegis each define distinct shape, equipment, and pose-read rules.
9. Runtime capture requirements demand actual Astral captures plus exact commit/camera/material metadata before runtime claims.
10. All promotion flags remain false until the named independent/native gates occur.
11. A standard-library verifier rejects representative semantic corruption without weakening the contract.

## Allowed paths

This bounded ART-012 packet owns only:

- `Content/ArtDirection/ProtagonistReadabilityV1/**`
- `Docs/QA/ART-012-*`
- `Docs/Agents/art-hourly/**`
- `Tasks/ART-012-PROTAGONIST-READABILITY-v1.md`

No renderer, editor, gameplay, engine source, build, CI, local scheduler, dependency, release, or unrelated worker paths are owned by this packet.

## Not claimed

No production protagonist mesh or texture, DCC import, topology/UV/rig approval, retargeting, gameplay reach change, native capture, performance evidence, independent visual approval, engine acceptance, UE5/Unity parity, merge permission, deployment, or release.

## Stop condition

Stop after publishing the source validator plus exact-head QA receipt, verifying the branch contains only ART-owned paths relative to the baseline, and opening a draft PR if the normal GitHub write path permits it. Do not merge under this task.
