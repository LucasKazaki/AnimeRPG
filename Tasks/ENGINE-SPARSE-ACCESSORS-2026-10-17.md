# Sparse integer accessors — October 17, 2026

Owner: `/root/candidate_review`, implementation role for this packet only.
Worktree: `work/astral-sparse-accessors`; branch
`engine/sparse-accessors-20261017`; deliberate base
`4552a31b1669ed34ebb23f05e502af6b425ce696` (reviewed C/D wrapper, tree equal
to `529f52eb221421436a0ec6f1ecb183cfe91f4a50`). Lucas's October 17 engine-only
assignment authorizes this bounded importer repair; technical and independent
review gates remain required. Earlier reload and CI worktrees are frozen.

## Problem and allowed paths

`ReadAccessor` substitutes sparse values in its float array but leaves its
optional raw integer array unchanged. Primitive indices and JOINTS_0 consume
that integer array: a valid sparse triangle can disappear as degenerate, and
a sparse joint replacement can deform a vertex with the wrong joint.

Allowed: `Engine/Assets/Gltf.{h,cpp}`, `Tests/EngineAssetsTests.cpp`, this task
and `Docs/QA/ENGINE-SPARSE-ACCESSORS-2026-10-17.md`. Do not change Scene,
AssetManager, GameHost, animation runtime, renderer, CMake, dependencies, policy,
game content or other worktrees. Scene is owned by a separate prefab-budget
packet. No push/merge or native/GPU execution by this owner.

## Approved contract and approach

1. Use exact unsigned byte/short/int decoding for both dense and sparse integer
   outputs; never round-trip integer data through a float. Sparse values replace
   the same elements in both outputs, including zero-initialized accessors with
   no base bufferView and accessors with a supplied base.
2. Within the existing sparse path, require a positive count bounded by the
   accessor count; selector types 5121/5123/5125; strictly increasing, in-range
   selectors; and valid existing offset/range checks. Keep layout handling
   bounded to the current supported accessors; no matrix-layout rewrite.
3. Preserve all C/D/E containment, URI observations, image-warning policy,
   transactional document publication and custom-loader contracts unchanged.
4. Original disposable data-URI fixtures cover all three unsigned primitive
   index value/selector widths, both valid JOINTS_0 value widths (UBYTE/USHORT),
   all selector widths, dense and absent bases, and nonzero aligned sparse
   offsets. Imported geometry must reach the CPU renderer; imported joints must
   reach existing `SkinMesh`. This does not claim animator/runtime completion.
5. Add bounded malformed sparse cases for count, selector type/order/range and
   offset/view truncation. Preserve the caller's last document on failure and
   retain existing float/normalized and dense importer behavior.

## Sources

Read October 17, 2026, using official Khronos sources (Firecrawl credits had
already been exhausted; built-in web fallback used):

- [glTF 2.0 sparse accessors](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#sparse-accessors): substitutions start from base data or zeros, and selectors are strictly increasing and below accessor count.
- [Sparse schema](https://raw.githubusercontent.com/KhronosGroup/glTF/main/specification/2.0/schema/accessor.sparse.schema.json): positive sparse count and required indices/values.
- [Sparse selector schema](https://raw.githubusercontent.com/KhronosGroup/glTF/main/specification/2.0/schema/accessor.sparse.indices.schema.json): selector component types are unsigned byte/short/int.
- [Sparse value schema](https://raw.githubusercontent.com/KhronosGroup/glTF/main/specification/2.0/schema/accessor.sparse.values.schema.json): replacement values use the accessor's component type and element shape.

## Gates, exact commands and risks

Task/tests were prepared first. Under the coordinator's old-source slot,
configuration and the focused Debug build passed; both separate consumer
regressions failed with actual child exit 3 against unchanged production.
The coordinator accepted those receipts and cleared the bounded production fix.
Source remained frozen during every build/run.

From this worktree, using the installed VS2022 bundled CMake:

```powershell
$sparseCmake = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
& $sparseCmake -S . -B ../build-sparse-accessors -G 'Visual Studio 17 2022' -A x64
& $sparseCmake --build ../build-sparse-accessors --config Debug --target EngineAssetsTests --parallel 2
& '../build-sparse-accessors/Debug/EngineAssetsTests.exe' GltfSparsePrimitiveIndicesReachRenderer
& '../build-sparse-accessors/Debug/EngineAssetsTests.exe' GltfSparseJointsReachSkinMesh
```

The coordinator granted fixed Debug/Release builds of EngineAssetsTests,
EngineFrameworkTests and EngineSampleTests, their registered three-suite CTest
runs, and the unchanged `Scripts/test_test_safety.py`. Both configurations
passed 3/3 suites (63 cases: 29 assets, 14 framework, 20 sample), including both
previously failing consumers and all five added controls. The safety suite
passed 3/3. Full verbose suite logs establish the consumer passes, so no
redundant filtered reruns were executed. Exact commands, stdout/stderr, exit
codes, machine/time and source patches are retained externally in
`work/sparse-evidence`; see the QA record for actual commands and results.
The compiler slot was released before documentation/commit work. Independent
source review is clean on the tested patch; final commit/document binding
precedes adoption. Coordinator owns combined all-target/native/hosted gates.

Risks: sparse changes share float and integer decoding, so valid float and
normalized data need positive coverage. The importer has broader schema and
matrix-layout limits outside this packet. A tiny consumer fixture proves this
repair, not a general glTF conformance or complete runtime-animation claim.
