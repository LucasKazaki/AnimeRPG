# E14 Release-build reproducibility diagnostic evidence, 2026-09-20

Status: hosted diagnostic passed with a reproducibility finding. This record does not claim byte-reproducible Release builds, deterministic-linker configuration, native runtime acceptance, performance parity, clean-machine compatibility, or independent acceptance.

## Scope and revisions

- Repository: `LucasKazaki/AnimeRPG`.
- Draft PR: #9, branch `repair/2026-09-20-r0-runner-safety`.
- Admitted packet baseline: `cf67edc632d77ee93de991bf4e263a38e68adc9f`.
- Implementation head that triggered the hosted diagnostic: `2903313b75ff730672da153faa769c6090f91393`.
- GitHub pull-request workflows tested synthetic merge revision `927f90efc11de52227234e4abcbe46c6bdcda3cb` (`2903313...` merged into the then-current PR base `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`). This record does not mislabel the synthetic merge binary as a branch-head binary.
- Bounded task: `Tasks/E14-RELEASE-BUILD-REPRO-DIAGNOSTIC-2026-09-20.md`.
- Changed implementation paths: `Scripts/diagnose_pe_reproducibility.py`, `Scripts/test_pe_reproducibility_diagnostic.py`, `.github/workflows/release-manifest-validation.yml`, and the task file above. This QA record is the fifth and final allowed path for the packet.
- No `Engine/`, `Game/`, `Tests/`, `CMakeLists.txt`, dependency, graphics API, Company Runtime, scheduler, or content file was changed.

## Why this packet ran

The preceding E14 benchmark-provenance pass observed different SHA-256 values for same-size Release `AstralGame.exe` outputs built for the same synthetic merge revision in separate Windows jobs. Because those jobs did not retain both images together, the changing PE fields were unknown. This packet therefore diagnosed the bytes before proposing any linker or CMake change.

## Primary-source research, accessed 2026-09-20

- Microsoft PE/COFF format: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
  - Defines the COFF header timestamp, PE debug directory, and `IMAGE_DEBUG_TYPE_REPRO` (type 16). Microsoft states that a type-16 debug entry indicates an image built for determinism/reproducibility and that unchanged inputs produce bit-for-bit identical output regardless of build time/location. The specification also notes that timestamp fields can hold content-derived values in reproducible images.
- Microsoft `DebugDirectoryEntryType.Reproducible`: https://learn.microsoft.com/en-us/dotnet/api/system.reflection.portableexecutable.debugdirectoryentrytype
  - Clarifies that deterministic PE/COFF output is based only on documented inputs instead of ambient environment and that deterministic COFF timestamps are content-derived rather than wall-clock timestamps.
- Microsoft DIA PDB validation: https://learn.microsoft.com/en-us/visualstudio/debugger/debug-interface-access/idiadatasourceex-loadandvalidatedatafrompdbex?view=visualstudio
  - Establishes that PDB signature/GUID plus age identify matching program/debug information. The diagnostic therefore retains CodeView identity/path evidence when present rather than treating it as arbitrary payload.
- Unreal Engine 5.8 BuildGraph: https://dev.epicgames.com/documentation/unreal-engine/buildgraph
  - UE models build automation as parameterized nodes with explicit produced artifacts. Astral's packet is much smaller, but similarly binds evidence to controlled build outputs rather than relying on prose.
- Unreal Engine 5.8 packaging/build operations: https://dev.epicgames.com/documentation/unreal-engine/packaging-your-project and https://dev.epicgames.com/documentation/unreal-engine/build-operations-cooking-packaging-deploying-and-running-projects-in-unreal-engine
  - UE separates Build from later Stage/Package/Deploy/Run operations. Astral likewise keeps binary-build reproducibility evidence separate from package/runtime acceptance.

No proprietary engine source was copied. No dependency or compiler/linker setting was added. No unsupported `/Brepro` claim is made here because this packet did not establish a current authoritative invocation contract for that option.

## Implemented diagnostic

`Scripts/diagnose_pe_reproducibility.py` is a dependency-free, read-only PE32/PE32+ analyzer with bounded input, section, debug-entry, PDB-path, and diff-range limits. It records file hash/size, machine, format, COFF timestamp, optional-header checksum, section raw hashes, debug-directory records, `IMAGE_DEBUG_TYPE_REPRO`, and RSDS CodeView GUID/age/PDB path where available.

Comparison classes are deliberately narrow:

- `identical`
- `recognized_pe_metadata_only`
- `layout_or_size_changed`
- `payload_or_unclassified_bytes_changed`

Only explicit COFF timestamp bytes, optional-header checksum bytes, debug-directory timestamp bytes, and CodeView raw-data ranges can count as recognized metadata. All other changed bytes remain unclassified and fail the hosted diagnostic gate. Pairwise identity never sets `deterministic_linker_contract_established`, `cross_machine_reproducibility_verified`, or `independent_acceptance` true.

## Contract verification

Portable pre-publication fixture checks:

```text
python -m py_compile Scripts/diagnose_pe_reproducibility.py Scripts/test_pe_reproducibility_diagnostic.py
python Scripts/test_pe_reproducibility_diagnostic.py
```

The disposable portable fixture passed 7/7 tests. It was useful for syntax/contract development only and is not Windows linker evidence.

The exact published candidate was then exercised by GitHub Actions on Windows. `Scripts/test_pe_reproducibility_diagnostic.py` passed 7/7 tests, covering identical images, timestamp-only changes, CodeView GUID/path changes, checksum changes, payload changes that must not be laundered as metadata, malformed input, and CLI/claim boundaries.

## Hosted Windows reproducibility result

Release-manifest workflow:

- Run: `35554174958`
- Job: `106194447047`, `windows-package-manifest`
- Conclusion: success
- Runner: Windows Server 2022 Datacenter `10.0.20348`, `windows-2022` image `20260913.307.1`, runner `2.337.0`.
- Windows SDK: `10.0.26100.0`.
- Compiler: MSVC `19.44.35228.0`.
- VC toolset path/version: `14.44.35207`.
- MSBuild: `17.14.51+25f168cee`.
- Controlled build path for both builds: `D:/a/_temp/Astral-repro-build`.

For both iterations the workflow deleted/recreated the same build directory and ran the same commands:

```text
cmake -S . -B D:/a/_temp/Astral-repro-build -G "Visual Studio 17 2022" -A x64
cmake --build D:/a/_temp/Astral-repro-build --config Release --target AstralGame --parallel
```

A two-second delay separated the iterations. Both builds generated the same `AstralGame.vcxproj` bytes:

- generated project SHA-256, both builds: `398a3bc352b09c8410c3526fb5ddf883b5154b34d97ac4490c612b135f34c37b`
- neither build emitted `AstralGame.pdb`, so no PDB identity claim is made.

The two Release executables were the same size but not byte-identical:

- Build 1: 69,632 bytes, SHA-256 `4d6ab2e5f46d4b85a5dae3302e3a8e5c5ba9f7aec1fc31f3f6d4fd9927daa5aa`
- Build 2: 69,632 bytes, SHA-256 `2a8df680ec8d8d4e49e4f966f9303def107a55516577ca0157f0f6f47470d614`
- Classification: `recognized_pe_metadata_only`
- Retained differing byte count: 2
- Retained diff-range count: 2, not truncated
- Changed range `[248, 249)`: within the COFF timestamp field
- Changed range `[51028, 51029)`: within the PE debug-directory timestamp field
- Build 1 COFF/debug timestamp value: `1789957615`
- Build 2 COFF/debug timestamp value: `1789957626`
- Optional-header checksum: `0` in both images
- CodeView-data change: none observed
- `IMAGE_DEBUG_TYPE_REPRO`: absent from both images

The debug-directory entry carrying the changing timestamp was type 13, with size 800, raw-data pointer 52708, and address 56292. The diagnostic did not infer semantics for type 13 beyond the standard debug-directory header fields.

All raw section hashes were identical except `.rdata`, which contains the changed debug-directory timestamp byte. The byte-level classifier localized that `.rdata` mismatch to the explicit debug-directory timestamp range; no unclassified payload change was observed for this pair. The generated project files were also byte-identical.

This establishes a narrow finding: for these two same-runner, same-path, same-toolchain builds, the executable mismatch is localized to the observed COFF and debug-directory timestamp fields. It does not establish the global root cause of all prior cross-job hash differences. The absence of `IMAGE_DEBUG_TYPE_REPRO`, together with Microsoft's documented deterministic-PE contract, means Astral has not yet established a deterministic-linker/reproducible-image configuration.

## Existing regression lanes at the same candidate

The exact synthetic merge candidate also passed the surrounding hosted checks.

Release-manifest job `106194447047`:

- release-manifest contracts: 7/7
- package runtime-smoke contracts: 11/11
- package restart-stress contracts: 11/11
- package continuous-soak contracts: 11/11
- soak-analysis contracts: 11/11
- benchmark-manifest contracts: 12/12
- PE reproducibility diagnostic contracts: 7/7
- repeated Release diagnostic: success with `recognized_pe_metadata_only`
- hosted Release package manifest: passed
- hosted benchmark-provenance contract fixture: passed
- clean tracked tree: passed

The later ordinary hosted package build in this job produced another 69,632-byte `AstralGame.exe`, SHA-256 `e060c8703987309c62bec1f38453a947457b3d4c0af0b16a2a586fc6fbeb0b6f`, further demonstrating why benchmark/package evidence must remain bound to the exact built bytes rather than assuming a revision alone determines the binary. Its benchmark descriptor SHA-256 was `d52a1b8f021bc1d9778d430f75d15d10b7561397f175a283193df4d310fef1cf`.

Full Windows workflow:

- Run: `35554174959`
- Job: `106194455559`, `MSVC Debug and Release`
- Conclusion: success
- R0 safety contracts: 13/13
- PE dependency contracts: 5/5
- Windows prerequisite-plan contracts: 8/8
- Windows runtime-environment contracts: 8/8
- Windows runtime-compatibility contracts: 7/7
- Windows Redistributable-bootstrap contracts: 8 passed, 1 intentional platform-specific skip
- Release assertion/CTest safety: 3/3
- Debug deterministic domain tests: 8/8
- Release deterministic domain tests: 8/8
- static milestone verifiers: passed
- clean tracked tree: passed

The full lane used VC Build Tools `14.44.35207`; its current Release package depended on the reviewed x64 central VC Redistributable policy. The hosted VC runtime files were `14.51.36247.0`, satisfying the recorded version floor, but package launch, supported installer provenance, clean-machine compatibility and independent acceptance remained false.

## Claim boundaries and deferred acceptance

Passed:

- the new parser/contract suite on the exact hosted candidate
- same-runner two-build evidence collection
- localization of this pair's changes to recognized timestamp fields
- surrounding hosted Debug/Release and packaging contract lanes

Not passed or not attempted here:

- byte-identical Release builds
- `IMAGE_DEBUG_TYPE_REPRO` emission
- an authoritative, tested deterministic MSVC linker configuration
- cross-job or cross-machine binary reproducibility
- interactive `M10RuntimeSmoke`
- finished-package launch on a supported clean Windows machine
- actual GPU behavior or E14 frame-time/RAM/VRAM budgets
- repeated native restart acceptance
- required 86,400-second soak
- independent review/acceptance

R0 was not invoked. No executable/package was merged, released, deployed, installed, or treated as production-ready.

## Single next useful action

Use a separate bounded build-configuration packet to establish the supported deterministic/reproducible MSVC linking mechanism for the repository's current MSVC `19.44` / VC tools `14.44` environment from authoritative Microsoft/CMake evidence or the installed linker help. Test the smallest isolated build-setting change without modifying architecture or dependencies. Require, at minimum, same-path bit identity across repeated clean Release builds and inspect whether `IMAGE_DEBUG_TYPE_REPRO` is emitted before considering independent-job reproducibility. If that setting changes payload/layout unexpectedly or cannot be justified from authoritative evidence, stop and preserve the diagnostic rather than forcing identity.

Native performance measurements, matched Unreal Engine 5.8/Unity 6.0 reference workloads, clean-machine package launch, continuous 24-hour soak, and independent review remain separate unresolved engine-comparability gates.
