# E14 Release-build reproducibility diagnostic packet, 2026-09-20

Status: bounded implementation packet for draft PR #9. This packet investigates the reproducibility discrepancy discovered by the prior benchmark-provenance pass. It is diagnostic only: no compiler/linker option, CMake behavior, production architecture, or dependency may be changed here.

## Dependency and baseline

- Repository: `LucasKazaki/AnimeRPG`.
- Candidate branch: `repair/2026-09-20-r0-runner-safety`.
- Exact baseline revision at admission: `cf67edc632d77ee93de991bf4e263a38e68adc9f`.
- Dependency: the E14 benchmark-provenance packet and the existing Release/package workflow on draft PR #9.
- Observed trigger: two independent `windows-2022` jobs built the same synthetic PR merge revision `9f2f31c134d6d527f045bcbd8afeaff4a2369216`, each reporting the same MSVC/toolset family and a 69,632-byte Release `AstralGame.exe`, but produced different SHA-256 values (`48e47c21...db60b` and `884a7a78...d3dc`). Those jobs did not retain both binaries in one place, so the changed PE fields are not yet known.
- Capability-map source: E14 in `Docs/Research/ENGINE-CAPABILITIES-2026-09-20.md` on draft PR #8. This packet advances evidence reproducibility only and does not complete E14.

## Reproducible question

Can two clean Release builds of one exact admitted revision, executed sequentially on one controlled Windows runner from the same build path and configuration, produce byte-identical `AstralGame.exe` files? If not, are all changed bytes localized to recognized PE/COFF metadata fields, or do payload/layout bytes also change?

The answer must be measured before changing build flags. A single matching pair is evidence for that pair only. A metadata-only mismatch localizes changed fields but does not prove the ambient input that caused them. Neither result establishes cross-machine reproducibility.

## Research basis

Primary sources accessed 2026-09-20:

- Microsoft PE/COFF format: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
  - Applicability: defines COFF timestamps, PE data directories, debug-directory entries and `IMAGE_DEBUG_TYPE_REPRO` (type 16). Microsoft states that a type-16 entry indicates a PE built for determinism/reproducibility and that, with unchanged inputs, such output is guaranteed bit-for-bit identical regardless of when or where it is produced. The same specification notes that date/time fields can contain content-derived hashes in reproducible images.
- Microsoft `DebugDirectoryEntryType.Reproducible`: https://learn.microsoft.com/en-us/dotnet/api/system.reflection.portableexecutable.debugdirectoryentrytype
  - Applicability: clarifies that a deterministic PE/COFF file is based only on documented inputs rather than ambient environment state; a deterministic COFF `TimeDateStamp` is content-derived rather than wall-clock time.
- Microsoft DIA `IDiaDataSourceEx::loadAndValidateDataFromPdbEx`: https://learn.microsoft.com/en-us/visualstudio/debugger/debug-interface-access/idiadatasourceex-loadandvalidatedatafrompdbex?view=visualstudio
  - Applicability: PDB GUID/signature and age are matching identities replicated in the corresponding executable/DLL. CodeView identity/path changes therefore need to be retained as evidence instead of treated as arbitrary payload changes.
- Unreal Engine 5.8 BuildGraph: https://dev.epicgames.com/documentation/unreal-engine/buildgraph
  - Applicability: UE models build automation as parameterized dependency-graph nodes that produce artifacts. Astral's much smaller diagnostic similarly binds two artifacts to one controlled build node, but this does not claim BuildGraph-equivalent tooling.
- Unreal Engine 5.8 packaging/build operations: https://dev.epicgames.com/documentation/unreal-engine/packaging-your-project and https://dev.epicgames.com/documentation/unreal-engine/build-operations-cooking-packaging-deploying-and-running-projects-in-unreal-engine
  - Applicability: UE treats Build as an independently testable stage before Stage/Package/Deploy/Run. Astral must distinguish binary-build evidence from later package/runtime acceptance.

No Epic, Unity, or Microsoft source code is copied. No dependency, SDK, plugin, or linker setting is added.

## Allowed paths

Only these paths may change in this packet:

- `Scripts/diagnose_pe_reproducibility.py`
- `Scripts/test_pe_reproducibility_diagnostic.py`
- `.github/workflows/release-manifest-validation.yml`
- `Tasks/E14-RELEASE-BUILD-REPRO-DIAGNOSTIC-2026-09-20.md`
- `Docs/QA/E14-RELEASE-BUILD-REPRO-DIAGNOSTIC-2026-09-20.md`

