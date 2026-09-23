# ART-006A author QA: National Mall reference ledger

**Status:** author-source QA only. Independent review remains separate.

## Scope

This pass records authoritative web references and modeling constraints for the first National Mall environment slice. It intentionally stores no external imagery, scans, map tiles, measured drawings or third-party assets.

## Research evidence, retrieved 2026-09-23

- NPS `The Mall`: core one-mile Capitol-to-Washington-Monument span, lawn-panel rhythm, path and tree-grove proportions.
- NPS `Lincoln Memorial Event Operations Guide` (official PDF): 2,029-foot by 167-foot Reflecting Pool, 4,392-foot perimeter, 2010-2012 restoration, and adjacent accessible-path relationship.
- NPS `Lincoln Memorial Building Statistics`: Colorado Yule marble, Milford granite, Indiana limestone and major height figures.
- NPS `Build Your Own Washington Monument`: white marble ashlar backed by blue gneiss and the 555 ft 5 1/8 in height.
- NPS `Constitution Gardens Cultural Landscape`: 6.75-acre lake, half-acre island and grade/landscape structure.
- NPS `Vietnam Veterans Memorial`: polished black granite wall and its Lincoln/Washington orientation.
- NPS `World War II Memorial FAQ`: named granite families and bronze-versus-stone material separation.
- NPS `World War II Memorial` place page: 56 granite pillars, elliptical plaza hierarchy and two 43-foot pavilions.
- NPS `The Mall Cultural Landscape`: four rows of American elms on each side and almost six hundred elms in the cultural-landscape inventory.
- Architect of the Capitol `U.S. Capitol Grounds`: 58.8-acre immediate grounds, curved walks and more than 100 varieties of trees/bushes.
- Smithsonian `Museum Maps`: ten Smithsonian museums on the Mall and the 3rd-to-15th-Street / Constitution-to-Independence visitor-map context.

The repository retains URLs and textual modeling facts only. It makes no blanket public-domain claim for page media. A pre-publication source-fidelity audit split World War II Memorial material facts from the separate NPS place-page massing facts so each recorded measurement is supported by its own cited page. The same audit replaced a now-404 legacy Reflecting Pool webpage with the current NPS Event Operations Guide PDF and its current 2,029 x 167 foot dimensional contract.

Independent review first identified two fail-closed gaps: unknown root/entry fields were accepted, and encoded derived provenance labels were not bound to their numeric source values. Those were repaired. A later exact-head review found the value-only provenance binding was still incomplete because the same relation/value could be moved to a different label or unit, and `count` measurements could be fractional. The verifier now binds each encoded derived relation to the complete `(entry id, measurement label, unit, value)` contract and requires every count-valued measurement to be integral. Dedicated regressions cover Lincoln unit drift, Lincoln measurement-identity drift and a fractional WWII pillar count.

## Commands

Run from packet root:

```text
python Scripts/verify_national_mall_reference_ledger.py Content/Reference/NationalMall/reference-ledger.json
python Scripts/test_national_mall_reference_ledger.py
python -m py_compile Scripts/verify_national_mall_reference_ledger.py Scripts/test_national_mall_reference_ledger.py
```

## Expected evidence boundary

Passing these checks means the reference ledger is structurally bounded and internally consistent. It does not prove the linked websites are immutable, establish survey-grade accuracy, grant media redistribution rights, or establish any runtime/art-approval state.

## Author execution receipts

The unchanged repository ledger previously passed the exact branch author checks before the latest two review repairs:

```text
python -m json.tool Content/Reference/NationalMall/reference-ledger.json
# exit 0

python Scripts/verify_national_mall_reference_ledger.py Content/Reference/NationalMall/reference-ledger.json
# PASS: 11 authoritative National Mall reference entries; source-only

python Scripts/test_national_mall_reference_ledger.py
# PASS: 17/17 National Mall reference-ledger tests

python -m py_compile Scripts/verify_national_mall_reference_ledger.py Scripts/test_national_mall_reference_ledger.py
# exit 0
```

For the latest review repair, the external sandbox could not resolve `raw.githubusercontent.com`, so it could not obtain a fresh exact-head checkout of the unchanged ledger. The exact verifier and test-script bytes prepared for GitHub were instead exercised against an isolated schema-valid 11-entry fixture that carries the same Lincoln, Washington and WWII measurement contracts. This validates the new control flow and all 20 regression definitions, but it is intentionally **not** recorded as an exact-branch ledger-suite receipt:

```text
python Scripts/verify_national_mall_reference_ledger.py Content/Reference/NationalMall/reference-ledger.json
# PASS: 11 authoritative National Mall reference entries; source-only

python Scripts/test_national_mall_reference_ledger.py
# PASS: 20/20 National Mall reference-ledger tests

python -m py_compile Scripts/verify_national_mall_reference_ledger.py Scripts/test_national_mall_reference_ledger.py
# exit 0
```

The exact branch suite against the repository ledger remains required before this packet is marked ready for integration review. Source inspection confirms the unchanged ledger's count values are integral and its two encoded derived measurements retain their intended entry ids, labels, `feet` units and source-derived numeric values; that inspection does not replace execution.

Current prepared-file SHA-256 values:

- `Content/Reference/NationalMall/reference-ledger.json`: `3c2664e72d2ce7c8024a020f2179c0c0116f02617cd22f41fd809cf632c40ba8`
- `Scripts/verify_national_mall_reference_ledger.py`: `0cd179f014bd4006a185cad2268f729bcc1738c16819b2b8aa8cc9834b9207f4`
- `Scripts/test_national_mall_reference_ledger.py`: `ab10e589d67e9e7e08fddc1699dc820791fa1c44093018cd72211ad4fcb1b6c1`
- `Docs/Art/ART-BIBLE-v0.1.md`: `ac44d468462a1d9c3aeadf9c2f3b35d1c2959d56c87e728c97fd446cdc352fb7`

No native Windows/GPU, Blender, Astral runtime, renderer, import, performance or visual-art acceptance was executed or inferred.
