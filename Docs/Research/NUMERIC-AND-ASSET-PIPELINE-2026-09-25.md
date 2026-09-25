# Numeric contracts and the missing asset-to-runtime pipeline

Research checked September 25, 2026. This document separates upstream facts,
observed repository defects and proposed implementation. It is not a claim of
GPU, importer, voice, animation or top-tier parity. Read the
[audit receipt](../QA/REPO-AUDIT-2026-09-25.md) and
[source ledger](research-source-ledger-2026-09-25.json).

## 1. Numeric inputs need a domain contract

The C++ working draft specifies that converting a floating value outside the
representable destination integer range has undefined behavior [N1]. Microsoft's
cmath reference includes the three-argument hypot overload used here [N2]. The
Linux hypot documentation explains why intermediate squaring should not cause
avoidable overflow or underflow [N3]. Those are library/number-system facts,
not proof that arbitrary float values are valid game inputs.

In this audit's actual source reproductions, a NaN time step corrupted idle player
coordinates, projection reported success with NaN output, large finite vector
components overflowed during squaring, and long elapsed recovery wrapped posture.
Invalid combat enum values also caused real state mutations. See the receipt for
before/after output and exact tests. The following contracts are implemented:

- Reject nonfinite movement/position inputs before changing actor state. Validate
  finite ordered movement bounds and nonnegative speed at construction. Invalid
  configuration throws invalid_argument; zero speed and degenerate bounds remain
  valid. Existing callers keep their signatures and ordinary XY movement.
- Use hypot for vector length and double intermediates for finite movement sums,
  bounds midpoints and projection. Narrow only after the applicable range check.
- Checked camera projection leaves output unchanged on failure, including NaN
  parameters and coordinates unsafe for signed 32-bit raster conversion. A
  4,096-unit headroom is deliberately reserved for rounding and existing small HUD
  offsets. This is not viewport clipping or a universal large-coordinate renderer.
- World-position traversal detects parent cycles and is iterative, avoiding
  recursive stack growth. TryWorldPosition reports failure without changing output;
  the compatible WorldPosition wrapper returns a nonfinite sentinel. Cameras use
  the checked interface. Raw parent-pointer lifetime is still the caller's duty.
- Reject unknown combat actions, affinities and settings without applying damage,
  consuming openings or resetting persistent combat state. LastAttack remains an
  observable attempt report. Unknown Definition queries return a zero definition.
- Check the accumulated clock against a safe int64-microsecond horizon before
  accepting a delta. Leave deadline arithmetic headroom. Clamp recovery to the
  actual posture available BEFORE narrowing, and saturate displayed DPS to a
  finite float maximum. This is not a redesign of simulation scheduling.

Do not enable fast-math assumptions that invalidate NaN checks without a separate
review. Do not extend these tests into a claim of safe dangling pointers, arbitrary
untrusted object graphs, or all floating arithmetic elsewhere in the repository.
Translation-only Transform behavior is retained; scale/rotation composition is a
future contract, not silently implemented by summing translations.

## 2. Correct the difference between a parser and an asset pipeline

The cgltf README says parsing does not load external buffers or images by default;
loading those resources is a distinct operation [A2]. bgfx describes itself as a
rendering library for an existing engine/framework and documents native-window
integration and threading constraints [A3]. Neither upstream description is proof
that Astral already has a material pipeline, editor, physics or character rig.

The glTF specification defines a right-handed, meter-based system with +Y up,
+Z forward and -X right. Node hierarchies cannot contain cycles or multiple
parents. Base-color RGB is sRGB, whereas metallic/roughness and normal data use
linear transfer functions; base-color alpha is linear [A1]. These constraints
must become importer tests rather than being inferred from a plausible screenshot.

### Proposed bounded importer profile, not installed by this patch

Start with ONE explicit static-mesh profile: glTF 2.0, indexed triangles, supported
vertex attributes, one opaque material, one embedded buffer and a reviewed texture
representation. Reject unsupported required extensions; do not silently ignore
features that change geometry or appearance. A parser's feature list is not our
supported runtime profile. Preserve source metadata separately from cooked data.

Specify byte/vertex/index/image limits, integer-only counts, checked offset/stride
arithmetic, index bounds, finite attributes, hierarchy depth/count limits and
ownership of every allocation. Decode only after validation. Test corrupt, tiny,
empty, oversized and truncated inputs. A future external-resource mode must enforce
asset-root containment after URI decoding/canonicalization and reject network,
absolute and escaping paths. Embedded data still needs decoded-size limits.
These are project admission requirements, not claims that cgltf is unsafe.

Choose one canonical world convention and one reviewed import conversion. The
current game uses XY movement and maps it to XZ presentation. This audit preserves
that behavior, so glTF coordinates cannot just be substituted into every combat
or camera call. Keep stable authored IDs for nodes and later hit regions; names
and array positions are not durable gameplay identities. Model scale, normals,
mirrored transforms, tangent handedness and texture orientation need independent
asymmetric fixtures and actual in-engine checks.