No `Engine/`, `Game/`, `Tests/`, CMake, dependency, graphics API, local-runtime, scheduler, or content change is authorized.

## Required implementation behavior

The diagnostic must be read-only and dependency-free. It must:

1. Parse bounded PE32/PE32+ images and reject malformed/out-of-range structures.
2. Record exact file size/SHA-256, machine, PE format, COFF timestamp, optional-header checksum, section raw hashes and debug-directory records.
3. Detect `IMAGE_DEBUG_TYPE_REPRO` and CodeView type-2 records; where the record is RSDS, retain its GUID, age and bounded PDB path.
4. Compare two images byte-for-byte and retain bounded contiguous diff ranges.
5. Classify differences conservatively as `identical`, `recognized_pe_metadata_only`, `layout_or_size_changed`, or `payload_or_unclassified_bytes_changed`.
6. Treat only explicit COFF timestamp, optional checksum, debug-directory timestamp and CodeView raw-data ranges as recognized metadata. Every other changed byte remains unclassified.
7. Never convert pairwise identity or metadata localization into a deterministic-linker, cross-machine, parity, or independent-acceptance claim.

The hosted workflow must run contract tests, then build the same exact checkout twice using fresh build trees at the same path with the same VS 2022 x64 Release command. It must retain the two EXEs in the runner temporary directory, record their hashes and generated-project hashes, run the diagnostic, and expose the classification through named CI steps. Do not enable `/Brepro`, alter CMake, or change compiler/linker options in this packet.

## Portable pre-publication verification

Run against the exact candidate source in a disposable fixture:

```text
python -m py_compile Scripts/diagnose_pe_reproducibility.py Scripts/test_pe_reproducibility_diagnostic.py
python Scripts/test_pe_reproducibility_diagnostic.py
```

The tests must cover byte identity, timestamp-only changes, CodeView identity/path changes, checksum changes, payload changes that must remain unclassified, malformed input, and CLI/claim-boundary behavior. Portable fixtures are not Windows linker evidence.

## Hosted Windows diagnostic

On one `windows-2022` job:

1. Resolve and print the exact checkout SHA.
2. Remove/recreate one runner-temporary build path, configure VS 2022 x64, build Release `AstralGame`, and copy the EXE/PDB/generated `AstralGame.vcxproj` to a retained runner-temporary evidence directory.
3. Wait at least two seconds, remove/recreate the same build path, repeat the identical configure/build commands, and retain the second artifacts.
4. Record SHA-256 for both EXEs, both generated project files and any retained PDBs.
5. Run `diagnose_pe_reproducibility.py` over the two EXEs and retain the JSON under the runner temporary directory.
6. Use named conditional steps to make `identical`, `recognized_pe_metadata_only`, and unsafe/unclassified/layout outcomes externally visible in the GitHub job receipt. Unknown payload/layout differences fail the diagnostic gate rather than being ignored.
7. Continue the existing package-manifest workflow only when the diagnostic itself parsed and classified the images successfully. A non-identical pair is a finding, not a reason to weaken tests.

## External outputs

All build trees, EXEs, PDBs, generated project files and diagnostic JSON remain under `RUNNER_TEMP`. They are evidence for hosted diagnosis, not a production release. No generated binary is committed.

## Rollback and stop conditions

- Stop at the first deterministic source/test/build/parser failure and retain that failure.
- Do not repeat an unchanged failed command.
- Do not change build flags, CMake, linker settings or source code to force byte identity in this packet.
- If the branch head moves before a write, re-read and reconcile before continuing.
- Rollback is removal/reversion of only the five allowed paths above.
- Do not invoke R0, merge, release, deploy, install prerequisites, launch GUI tests, operate the Company Runtime, or claim local Windows/GPU execution.

## Follow-up decision rule

- If two same-runner builds are byte-identical, record that narrow observation. The prior cross-job mismatch remains unresolved; next work should compare retained environment/path/tool inputs across independent jobs before modifying the build.
- If differences are recognized PE metadata only, record the exact changing field classes and whether a reproducible debug entry is present. Research the specific MSVC deterministic-link contract in a separate packet before proposing flags.
- If payload/layout bytes differ, stop and preserve the exact ranges/section hashes. The next packet should isolate changing compiler/link inputs rather than enabling a blanket deterministic option.

Native frame/GPU/RAM/VRAM benchmarks, clean-machine package launch, interactive runtime acceptance, the required 24-hour soak and independent review remain separate gates regardless of this result.
