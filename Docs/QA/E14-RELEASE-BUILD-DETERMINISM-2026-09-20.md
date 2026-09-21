# E14 Release-build determinism QA receipt, 2026-09-20

Status: bounded implementation checkpoint for draft PR #9. This receipt covers only the scoped Release-link determinism repair defined in `Tasks/E14-RELEASE-BUILD-DETERMINISM-2026-09-20.md`. It does not invoke or accept R0, and it does not establish engine parity, package launch, clean-machine compatibility, native GPU/performance evidence, soak acceptance, or independent review.

## Exact revisions and scope

- Repository: `LucasKazaki/AnimeRPG`.
- Branch: `repair/2026-09-20-r0-runner-safety`.
- Admission baseline: `0297b59f178e70915ba710ba7dc9e71e2b31e964`.
- Task packet commit: `bb32341838d5a8474b3da746a5b381d68f8242f5`.
- Implementation commit: `a9338859b8103d715c0a4980313e0b9d6ce9f579`.
- Hosted PR merge revision actually checked out by both Windows jobs: `e80c7b200f504207e2eb0cc03e27d094368287b2`, which merges implementation commit `a9338859...` into base `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`.
- Production change: `CMakeLists.txt` adds `/Brepro` only to `AstralGame` when the generator is MSVC and the configuration is `Release`.
- No `Engine/`, `Game/`, `Tests/`, dependency, graphics API, packaging architecture, Company Runtime, scheduler, or game-content file changed in this packet.

The CMake change is intentionally narrow:

```cmake
if(MSVC)
    target_link_options(AstralGame PRIVATE "$<$<CONFIG:Release>:/Brepro>")
endif()
```

Debug and all test executable linker settings remain unchanged.

## Research basis

Primary/vendor-owned sources consulted 2026-09-20:

1. Microsoft PE/COFF format: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
   - Microsoft defines `IMAGE_DEBUG_TYPE_REPRO` (type 16) as PE determinism/reproducibility evidence and states that unchanged input produces a bit-for-bit identical PE regardless of when or where it is produced; timestamp-like fields become content-derived rather than wall-clock values.
2. Microsoft BuildXL source, `Public/Sdk/Experimental/Msvc/Native/Tools/Link/Link.dsc`: https://github.com/microsoft/BuildXL/blob/main/Public/Sdk/Experimental/Msvc/Native/Tools/Link/Link.dsc
   - Microsoft-owned, MIT-licensed source maps BuildXL's `linkDeterminism` option directly to the MSVC linker flag `/Brepro` via `Cmd.flag("/Brepro", args.linkDeterminism)`.
   - Latest observed commit affecting this file during this pass: `1a8b856f37984c8e2dcbf4862edcdeca0dac49d6`, dated 2026-02-27.
   - Limitation: `/Brepro` is not listed in the current public `link.exe` option index. This receipt therefore treats the BuildXL mapping plus the actual MSVC 19.44 execution below as vendor-owned implementation evidence, not as a claim that Microsoft exposes a fully documented public CLI contract for every linker version.
3. Unreal Engine 5.8 BuildGraph: https://dev.epicgames.com/documentation/en-us/unreal-engine/buildgraph-for-unreal-engine
   - Applicability: UE build nodes declare outputs and transfer intermediate artifacts across build agents, reinforcing that build artifact identity/provenance is separate from package/runtime acceptance.
4. Unreal Engine 5.8 C++ Cooking Development Reference: https://dev.epicgames.com/documentation/unreal-engine/cplusplus-cooking-development-reference
   - Applicability: Epic requires byte-for-byte equivalence for unchanged cooked inputs in incremental-cook correctness checks and warns that a clean run is not proof because some indeterminism is intermittent.

No proprietary Unreal or Unity engine source was copied. No dependency was imported.

## Sandbox/compiler attempt

Attempted from the bounded sandbox to obtain a disposable checkout for portable source inspection. The clone failed before source mutation with `Could not resolve host: github.com` and exit code 128 because that container had no usable GitHub DNS/network route. The identical command was not repeated. Therefore this receipt makes no sandbox compile claim; the real compiler evidence for this packet is the hosted Windows execution below.

## Hosted reproducibility evidence

Workflow: `Release manifest integrity`

- Run: `35557304307`
- Job: `106203337540`
- Runner label: `windows-2022`
- Runner image: Windows Server 2022, image version `20260913.307.1`.
- Checkout: synthetic PR merge `e80c7b200f504207e2eb0cc03e27d094368287b2`.
- Compiler: MSVC `19.44.35228.0`.
- VC tools: `14.44.35207`.
- Windows SDK: `10.0.26100.0`.
- MSBuild: `17.14.51+25f168cee`.

The existing controlled fixture removed/recreated the same external build path, configured Visual Studio 2022 x64, built only `AstralGame` Release, retained the generated project and executable, slept two seconds between builds, then parsed both PE files.

Observed pair:

- Build 1 `AstralGame.exe`: 69,632 bytes, SHA-256 `9b733e83de3a6f1331c28e1b4e3920b4ae2b4b8d8960d62a6124fbce4c7cf490`.
- Build 2 `AstralGame.exe`: 69,632 bytes, SHA-256 `9b733e83de3a6f1331c28e1b4e3920b4ae2b4b8d8960d62a6124fbce4c7cf490`.
- Build 1 generated `AstralGame.vcxproj`: SHA-256 `e9ae7f4407479f678e9f88e8fe6f02b0182e33b198f0af6914398555167fafa7`.
- Build 2 generated `AstralGame.vcxproj`: same SHA-256 `e9ae7f4407479f678e9f88e8fe6f02b0182e33b198f0af6914398555167fafa7`.
- PDB: neither controlled build emitted `AstralGame.pdb`; no PDB reproducibility claim is made.
- PE comparison: `classification = identical`, `byte_identical = true`, `differing_byte_count = 0`, no changing-field classes, no layout/unclassified differences.
- PE image A `IMAGE_DEBUG_TYPE_REPRO`: present, type 16.
- PE image B `IMAGE_DEBUG_TYPE_REPRO`: present, type 16.
- Workflow log explicitly printed `Both images contain IMAGE_DEBUG_TYPE_REPRO: True`.
- The workflow's named `Repro classification: byte-identical pair` step passed; all metadata-difference classification steps were skipped because there were no differences.
- Job conclusion: **success**.

This directly repairs the earlier controlled failure mode where the same fixture produced different COFF/debug timestamps and no type-16 reproducibility entry. It establishes byte identity for this exact controlled pair with this exact hosted toolchain. It does **not** prove that every ambient input is pinned across machines or future image/toolchain updates.

The same workflow also passed the existing contract suites before and after the controlled pair:

- release-manifest tests: 7/7 pass;
- package runtime-smoke wrapper tests: 11/11 pass;
- restart-stress tests: 11/11 pass;
- continuous-soak tests: 11/11 pass;
- soak-analysis tests: 11/11 pass;
- benchmark-manifest tests: 12/12 pass;
- PE reproducibility diagnostic tests: 7/7 pass;
- separate Release package build, manifest generation/verification, hosted benchmark-manifest fixture, and clean tracked-tree checks: pass.

The separately staged hosted `AstralGame.exe` in that same job also had SHA-256 `9b733e83de3a6f1331c28e1b4e3920b4ae2b4b8d8960d62a6124fbce4c7cf490`, matching the controlled pair. Its manifest verification correctly retained `package_launch_verified=false`, `clean_machine_compatibility_verified=false`, and `independent_acceptance=false`.

## Full Windows regression evidence

Workflow: `Windows build and deterministic tests`

- Run: `35557304286`
- Job: `106203338245`
- Runner label/image: `windows-2022`, Windows Server 2022 image `20260913.307.1`.
- Checkout: same synthetic PR merge `e80c7b200f504207e2eb0cc03e27d094368287b2`.
- Compiler: MSVC `19.44.35228.0`.
- VC tools: `14.44.35207`.
- Windows SDK: `10.0.26100.0`.
- Job conclusion: **success**.

Passed checks include:

- R0 runner-safety contracts: 13/13;
- PE dependency inspector: 5/5;
- Windows prerequisite plan: 8/8;
- Windows runtime-environment probe: 8/8;
- Windows runtime-compatibility audit: 7/7;
- Windows Redistributable bootstrap: 8 pass, 1 expected Windows-only skip in the portable-refusal test;
- assertion/CTest safety: 3/3;
- Debug domain tests excluding GUI RuntimeSmoke: 8/8;
- Release domain tests excluding GUI RuntimeSmoke: 8/8;
- static M1/M2/M3 milestone verifiers: pass;
- clean tracked-tree check: pass.

The Release executable in this lane was also 69,632 bytes with SHA-256 `9b733e83de3a6f1331c28e1b4e3920b4ae2b4b8d8960d62a6124fbce4c7cf490`. This is useful cross-job evidence because the two separate hosted jobs, running in different Azure regions/workers during this run, produced the same exact Release executable bytes. It remains a hosted-image observation, not a general cross-machine/toolchain guarantee.

The prerequisite evidence remained unchanged in meaning: the Release image imports the normal x64 VC runtime, observed hosted central runtime files were version `14.51.36247.0`, which satisfied the `14.44.35207.0` build-tool floor. The bootstrap planner selected `skip_install` on this hosted machine, but no installer was downloaded/executed and no clean-machine package launch was performed.

## Acceptance result for this bounded packet

Passed for the exact candidate:

- smallest scoped build-setting repair compiled successfully under the actual VS 2022 / MSVC 19.44 lane;
- two fresh same-runner Release builds from identical generated project inputs became byte-identical;
- both controlled images carry `IMAGE_DEBUG_TYPE_REPRO`;
- the independent hosted full-Windows regression lane still passed all pre-existing non-GUI gates;
- a separately staged Release build in the package lane and the separate full-Windows job produced the same 69,632-byte executable SHA-256 in this run.

Not established and still required:

- independently reviewed acceptance of this change;
- registered local Windows GUI/RuntimeSmoke evidence;
- clean-machine package launch and prerequisite/install provenance;
- native GPU correctness, frame-time, RAM, and VRAM measurements;
- matched 2D/3D Astral/UE5/Unity benchmark scenes on approved hardware/settings;
- stress/recovery acceptance and the real 86,400-second soak;
- byte reproducibility across intentionally different source/build paths, compiler/SDK versions, or arbitrary machines;
- completion of the remaining capability catalogue.

R0 was **not invoked** in this pass. No merge, release, deployment, dependency installation, permission change, architecture change, graphics-API change, local scheduler action, or paused content work occurred.

## Next useful action

Use this now-reproducible Release artifact identity as a prerequisite for E14 benchmark evidence rather than broadening linker work further. The next bounded dependency-ready packet should implement or instrument a real native frame-timing capture for one frozen procedural Astral workload, with package/revision/driver/hardware provenance bound by the existing benchmark manifest. It must first remain a measurement tool and must not invent UE5/Unity parity thresholds. Native execution should be left to the registered local Windows executor with exact source SHA, toolchain/driver identity, commands, timestamps, retained raw samples, and hashes.