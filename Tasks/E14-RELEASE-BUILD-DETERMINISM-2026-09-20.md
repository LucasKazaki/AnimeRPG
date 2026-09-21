# E14 Release-build determinism repair packet, 2026-09-20

Status: bounded implementation packet for draft PR #9. This follows the completed E14 Release reproducibility diagnostic and changes one Release-link setting only. It does not invoke R0, merge, release, deploy, install dependencies, change the graphics API, restart paused content work, or claim local/native acceptance.

## Dependency and baseline

- Repository: `LucasKazaki/AnimeRPG`.
- Candidate branch: `repair/2026-09-20-r0-runner-safety`.
- Exact baseline revision at admission: `0297b59f178e70915ba710ba7dc9e71e2b31e964`.
- Dependency: `Tasks/E14-RELEASE-BUILD-REPRO-DIAGNOSTIC-2026-09-20.md` and its QA receipt.
- Trigger: two sequential clean VS 2022 x64 Release builds from the same path produced same-size `AstralGame.exe` files whose only observed byte differences were the COFF timestamp and one PE debug-directory timestamp; neither image carried `IMAGE_DEBUG_TYPE_REPRO`.
- Capability-map source: E14 in `Docs/Research/ENGINE-CAPABILITIES-2026-09-20.md` on draft PR #8. This packet advances build/evidence reproducibility only and does not complete E14 or E15.

## Research basis

Primary/vendor-owned sources consulted 2026-09-20:

- Microsoft PE/COFF format: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
  - Applicability: `IMAGE_DEBUG_TYPE_REPRO` (type 16) denotes deterministic/reproducible PE output. Microsoft states that with unchanged inputs such an image is bit-for-bit identical regardless of build time or location, and timestamp-like fields become content-derived rather than wall-clock values.
- Microsoft BuildXL, `Public/Sdk/Experimental/Msvc/Native/Tools/Link/Link.dsc`: https://github.com/microsoft/BuildXL/blob/main/Public/Sdk/Experimental/Msvc/Native/Tools/Link/Link.dsc
  - Applicability: Microsoft-owned, MIT-licensed upstream code maps BuildXL's `linkDeterminism` setting directly to the MSVC linker flag `/Brepro` using `Cmd.flag("/Brepro", args.linkDeterminism)`. This is vendor-owned implementation evidence for the flag. The public `link.exe` option index does not currently document `/Brepro`, so this packet does not overstate it as a fully documented public CLI contract.
- Epic Unreal Engine 5.8 BuildGraph: https://dev.epicgames.com/documentation/en-us/unreal-engine/buildgraph-for-unreal-engine
  - Applicability: UE treats build nodes as declared artifact-producing steps and transfers outputs between agents. Astral's much smaller CI reproducibility check likewise keeps build output provenance separate from package/runtime acceptance.
- Epic Unreal Engine 5.8 C++ cooking development reference: https://dev.epicgames.com/documentation/en-us/unreal-engine/cplusplus-cooking-development-reference
  - Applicability: Epic explicitly treats byte differences from unchanged build inputs as indeterminism worth detecting; a clean single run is not sufficient proof because some indeterminism is intermittent.

No proprietary UE/Unity source is copied. `/Brepro` is passed only to Microsoft's linker already used by the project; no new dependency is added.

## Allowed paths

Only these paths may change in this packet:

- `CMakeLists.txt`
- `Tasks/E14-RELEASE-BUILD-DETERMINISM-2026-09-20.md`
- `Docs/QA/E14-RELEASE-BUILD-DETERMINISM-2026-09-20.md`

The existing `.github/workflows/release-manifest-validation.yml`, `Scripts/diagnose_pe_reproducibility.py`, and their tests are read-only verification infrastructure for this packet. No `Engine/`, `Game/`, `Tests/`, dependency, graphics API, local-runtime, scheduler, or content change is authorized.

## Bounded implementation

For MSVC only, add `/Brepro` to the `AstralGame` linker command only for the `Release` configuration. Do not change Debug, test executables, compile options, optimization, symbols, runtime-library policy, architecture, dependencies, or packaging behavior.

Expected CMake behavior:

```cmake
if(MSVC)
    target_link_options(AstralGame PRIVATE "$<$<CONFIG:Release>:/Brepro>")
endif()
```

The existing hosted reproducibility lane is the acceptance fixture. It already removes/recreates one build path, configures the same VS 2022 x64 project twice, builds `AstralGame` Release twice, hashes the generated `.vcxproj` and executable outputs, parses both PEs, reports `IMAGE_DEBUG_TYPE_REPRO`, and rejects payload/layout differences.

## Acceptance tests

For the exact candidate revision, hosted `windows-2022` must establish all of the following without weakening an existing test:

1. Both fresh Release builds succeed with the same generated-project hash.
2. The two `AstralGame.exe` outputs are byte-identical for the controlled pair.
3. Both images contain `IMAGE_DEBUG_TYPE_REPRO`.
4. The PE diagnostic reports no unclassified/layout differences.
5. The separate full Windows lane still passes the existing R0 safety, dependency/runtime/prerequisite checks, Debug build/tests, Release build/tests, milestone verifiers, and clean-tree check.
6. Package/evidence tooling continues to bind exact executable bytes and must not promote this result into package launch, performance, clean-machine, soak, parity, or independent acceptance.

A single same-runner matching pair is not cross-machine proof. The Microsoft PE guarantee applies only when inputs are unchanged; this packet does not prove that every ambient input is pinned across machines.

## External outputs

All generated builds, EXEs, PDBs, generated projects, diagnostic JSON, and package fixtures remain under GitHub runner temporary storage. No generated binary is committed.

## Rollback and stop conditions

- Stop at the first deterministic configure/build/test or unsafe diagnostic failure and retain it.
- Do not repeat an unchanged failed command.
- If `/Brepro` is rejected by the actual VS 2022 17.14 / MSVC 19.44 linker, revert only this packet's CMake change and record the vendor/toolchain mismatch rather than trying additional undocumented flags.
- If the pair remains non-identical, retain the exact PE diagnostic and do not add broader compiler/linker settings in this packet.
- If the branch head moves before a write, re-read and reconcile before continuing.
- Rollback is removal/reversion of the one scoped `CMakeLists.txt` addition plus this task/QA record.
- Do not invoke R0, merge, release, deploy, install prerequisites, run local processes, or claim access to Lucas's PCs.

## Remaining acceptance boundary

Even if this packet passes, native GPU/frame-time/RAM/VRAM benchmarks, clean-machine package launch, registered interactive RuntimeSmoke, the real 86,400-second soak, matched UE5/Unity scenes, and independent review remain open.