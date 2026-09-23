# ART-006A author QA: National Mall reference ledger

**Status:** author-source QA only. Independent review remains separate.

## Scope

This pass records authoritative web references and modeling constraints for the first National Mall environment slice. It intentionally stores no external imagery, scans, map tiles, measured drawings or third-party assets.

## Research evidence, retrieved 2026-09-23

- NPS `The Mall`: core one-mile Capitol-to-Washington-Monument span, lawn-panel rhythm, path and tree-grove proportions.
- NPS `Lincoln Memorial Other Places`: Reflecting Pool length/width, capacity and west-axis role.
- NPS `Lincoln Memorial Building Statistics`: Colorado Yule marble, Milford granite, Indiana limestone and major height figures.
- NPS `Build Your Own Washington Monument`: white marble ashlar backed by blue gneiss and the 555 ft 5 1/8 in height.
- NPS `Constitution Gardens Cultural Landscape`: 6.75-acre lake, half-acre island and grade/landscape structure.
- NPS `Vietnam Veterans Memorial`: polished black granite wall and its Lincoln/Washington orientation.
- NPS `World War II Memorial FAQ`: named granite families; 56-pillar and pavilion hierarchy.
- NPS `The Mall Cultural Landscape`: four rows of American elms on each side and almost six hundred elms in the cultural-landscape inventory.
- Architect of the Capitol `U.S. Capitol Grounds`: 58.8-acre immediate grounds, curved walks and more than 100 varieties of trees/bushes.
- Smithsonian `Museum Maps`: ten Smithsonian museums on the Mall and the 3rd-to-15th-Street / Constitution-to-Independence visitor-map context.

The repository retains URLs and textual modeling facts only. It makes no blanket public-domain claim for page media.

## Commands

Run from packet root:

```text
python Scripts/verify_national_mall_reference_ledger.py Content/Reference/NationalMall/reference-ledger.json
python Scripts/test_national_mall_reference_ledger.py
python -m py_compile Scripts/verify_national_mall_reference_ledger.py Scripts/test_national_mall_reference_ledger.py
```

## Expected evidence boundary

Passing these checks means the reference ledger is structurally bounded and internally consistent. It does not prove the linked websites are immutable, establish survey-grade accuracy, grant media redistribution rights, or establish any runtime/art-approval state.

## Author execution receipt

Executed in the sandbox against the exact files prepared for the branch:

```text
python -m json.tool Content/Reference/NationalMall/reference-ledger.json
# exit 0

python Scripts/verify_national_mall_reference_ledger.py Content/Reference/NationalMall/reference-ledger.json
# PASS: 10 authoritative National Mall reference entries; source-only

python Scripts/test_national_mall_reference_ledger.py
# PASS: 12/12 National Mall reference-ledger tests

python -m py_compile Scripts/verify_national_mall_reference_ledger.py Scripts/test_national_mall_reference_ledger.py
# exit 0
```

Prepared-file SHA-256 values:

- `Content/Reference/NationalMall/reference-ledger.json`: `0de5556321fd6ac92386b28137ae6a3f226ed68881a764121be89470d1f19656`
- `Scripts/verify_national_mall_reference_ledger.py`: `58d8b558a3bfb6e6a593a5427188b5b4bfc6ecbf57fb1cab6559e743922e82b2`
- `Scripts/test_national_mall_reference_ledger.py`: `aec47544ba9a8b5c18a6bd1280991c83b25ec04f0792bc7e6ab113cc03b318d9`
- `Docs/Art/ART-BIBLE-v0.1.md`: `ac44d468462a1d9c3aeadf9c2f3b35d1c2959d56c87e728c97fd446cdc352fb7`

No native Windows/GPU, Blender, Astral runtime, renderer, import, performance or visual-art acceptance was executed or inferred.
