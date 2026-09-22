# AnimeRPG / Astral Engine — creative production and starter-content plan

**Decision date:** September 21, 2026, America/New_York.  
**Status:** researched plan plus original procedural engine fixtures; not production-art approval.  
**Audited candidate:** `4b37959f5fbbccc387fc8ef54dbe753f66e27958` (PR #10), dependent on PR #8 and #6.  
**Main at audit:** `771b61ac116dfa4a70d81b53a390f672aaf3bf4e`.  
**Entry point:** [Starter pack and tutorial](../../Content/Starter/README.md).  
**Verification:** [Author QA and remaining gates](../QA/CREATIVE-STARTER-2026-09-21.md).

## 1. What this change delivers, and what it does not

The immediate deliverable is a reproducible archive containing **32 original
wireframe meshes, five PNG texture-source files, 16 material recipes, and a
SHA-256 manifest**. Source generation and independent validation scripts accompany
it. The geometry uses the actual `ASTRAL_MESH 1` format. Textures are staged source
assets because Astral does not yet render them. Material recipes are not shader
implementations. The static training dummy is not a rigged mannequin.

The pack is for engine tests, scale checks and future tutorials. It is not equivalent
to UE5 Starter Content in visual quality, breadth or editor integration. Neither
passing a loader test nor producing many asset names establishes that equivalence.
There are **zero delivered skeletal characters, animation clips, particle systems,
audio clips or loadable tutorial scenes** in this change.

The retained game vision is a modern supernatural Washington, DC National Mall
action RPG. The backlog includes Shadowblade, Arc Mage and Aegis; male/female
character art; Shadow Crypt and Mana Reactor dungeons; enemies, a summon, and
destruction. These are product intentions, not all implemented features. The
quantum-experiment/mana premise is fictional world-building, not established
physics. Preserve an internally consistent science-fiction explanation rather
than using scientific vocabulary to excuse arbitrary gameplay rules. [R1, R2]

**Scope boundary:** the current director controls pause RPG production art,
encounters, narrative, audio content and game playtests until engine acceptance.
This request adds planning and generic original engine fixtures, not a silent
unpause. Do not change that control file, merge the parent stack, run R0, start a
local scheduler, install dependencies or spend provider credits under this plan.
Any future production batch needs its own admitted task and Lucas's approval. [R1]

## 2. Compatibility audit before making expensive assets

| Area | Observed support at the pinned candidate | Creative consequence |
|---|---|---|
| Geometry | `ASTRAL_MESH 1`: positions and zero-based edges only; bounded loader in PR #8 | Generic wireframes can be loaded. No triangle faces, UVs, vertex normals or tangents in this format. |
| Display | Win32/GDI perspective wireframe game and separate editor shell | A PNG or PBR recipe cannot become a visible textured material merely by copying it into the repository. |
| Coordinates | World blockout maps controller `(x,y)` to world `(x,0,y)` | Use Y-up fixtures. Do not assume Unreal's transform convention or copy its scale silently. |
| Editor | Candidate Outliner, Inspector, viewport and primitive browser; editing/play controls remain pending | Do not claim drag/drop, import, editable transforms, saved scenes, Play/Stop or this pack's browser registration. |
| Characters | No demonstrated production skinning, skeleton import, morph/face or animation pipeline | Keep character generation in a quarantined authoring stage; first prove one rig through the runtime. |
| Materials/lighting | No demonstrated textured/PBR GPU pipeline | GPU API, texture decode/upload, shaders and lighting are engineering prerequisites, not assets an image model supplies. |
| Other media | No demonstrated production VFX, audio, genuine 2D or package pipeline | Plan sources and tests now; do not rename files into unsupported runtime formats. |

Evidence is from source inspection, not a live Windows host. The sibling
`AnimeRPG-UE5` is a separate minimal movement experiment, not the production
runtime or a source of transferable Epic assets. Existing PR #6/#8/#10 acceptance
is still separate from this pack's verification. [R1–R4]

### Proposed production interchange contract — not yet implemented

Keep editable authoring files outside the runtime package, and admit a documented
interchange format only after an importer packet is approved. **glTF 2.0/GLB is a
candidate**, not an existing supported format. FBX, VRM, Blender files, USD and
Unreal `.uasset` files must never be described as directly loadable today.

For this starter pack, one unit represents one metre, +Y is up, and bounds describe
the pivot. This is a pack convention, not a retrospective claim that every existing
game coordinate is a surveyed metre. Test a one-metre cube, asymmetric axis marker,
known camera and normal-map sphere before accepting any future axis/handedness
conversion. Keep conversion at the import boundary; do not scatter sign flips
through shaders and scene code.

Future asset records should include: stable ID; source and output hashes; exact
model/checkpoint and tool versions; prompt/reference IDs and seeds where supported;
source URL and license snapshot; author/reviewer; purpose; unit/pivot transform;
LOD and collision relationships; dependencies; texture channel/color-space rules;
approved shader family; CPU/GPU memory estimate; QA state and rollback revision.
Runtime states must progress through **source → validated → imported → runtime
verified → art approved**. A generator may not promote its own work to the last
two states without independent evidence.

For future PBR import, propose sRGB base color and emissive color; linear roughness,
metallic, occlusion and normal data; packed ORM as R=occlusion, G=roughness,
B=metallic; documented positive-Y tangent-space normals and tangent basis. The
starter normal map is flat, not a reconstruction of surface relief. Alpha modes
must be explicit. Do not assume glass, water, hair, foliage or emissive effects work
because a JSON field mentions them. Build each shader family with its own tests.

## 3. Creative work breakdown for the game

The quantities below are **planning allowances for a later vertical slice**, not
approved purchases, existing assets, UE counts or estimates of the entire game.
Before production, reconcile them with the retained backlog and a signed-off art
bible. Prefer fewer complete, reusable assets over hundreds of inconsistent outputs.

| Workstream | Concrete deliverables | Initial planning allowance | Acceptance dependency |
|---|---|---|---|
| Art direction | Original silhouette language, palette, line/shadow rules, material response, anatomy, costume and architecture reference boards; examples of what not to make | One art bible; six representative look-development sheets | Lucas's style approval; consistent in-engine test lighting |
| Main characters | Male/female body bases, head/hair, eyes/teeth, skin and costume materials, class-specific silhouettes and equipment | Two body bases; three class looks per base, sharing assets where sensible | One accepted humanoid skeleton, skinning, face and material pipeline |
| Class equipment | Shadowblade weapon, Arc Mage focus, Aegis protection equipment; grips, sockets and readable silhouettes | Three primary equipment families plus approved variants | Rig sockets, collision and animation alignment |
| Enemies and bosses | Melee/ranged/heavy/support silhouettes; clear wind-ups, weak points, damage state and death treatment | Six enemy archetypes and two bosses proposed, not approved | Combat readability, navigation, animation and performance |
| Summon/familiar | Distinct silhouette, summon/dismiss effect, follow/idle/movement, interaction, class relationship | One summon and one familiar concept slot | Scope confirmation, rig/AI/animation support |
| Animation | Locomotion starts/stops/turns, idle, directional movement, light/heavy attacks, dash, guard, hit reactions, defeat, interaction and class actions | Inventory actions first; target 20–30 shared humanoid clips before variants | Root-motion policy, event timing, blending and deformation QA |
| Facial/cinematic acting | Expressions, lip/eye controls, blinks, gaze, dialogue poses and camera staging | Eight core expressions proposed; cinematic count deferred | Face rig, dialogue scope and consented voice workflow |
| National Mall | Ground/paths, Reflecting Pool, landmark silhouettes, stairs, columns, vegetation, street furniture, signage, lighting and distance views | First three existing proxy landmarks; a separate full-Mall coverage inventory | Approved geographic reference and scale; scene/streaming/collision |
| Shadow Crypt | Modular floor/wall/corner/door/stair kit, landmarks, encounter dressing and environmental storytelling | One reusable kit, roughly 20–30 distinct modules | Scene assembly, lighting, nav and encounter acceptance |
| Mana Reactor | Modular structural kit, machinery, conduits, lab props, portal apparatus and hazards | One reusable kit, roughly 20–30 distinct modules | Same gates; distinct visual language from Crypt |
| Shared props | Benches, barriers, lights, crates, doors, debris, rails and interaction targets | 20–40 reusable silhouettes before variants | Correct scale, pivots, LODs and collision |
| Surfaces | Stone, concrete, brick, metals, wood, fabrics, foliage, skin, hair and supernatural materials | 12–20 foundational material families, then approved variants | Texture/PBR or toon-material pipeline and neutral light rig |
| Destruction | Intact/broken states, fracture interiors, debris, dust, collision proxies and cleanup rules | One wall, one prop and one environmental breakable first | Bounded destruction/physics implementation, not just broken-looking art |
| VFX | Portal, mana flow, class casts, weapon trails, hits, dodge, guard, summon, dust and ambient effects | Eight essential gameplay effect families before cosmetic variants | Particle/trail/decal systems, transparency and overdraw budgets |
| Sky/background/lighting | Day/night mood boards, sky or licensed HDRI, cloud layers, distant skyline, fog, exposure and weather direction | Three lighting looks; one approved outdoor baseline | Sky/lighting renderer; fixed exposure and color-management checks |
| UI and genuine 2D | HUD, health/resource/cooldown feedback, menus, settings, inventory, map, icons, cursor and tutorial prompts | One UI kit; 24–40 functional icons proposed | UI/input/text stack, scalable layout and accessibility |
| Sound/music/voice | Footsteps and surfaces, weapons, abilities, enemies, interfaces, ambience, dungeon motifs and dialogue | Event-driven sound list before recording; music/voice scope deferred | Audio engine, mix groups, captioning and rights approval |
| Scene composition | Traversal lanes, focal points, occlusion, encounter readability, spawn placements and level dressing | One end-to-end representative scene before bulk world production | Working scene save/load, collision, nav and real runtime review |
| Presentation | Turntables, orthographic sheets, comparative screenshots, trailer storyboard and store/key art | One honest visual-slice presentation after acceptance | Captured runtime must be labeled separately from offline concept renders |

### The full National Mall is not three landmarks

Keep a coverage ledger with geographic extents, reference source/date/license,
terrain/building/landmark layers, accuracy class, traversal bounds and what is
intentionally simplified. Existing landmark proxies are a starting slice, not a
survey or a complete in-game Mall. Approve playable boundaries before committing
to full-detail interiors or off-route buildings. Split the world into reusable
modules and independently testable areas once streaming/scene ownership exists.
Do not extract proprietary map imagery or copy scanned buildings without checking
rights. An internet reference image is not automatically a redistribution license.

### Proposed style direction to review

Use grounded contemporary materials and architecture, anime-readable characters,
controlled shadow shapes and restrained surface noise. Give each class and enemy
role a distinctive silhouette and effect shape, not just a different hue. Reserve
the strongest contrast and motion for gameplay information. Make supernatural
energy feel like a consistent phenomenon with repeatable visual rules. Avoid
baking dramatic shadows into base-color textures that will later be lit again.
Do not rely solely on color for hazards, damage states or class identification.

## 4. Models and tools worth testing

These are researched **candidates**, not hands-on benchmark winners. No listed
external generation service or local model was executed for this change. The
starter pack was generated deterministically with Python. Provider terms and
hardware requirements must be rechecked at execution; no API key, paid credit,
model download or software installation is authorized here.

| Candidate | Appropriate role | Limits and production path |
|---|---|---|
| **GPT Image 2.5 Sunburst / Flare** | Concept exploration, reference-sheet iteration, UI and background drafts. Official docs list `gpt-image-2.5-sunburst` and `gpt-image-2.5-flare` for generation/editing. [S1] | A compelling image is not a consistent multi-view character, mesh, PBR set or rig. Use approved reference sheets and iterative edits; check layout/text manually. Hosted usage requires approved access and budget. |
| **FLUX.2 [klein] 4B** | Local concept/image iteration candidate; BFL lists this variant as open-weight Apache 2.0. [S2] | Do not extend that license claim to every FLUX variant. Measure actual peak memory, image consistency and speed on the admitted machine. It is not a 3D mesh model. |
| **Hunyuan3D-2.1** | Image-conditioned prop/organic shape and PBR-texture candidate. Official repository documents separate shape and paint stages. [S3] | Documented VRAM: 10 GB shape, 21 GB texture, 29 GB combined. Shape-only is the plausible first experiment on a nominal 16 GB GPU; full textured inference is not a proven fit. Check the exact model license and dependencies. Retopology, UVs, collision and runtime import remain required. |
| **TRELLIS.2** | Image-to-3D/PBR candidate for static assets where higher-capacity hardware is available. Official project lists 4B parameters and MIT code/model licensing. [S4] | Official setup is Linux with at least 24 GB NVIDIA memory; do not promise it runs unchanged on 16 GB. Its example high-poly export is not a sensible game budget. Dependency licenses and generated-asset provenance still need review. |
| **Meshy 6** | Hosted prop and initial character-shape comparison candidate, including documented low-poly and API workflows. [S5] | Vendor claims of improved geometry are not proof of game-ready deformation or human-level artistry. Require account/credit/export-rights approval, then test topology and cleanup time. No connected generation tool was available in this session. |
| **VRoid Studio + artist cleanup** | Controllable anime humanoid base workflow. VRoid documents parameterized bodies, hair and clothing, texture editing and VRM export. This is an authoring tool, not a text-to-3D model. [S6] | Review preset/third-party item terms separately. Astral does not import VRM today. A technical artist must adapt skeleton, topology, hair, materials and export contract. This is a strong consistency baseline to compare with unconstrained generation. |
| **Substance 3D Sampler** | Convert a suitable source image into an editable material starting point. Adobe documents AI generation of normal/height/roughness and removal of illumination; B2M is a separate procedural algorithm. [S7] | Neither inferred relief nor metallic classification is ground truth. Inspect tiling, lighting removal, physical response and channel conventions. Requires authorized software access and a renderer that supports the result. |
| **Cascadeur + conventional animation editing** | AI-assisted keyframe animation candidate for locomotion, combat posing and secondary motion. [S8] | Not a substitute for combat timing, contact cleanup, deformation tests or retargeting. Prove one animation through Astral before licensing or producing a large library. |
| **Blender/manual/procedural authoring** | Recommended editable production hub for mesh cleanup, UVs, rigging, modular scene construction and baking; procedural generation for repeatable architecture | Treat this as a pipeline recommendation, not a claim that Blender is installed or integrated. Pin the actual version and approved export tools in a later packet. |
| **Poly Haven** | Human-authored comparison/reference candidates and a possible later source for HDRIs, materials and props. Its asset library license is CC0. [S9] | Select individual relevant assets, keep provenance, and adapt style/performance. No library was downloaded here. CC0 assets are not evidence that Astral can render them. |

For Lucas's machines, the useful distinction is **local concept/shape experiments
versus full textured-3D production**. The documented 21/24/29 GB paths should not
be scheduled blindly on a 16 GB card or split across different GPUs as if memory
were automatically pooled. Actual free VRAM, system RAM, model precision and
supported offload need a fresh local probe. A second model worker competing for
the same GPU is a resource decision, not free capacity.

Do not select a cloud vendor on an unverified per-asset price. Record dollars per
**accepted asset**, including unsuccessful generations, reference-image calls,
retopology, baking, review and revision. A cheap generation with hours of cleanup
may lose to a human-authored or parametric asset.

## 5. A defensible test for “as good as a human creative”

No source reviewed establishes that one model replaces a professional concept
artist, modeler, rigger, animator, material artist and environment artist across
this game's requirements. Use a repeatable acceptance process rather than a
marketing label. The goal is an asset that reaches an agreed professional bar,
regardless of whether its creation began with a model or a human.

After approval, commission or select an appropriately licensed human-authored
reference for four matched briefs: an anime character turnaround plus deformation
poses; a modular architectural prop; a tileable material under neutral light;
and a short combat action with clear foot/weapon contact. Add a composed scene
only once Astral can render all of its constituent assets. Match camera, lighting,
output resolution, triangle/material budgets and revision allowance.

For each admitted model/workflow, retain three attempts per brief, including failed
ones. Do not select the one unusually good output and hide the failure rate. Use
two independent reviewers, anonymized workflow labels, and a 1–5 rubric for style
fit, silhouette/readability, anatomy or geometry, consistency and finish. A proposed
art gate is a mean of at least 4/5 with no critical category below 3/5 and no
unresolved reviewer objection. These are proposed project thresholds, not a
published result or a substitute for Lucas's taste.

Technical gates are mandatory and separate: valid topology/UVs, acceptable
silhouette at each LOD, no inverted or missing surfaces, correct scale/pivots,
working collisions, stable material response, required motion and deformation,
no material/shader fallbacks, and measured frame/memory budgets. For materials,
inspect a 3×3 tiled view and grazing light; for characters, inspect fingers, eyes,
shoulders, elbows, hips and knees through action poses; for scenes, inspect actual
third-person traversal and camera occlusion rather than only a flattering still.

Track acceptance rate, generation minutes, artist cleanup minutes, review minutes,
revisions, cost per accepted asset and reproducibility. Promote a workflow only
when it meets both visual and technical gates across the batch. Preserve rejected
assets and reasons in quarantine; do not import them into the approved catalog.

## 6. UE5-like defaults: define the comparison correctly

Epic's versioned Starter Content documentation describes meshes, materials,
particles and sounds, along with a StarterMap and advanced lighting example. That
is a useful category reference, but the inspected page is explicitly **UE 5.1**.
It is not an exact inventory of an installed UE 5.8 project. Engine content,
Starter Content, project templates and separately acquired Fab content are four
different scopes. Do not collapse them into one imaginary default count. [S10]

Use **functional equivalence**, not copied names, meshes, textures or animations.
Epic's EULA distinguishes Starter Content from Examples and other assets and
places conditions on distribution. This plan does not authorize extracting or
redistributing Epic content into Astral. Asset-by-asset terms matter; do not claim
that everything available through Epic has the same license. [S11]

| Reference function | This pack | Gap before a professional starter library |
|---|---|---|
| Basic forms and scale tests | Cube, plane, sphere, cylinder, cone, capsule, ramp, pyramid, octahedron, disc, axes and grid | Solid faces, UVs/normals/tangents, collision, LODs and working placement tools |
| Modular architecture | Floor, wall, doorway, window wall, pillars, beam, stairs, arch, platform and fence | Production topology, trim materials, snapping, collision/nav and construction tutorial |
| Generic props | Crate, barrel, table, chair, bench, pedestal, bollard and static training dummy | Detailed, optimized, textured meshes and art review |
| Materials and diagnostic textures | 16 source recipes in two atlases, flat normal, checker and UV diagnostic | True tileable higher-resolution PBR/toon sets, shader graph/instances and runtime material editor |
| Character template | No skeletal character | Original rigged mannequin, animation graph, camera/input and locomotion template |
| Effects | None | Original fire, smoke, sparks, dust, impacts, trails, portal and test scenes |
| Audio | None | Original/licensed cues, ambience, mixing, spatial audio and tutorial |
| Starter map/light stage | No loadable scene | Scene serialization plus material gallery, lighting stage and generic playable sandbox |
| 2D starter template | None | Sprite import, atlas handling, camera, collision, input, UI and a genuine 2D sample |
| Onboarding and packaging | Source-level verification walkthrough | Interactive editor onboarding, save/reopen, playable template and self-contained package |

### Quantity and quality gates

First capture a read-only inventory from Lucas's **chosen, legally available Unreal
version and selected content pack**. Record engine build, template/pack identifier,
package paths and asset classes; export metadata only, not copyrighted asset data.
Count unique functional meshes, material families, animations, effects, sounds and
example levels separately. Do not inflate Astral counts with recolors, LOD files,
JSON metadata, archive entries or copies of the same asset.

Freeze that inventory as the comparison baseline. Set targets per category and
track `accepted_Astral / reference_count`, with an explicit “baseline not captured”
state until real counts exist. This work does **not** provide an exact UE inventory
or a parity percentage. The current 16 material recipes represent 16 surface
swatches, not 16 complete shader systems or 48 separate texture files.

For quality, compare fixed views, neutral and grazing lighting, silhouette, texel
density, seams, deformation, collision and tutorial usability. Measure memory and
frame time in Astral and the reference using equivalent content and settings;
separate asset quality from renderer capability. A high-quality offline render
cannot establish runtime quality, and a green file validator cannot establish
art quality. Never mark a category complete from a screenshot alone.

The fastest later path to credible quality is a small hand-reviewed combination
of original procedural geometry, appropriately licensed human-authored materials
and model-assisted drafts that survive cleanup—not a mass download or thousands
of unchecked generations. Make the acceptance bar repeatable before scaling up.

## 7. Production workflow and ownership

Use the existing coordinator and task machinery; do not introduce another scheduler.
A creative director owns briefs and taste decisions. A concept worker proposes
reference sheets. A modeling/technical-art worker owns topology, UVs, materials,
rigging and export. Animation, environment, VFX and audio specialists receive
separate bounded packets only when their prerequisites exist. An independent
reviewer owns acceptance evidence. These are responsibilities, not instructions
to launch seven simultaneous GPU jobs.

Each asset packet must name: allowed paths; asset IDs and exact count; references
with permissions; output format; geometry/texture/rig budget; lighting and camera
for review; success tests; retries/cost limit; quarantine and approved destinations;
and required receipts. Untrusted downloaded/generated files must remain outside
shipping content until validated. Disable executable scripts or autorun behavior
in received source files. No secrets or personal data belong in the asset manifest.

Keep source files, generated caches and approved runtime assets distinguishable.
Once large production binaries are necessary, approve Git LFS or an artifact store
in its own change, with reproducible acquisition and hashes. This small pack uses
an ordinary TAR.XZ plus inspectable generator because its binary archive is only
about 8 KB. No storage migration or license grant is smuggled into this patch.

### Provisional resource budgets

These are starting hypotheses for later measurement, **not engine capacities**:
50–80k triangles for the main character near camera; 15–40k for a common enemy;
0.5–10k for ordinary props; one or two materials for repeated small props;
1–2K textures for common material families, with 4K reserved for justified hero
needs. Build LOD and collision plans at the same time as the high-detail asset.
Set actor-count, draw-call, transparency/overdraw, animation and texture-residency
budgets from an actual representative scene, not independent per-asset wish lists.

Choose a target machine and a frame-time objective with Lucas before approving
content volume. Log CPU/GPU frame time separately, peak/resident RAM/VRAM, loading
spikes and 1% worst-frame behavior. The current GDI prototype cannot validate a
modern GPU renderer's performance budget. Preserve the existing native stress,
load/unload, failure-recovery, soak and independent acceptance requirements. [R1]

## 8. Dependency-ordered packets

1. **This packet: planning and generic fixtures.** Deliver original archive,
   generator, manifest, format/probe tests, source tutorial and truthful gaps.
   Keep the branch a draft while independent/native gates remain open.
2. **Asset/renderer contract.** After approval, choose the GPU/rendering boundary,
   triangle data, texture decoder, shader/material model and safe importer.
   Acceptance: textured asymmetric cube and normal-map calibration object survive
   import, rendering, failure tests and load/unload under measured budgets.
3. **Asset catalog and scene document.** Stable IDs, import metadata, dependency
   tracking, placement, transforms, save/reopen and undo. Acceptance: save a room
   assembled from the starter kit, restart, reopen, and compare transforms and
   assets without lost references. Do not pretend the current shell already does this.
4. **Professional default-material and prop slice.** Replace a few swatches and
   wireframes with approved triangle/PBR or toon assets. Prove gallery lighting,
   collision, LODs and package loading, then expand the reference coverage table.
5. **Character/animation and genuine 2D templates.** Separate bounded tasks;
   original rigged mannequin and locomotion first, and a real sprite-based test
   project with its own camera/input/render behavior. No copy of Epic's mannequin.
6. **Interactive tutorials and packaging.** Material gallery, modular-room build,
   third-person playground, 2D example and lighting stage. Every instruction must
   correspond to a working UI action; maintain screenshots and versioned expected
   results. Verify a clean machine and a package launched outside the source tree.
7. **Engine acceptance and production approval.** Only then admit the RPG art
   bible, human/model bakeoff and one character-plus-scene vertical slice.
8. **Scale production.** Expand the National Mall, the two dungeons, enemy set,
   class variants, effects and audio after the representative scene proves the
   complete asset-to-runtime path. Keep full-world scope and costs under review.

No completion date is invented for these packets. The next useful engineering
result is a verified import/render/scene workflow—not another batch of hero
characters that the engine cannot display.

## 9. Sources and evidence boundaries

Primary sources accessed for this task on September 21, 2026 local time (some
service responses use September 22 UTC). Vendor capability statements are not
independent quality benchmarks. License summaries are workflow screening, not a
blanket legal determination. Recheck exact terms before acquisition/distribution.

### Repository records

- **R1:** `AGENTS.md`, `GAME_DEVELOPMENT_CONTROL.md`, `Docs/Project-Status.md` at the audited candidate; PR #6/#8/#10 status and acceptance boundaries.
- **R2:** `Docs/Planning/MILESTONES.md` / retained product backlog as reconciled in `Docs/Project-Status.md`; implementation M8/M9/M10 labels are not original backlog completion percentages.
- **R3:** `Engine/Assets/StaticMesh.cpp/.h`, `Engine/Math/Math.h`, `Engine/Scene/WorldBlockout.cpp`, editor PR #10. Loader/math test fixture hashes are in the QA report.
- **R4:** `LucasKazaki/AnimeRPG-UE5`, `README.md` on `master`, inspected as a separate experiment.

### External primary references

- **S1 — OpenAI, image generation guide:** https://developers.openai.com/api/docs/guides/image-generation
- **S2 — Black Forest Labs, deployment/model options:** https://bfl.ai/enterprise
- **S3 — Tencent Hunyuan3D-2.1, requirements and pipeline:** https://github.com/Tencent-Hunyuan/Hunyuan3D-2.1
- **S4 — Microsoft TRELLIS.2, requirements, export and licensing:** https://github.com/microsoft/TRELLIS.2
- **S5 — Meshy 6 official release:** https://www.meshy.ai/blog/meshy-6-launch
- **S6 — VRoid Studio, creation and export capabilities:** https://vroid.com/en/studio
- **S7 — Adobe, Image to Material algorithms:** https://experienceleague.adobe.com/en/docs/substance-3d-sampler/using/filters/tools/image-to-material
- **S8 — Cascadeur, AI-assisted keyframe animation:** https://cascadeur.com/
- **S9 — Poly Haven asset license:** https://polyhaven.com/license
- **S10 — Epic Starter Content, version-pinned UE 5.1 category reference:** https://dev.epicgames.com/documentation/unreal-engine/starter-content-in-unreal-engine?application_version=5.1
- **S11 — Unreal Engine EULA, Starter Content/Examples/distribution distinctions:** https://www.unrealengine.com/eula/unreal