Define resource states: unloaded, CPU-validated, upload-pending, GPU-ready, failed,
and retired. Give every request an owner and generation; stale completion after
scene unload must not install a resource into a reused handle. Keep upload memory
alive until the backend's ownership contract permits release. Bound caches and
queues. Hot reload needs atomic replacement and a recoverable old resource, not
partial overwrite. File validity and GPU readiness are separate acceptance states.

### Backend decision and first proof

The remaining decision is not 'which free engine replaces Astral'. Keep Astral
and evaluate a native graphics API path versus bgfx under a small renderer
interface. Compare Win32 integration, shader tools, supported compiler versions,
resource ownership, debug capture and native maintenance cost. No option is
selected or installed by this audit. bgfx's BSD-2-Clause license and supported
backend list do not confer advanced renderer features or console SDK rights [A3].

Acceptance sequence: one imported triangle mesh with depth and UVs; one correctly
sampled material; camera resize and near clipping; one light; destroy/reload with
no stale resources; then edit, undo, save/reopen and Play. Only after that should
skeletal animation, facial rigs, actual part colliders and streaming expand the
profile. Record source/asset hashes, screenshots, CPU/GPU timings and memory from
a native build. No timing number, image quality score or completion increment is
claimed from writing this sequence.

## 3. Voice research needs a model/interface matrix

Qwen's documentation separates CustomVoice instruction-driven preset speakers,
VoiceDesign descriptions and Base voice cloning. It gives a design-then-clone
workflow with reusable prompts and lists English/Japanese support [V1]. Do not
assume Base has every CustomVoice emotion-control capability merely because both
are called Qwen3-TTS. The documented API is a candidate interface, not an audition.

The project pilot should freeze a character reference, then compare neutral and
emotional lines in both languages with exact model/reference versions. Record
identity drift, pronunciation, failures and native listening review. Treat a
model accepting Japanese as different from a convincing Japanese performance.
Approve each character/locale identity; do not clone a real actor without the
appropriate permission. Generate reviewed assets offline. No voice model,
reference recording, paid API or speech output is added here. Earlier paid
alternatives remain outside Lucas's new required free-tool baseline.

## 4. CI evidence must identify what actually ran

GitHub documents pull_request's default merge-result SHA separately from
`github.event.pull_request.head.sha` [C1]. Our dedicated numeric workflow explicitly
checks both head and merge result on Windows/Linux Debug/Release, plus a Clang
sanitizer job on the exact head. Every job verifies its checkout before testing.
This is configured coverage; hosted success is not asserted until a run completes.
The workflow uses read-only permissions and does not persist checkout credentials.

The root Windows pipeline remains unchanged and must still check broader combat,
missions and interactions. The new standalone suite directly runs its 45 cases
and the existing scene regression, instead of relying on a root build that would
not discover it. Neither pipeline replaces interactive Windows/GPU acceptance.

## 5. Research and status corrections from this audit

README's blanket content pause contradicted the later operator update already in
AGENTS/control. Its old test count and unmerged-audit wording were stale. Replace
those with the actual lane boundary, configured-test discovery and dated evidence.
Keep old records historical rather than treating their wording as current status.
The current PR #52 head and workflow still differ from its older green body; do
not inherit that body's acceptance. This audit does not overwrite its owner.

The roughly 10% number in PR #74 remains an author-weighted planning estimate over
an incomplete catalogue, not measured engine completion. No new percentage is
awarded for this audit. Exact Blender/Unreal release pins from earlier research
were not independently cleared here; unsuccessful page retrieval is not evidence
that a version is false. Require official release identity, hash, rights and a
local version receipt before accepting a pinned tool. Do not label uncertainty as
a discovered falsehood, or generalize one model's license to its whole family.

## Primary sources

[N1] C++ working draft, floating-integral conversions (live draft, not a pinned C++17 edition):
https://eel.is/c++draft/conv.fpint
[N2] Microsoft cmath reference, three-dimensional hypot:
https://learn.microsoft.com/en-us/cpp/standard-library/cmath?view=msvc-170
[N3] Linux man-pages hypot, intermediate overflow/underflow behavior:
https://man7.org/linux/man-pages/man3/hypot.3.html
[A1] Khronos glTF 2.0 specification, retrieved header version 2.0.1:
https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
[A2] cgltf official repository, loading section:
https://github.com/jkuhlmann/cgltf
[A3] bgfx official overview, rendering-library scope and integration:
https://bkaradzic.github.io/bgfx/overview.html
[V1] Qwen3-TTS official repository, model matrix and package examples:
https://github.com/QwenLM/Qwen3-TTS
[C1] GitHub workflow event reference, pull_request head versus merge result:
https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows
