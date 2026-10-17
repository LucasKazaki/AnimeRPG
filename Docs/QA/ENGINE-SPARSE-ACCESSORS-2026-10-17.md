# Sparse accessor QA — October 17, 2026

Packet: `Tasks/ENGINE-SPARSE-ACCESSORS-2026-10-17.md`.
Base: `4552a31b1669ed34ebb23f05e502af6b425ce696`.
Owner: `/root/candidate_review`; dedicated `work/astral-sparse-accessors`.

## Original reproduction

Source inspection confirms that sparse substitutions update the float output
alone, while primitive indices and JOINTS_0 consume the optional integer output.
The coordinator approved exact unsigned decoding plus bounded sparse selector
count/type/order/range validation. It granted a serialized old-source slot after
the separate combined baseline and prefab old filters finished.

On `DESKTOP-AKPLL81`, October 17, 18:57:52–18:58:38 UTC:

| Retained receipt in `work/sparse-evidence` | Result |
| --- | --- |
| `old-configure` | Exit 0, 3.344 s; VS2022 x64, external build directory |
| `old-debug-build` | Exit 0, 32.078 s; only EngineAssetsTests target, two build jobs |
| `old-index-consumer` | Child exit 3, 0.031 s; exact imported indices assertion failed |
| `old-joint-consumer` | Child exit 3, 0.016 s; imported sparse vertex-1 joint assertion failed |

The expected assertions occur before the renderer/skinning calls, so these old
receipts establish incorrect imported data rather than claiming those calls ran.
Each executable was invoked separately. The generic evidence helper records the
actual child exit 3; PowerShell surfaced its nonzero wrapper result as exit 1.
No importer production edit existed during configuration, compilation or either
run. The Engine/Tests patch was test-only, SHA256
`66f0afccdf2a3fffe4726d92abe443659c6716fe31974802bd36e6da15053bc8`.
Every command has separate stdout, stderr, exact argv/cwd, source head/patch,
machine and UTC/elapsed receipt. No compile/link/test process remained at release;
an idle existing MSBuild node-reuse worker predating this slot was left alone.

Exact child commands, from the owned worktree:

```powershell
$sparseCmake = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
& $sparseCmake -S . -B ../build-sparse-accessors -G 'Visual Studio 17 2022' -A x64
& $sparseCmake --build ../build-sparse-accessors --config Debug --target EngineAssetsTests --parallel 2
& '../build-sparse-accessors/Debug/EngineAssetsTests.exe' GltfSparsePrimitiveIndicesReachRenderer
& '../build-sparse-accessors/Debug/EngineAssetsTests.exe' GltfSparseJointsReachSkinMesh
```

These were executed through bundled Python and the external
`work/sparse-evidence/run_logged.py` helper, which uses an argv list and retains
the child process's outputs and exit. The coordinator accepted both old failures
and cleared production implementation before any importer edit.

## Fixed implementation and scheduled evidence

The importer now uses the same raw unsigned decoder for dense and sparse
integer slots, preserving exact byte/ushort/uint data separately from float
conversion. Sparse count must be positive and no greater than accessor count;
selector types must be unsigned byte/short/int; targets must increase strictly
and remain in range. Existing negative-offset and byte-range checks remain.
No containment, dependency observation, warning, image, publication or public
API code changed. Gltf.h is unchanged.

On `DESKTOP-AKPLL81`, October 17, 19:00:06–19:01:44 UTC, under one granted slot:

| Retained receipt in `work/sparse-evidence` | Result |
| --- | --- |
| `fixed-debug-build` | Exit 0, 6.219 s; three affected targets, two build jobs |
| `fixed-debug-test` | Exit 0, CTest 3/3 in 7.01 s |
| `fixed-release-build` | Exit 0, 48.563 s; same three targets/two jobs |
| `fixed-release-test` | Exit 0, CTest 3/3 in 1.05 s |
| `fixed-safety` | Exit 0; unchanged safety 3/3 in 4.968 s (wrapper 5.078 s) |

Each configuration executed 63 registered cases: EngineAssetsTests 29,
EngineFrameworkTests 14 and EngineSampleTests 20. Verbose logs explicitly show
all seven added cases pass, including actual CPU object-ID rendering of the
sparse triangle and `SkinMesh` deformation using imported sparse joints. The
primitive fixture covers 18 valid value/selector/base combinations; the joint
fixture covers 12, using only spec-valid UBYTE/USHORT JOINTS_0 components.
Controls reject invalid count/types/order/range, negative/oversized offsets and
truncated views; rejected imports preserve the caller's sentinel document.
Positive float sparse positions, normalized sparse colors and all dense
unsigned index widths pass. Existing containment/reload asset and host tests
also ran unchanged. No assertion or registration was weakened.
Release compilation emitted the existing expected MSVC D9025 notice when the
test helper overrides `/DNDEBUG` with `/UNDEBUG`; the unchanged safety fixture
confirmed Release assertions execute and product Release flags stay separate.

Exact fixed commands from the owned worktree (through the same evidence helper):

```powershell
$sparseCmakeBin = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin'
$sparsePython = 'C:/Users/lucas/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& "$sparseCmakeBin/cmake.exe" --build ../build-sparse-accessors --config Debug --target EngineAssetsTests EngineFrameworkTests EngineSampleTests --parallel 2
& "$sparseCmakeBin/ctest.exe" --test-dir ../build-sparse-accessors -C Debug -R '^(EngineAssetsTests|EngineFrameworkTests|EngineSampleTests)$' --output-on-failure -V
& "$sparseCmakeBin/cmake.exe" --build ../build-sparse-accessors --config Release --target EngineAssetsTests EngineFrameworkTests EngineSampleTests --parallel 2
& "$sparseCmakeBin/ctest.exe" --test-dir ../build-sparse-accessors -C Release -R '^(EngineAssetsTests|EngineFrameworkTests|EngineSampleTests)$' --output-on-failure -V
$env:PATH = "$sparseCmakeBin;" + $env:PATH
& $sparsePython Scripts/test_test_safety.py
```

PATH was changed only in the safety command's process. All five fixed receipts
bind to Engine/Tests patch SHA256
`cf0c19b85c71b28dae32db295da2a844e44e78d622161ec0294ea5f6756afd1f`.
There were no unexpected failures or source changes during fixed gates, and no
redundant consumer reruns after full suite success. Post-run inspection found
no compiler/linker/CMake/CTest or affected suite process, and the coordinator was
notified of slot release immediately. `git diff --check` passed.

## Review, scope and limits

Independent reviewer `/root/containment_implementation` reviewed the exact
Engine/Tests patch above, read-only, and reported clean: raw integer decoding,
sparse slots, count/type/order/range, float/normalized behavior and transactional
publication agree with the contract. Reviewer ran no builds/tests and edited no
files. Final committed source/document/evidence binding remains a separate
coordinator gate; this record does not self-approve adoption.

Only Gltf.cpp, EngineAssetsTests and this packet's task/QA files change. All
fixtures are original, small, in-memory engine data. Existing C/D/E read/watch
and warning behavior is preserved; broader glTF schema/matrix-layout validation
is outside this bounded fix. Earlier reload/CI/local-editor worktrees remain
untouched. No full all-target, interactive native, GPU or hosted execution was
performed in this slot; those integration gates belong to the coordinator.

Primary Khronos specification/schema links are recorded in the task. No
animation runtime completion, full engine conformance, hosted acceptance or
merge claim is made.
