# Free production toolchain for Astral Engine and AnimeRPG

September 24, 2026. Research and proposed adoption plan, not an installation list.
Read [progress assessment](ENGINE-PROGRESS-2026-09-24.md) and
[scoped task](../../Tasks/FREE-TOOLCHAIN-AUDIT-2026-09-24.md).

## Decision and cost boundary

Lucas requests free tools for the engine and game. Use a zero software-license-fee,
zero required paid-API baseline for authoring, building and running the game on
already available hardware. Do not introduce mandatory subscriptions, trial-only
features, paid export, purchased cloud credits or revenue-triggered middleware
fees. This supersedes the optional paid-provider suggestions in the earlier
creative-production plan and voice PR #71. Existing subscriptions elsewhere are
not evidence that a production dependency is free.

The table identifies a free route for each major production role. It is NOT a
complete bill of materials for a finished engine: most future subsystems are not
yet implemented, so their exact versions and transitive dependencies do not yet
exist to audit. Every new package still needs the admission check below. No tool
listed here was installed or benchmarked, and no provider was called for generation.

Free software does not make electricity, GPU/RAM upgrades, storage, an operating
system license, a domain, hosting, signing, store distribution or human review
free. Retain the existing Windows environment; this report does not supply a new
Windows license. Free local model weights do not promise a fit on existing GPUs.
Do not buy hardware or rent cloud GPUs to make an optional candidate work without
separate approval. Manual/procedural Blender workflows remain the fallback.

## Tool selection by production role

Links point to the official project/license/model source checked for this audit.
Licenses describe the named upstream project, not every plugin, asset, codec or
model that could be attached to it. All new integrations are PROPOSED. Do not
install every item: alternatives and later-stage candidates are labeled.

### Build, runtime and engine infrastructure

| Role | Free baseline or candidate | License/cost condition | Integration still required |
|---|---|---|---|
| Native build, tests, packaging | [CMake/CTest/CPack](https://cmake.org/licensing/) | BSD-3-Clause; use the existing build approach. | Keep external build roots, assertions, exact-head and integration checks. Packaging still needs native launch evidence. |
| Compiler and developer diagnostics | [LLVM/Clang](https://llvm.org/docs/DeveloperPolicy.html); existing MSVC path via [Visual Studio Community](https://visualstudio.microsoft.com/vs/community/) where eligible | LLVM Apache-2.0 with exceptions. Community is free for individual developers; organization eligibility has limits. | Preserve C++17 and tested Windows SDK/ABI assumptions. This is not permission to switch compilers or upgrade the language standard. |
| Build scripts and version control | [Python](https://docs.python.org/3/license.html), [Git](https://github.com/git/git) | PSF license and GPL-2.0 respectively; standalone development tools. | Pin environment and preserve reproducible scripts. No paid coding assistant is required. |
| Window, input and controllers | Retain Win32; [SDL3](https://github.com/libsdl-org/SDL) as a later portability/controller candidate | zlib license for SDL. | Input mapping, rebinding, accessibility and controller ownership remain Astral tasks. Do not replace the current platform layer blindly. |
| GPU abstraction | [bgfx](https://github.com/bkaradzic/bgfx) | BSD-2-Clause. | Candidate below an Astral renderer interface. It does not provide a complete engine, asset editor, Nanite or Lumen. Choose one backend strategy through an architecture decision. |
| Task scheduling | [enkiTS](https://github.com/dougbinks/enkiTS) | zlib; C++11-capable library. | Bounded job ownership, cancellation, thread safety and profiling. Not a second agent/workstation scheduler. |
| Entity/component storage | [EnTT](https://github.com/skypjack/entt), optional | MIT. | First decide whether an ECS is needed. Avoid rewriting existing scene state just to add a library. |
| Structured metadata and saves | [nlohmann/json](https://github.com/nlohmann/json); [SQLite](https://www.sqlite.org/copyright.html), optional | MIT; SQLite public-domain source. | Stable IDs, schemas, migrations, atomic saves and exactly-once rewards are game/engine code, not provided by a file format alone. |
| Mesh and image input | [cgltf](https://github.com/jkuhlmann/cgltf), [stb](https://github.com/nothings/stb) | MIT; stb public-domain/MIT options. | Agree glTF/GLB subset, validation, coordinates, materials, skeletons, error handling and resource lifetimes. Disable unwanted formats/features. |
| Mesh/texture optimization | [meshoptimizer](https://github.com/zeux/meshoptimizer), [KTX-Software](https://github.com/KhronosGroup/KTX-Software) | MIT; Apache-2.0 main project with separately licensed components. | LOD, compression, streaming and GPU upload need a real content cooker and quality tests. Audit optional encoders and bundled dependencies. |
| Asset validation | [Khronos glTF-Validator](https://github.com/KhronosGroup/glTF-Validator) | Apache-2.0. | Run against exact generated/re-exported bytes. A valid file is not evidence that Astral imports or renders it. |
| 3D physics | [Jolt Physics](https://github.com/jrouwe/JoltPhysics) | MIT. | Character controller, contacts, queries, collision layers, body-part mapping and gameplay integration. |
| Genuine 2D physics | [Box2D](https://github.com/erincatto/box2d) | MIT. | Keep a separate 2D scene/collision adapter, not a label applied to a 3D HUD. Match its C API/toolchain to C++17 boundaries. |
| Navigation | [Recast/Detour](https://github.com/recastnavigation/recastnavigation) | zlib. | Navmesh baking/loading, obstacles, path following and behavior selection. A pathfinder is not finished enemy AI. |
| Skeletal animation | [ozz-animation](https://github.com/guillaumeblanc/ozz-animation) | MIT; documented runtime C++17. | glTF conversion, blending, events, root motion, retargeting and authored facial layers. Avoid making the optional FBX SDK a baseline dependency. |
| Audio playback/mixing | [miniaudio](https://github.com/mackron/miniaudio) | Public domain or MIT No Attribution. | Voice/UI/music buses, fades, streaming, spatial policies, localization, stale-callback protection and pause behavior. No FMOD/Wwise dependency is required. |
| Editor interface | [Dear ImGui](https://github.com/ocornut/imgui) | MIT. | Astral must still implement scene editing, gizmos, undo/redo, inspectors, import, asset browsing, save/reopen and Play. Widgets alone are not editor parity. |
| Game interface | [RmlUi](https://github.com/mikke89/RmlUi) | MIT; inspect samples/dependencies separately. | Controller focus, HUD/dialogue layouts, scaling, accessibility and input ownership. Keep game UI distinct from developer panels. |
| Text and Japanese glyphs | [FreeType](https://freetype.org/license.html), [HarfBuzz](https://github.com/harfbuzz/harfbuzz), [Noto CJK](https://github.com/notofonts/noto-cjk) | Select FreeType's FTL option; HarfBuzz MIT-style; Noto OFL-1.1. | Preserve notices, test shaping/fallback, line breaks and IME. Font rights are separate from the renderer. No font files included here. |
| Particle effects | [Effekseer](https://github.com/effekseer/Effekseer) | MIT project with tool/runtime license inventories to retain. | Author effects offline; implement a renderer adapter or sprite-sheet route. No copied game effects or assumed console SDK access. |
| CPU/GPU investigation | [Tracy](https://github.com/wolfpld/tracy/blob/master/LICENSE), [RenderDoc](https://github.com/baldurk/renderdoc), Clang sanitizers | BSD-3-Clause; MIT; LLVM terms. | Measure actual frames, allocations and captures from Astral. Use RenderDoc on our own accepted application, not to extract another game's assets. |
| Optional future multiplayer transport | [GameNetworkingSockets](https://github.com/ValveSoftware/GameNetworkingSockets) | BSD-3-Clause. | Later engine scope, not required for the current single-player slice. Transport does not make servers, relays, accounts or moderation free. |

### Game content, art, audio and localization

| Role | Free baseline or candidate | License/cost condition | Production caveat |
|---|---|---|---|
| Modeling, sculpting, retopology, UVs, rigging, animation, baking and procedural environments | [Blender](https://www.blender.org/features/), [license](https://github.com/blender/blender/blob/main/COPYING) | GPL standalone authoring application. | Use original/cleared source material and approved export tools. Build deformable rigs, collisions and LODs; a pretty offline render is not runtime acceptance. |
| Concept art, portraits, hand-painted textures and UI art | [Krita](https://krita.org/en/about/license/) | GPL application; artwork rights handled separately. | Consistent original style, editable source and explicit approval. No subscription art package required. |
| Procedural materials | [Material Maker](https://github.com/RodZill4/material-maker) and Blender | MIT for Material Maker. | Its authoring application uses Godot; this does NOT migrate Astral to Godot. Test exported maps, color spaces, seams and shader response. |
| Local AI image assistance | [ComfyUI](https://github.com/Comfy-Org/ComfyUI) with [FLUX.2 klein 4B](https://huggingface.co/black-forest-labs/FLUX.2-klein-4B); optional [FLUX.1 schnell](https://huggingface.co/black-forest-labs/FLUX.1-schnell) | GPL-3.0 app; named weights Apache-2.0. Use local inference, not paid API nodes. | Audit every node/model; do not generalize to other FLUX variants. Exact memory and quality need local tests. schnell's download may require account/access acceptance. |
| Optional image-to-3D research | [TRELLIS.2](https://github.com/microsoft/TRELLIS.2) | MIT model/code; dependencies have separate terms. | Official setup requires Linux and at least 24 GB NVIDIA memory. Not a guaranteed fit on a 16 GB GPU and not required to produce assets. Blender is the free fallback, not rented compute. |
| Optional anime-base authoring | [VRoid Studio](https://vroid.com/en/studio) | Free proprietary authoring application, subject to its terms and item rights. | Nonessential candidate. Direct commercial-use help/terms pages could not be retrieved in this audit, so release-use clearance is pending. Do not treat marketplace clothing as automatically free. |
| Character voices, English/Japanese | [Qwen3-TTS](https://github.com/QwenLM/Qwen3-TTS), [VoiceDesign](https://huggingface.co/Qwen/Qwen3-TTS-12Hz-1.7B-VoiceDesign), [Base](https://huggingface.co/Qwen/Qwen3-TTS-12Hz-1.7B-Base), [Tokenizer](https://huggingface.co/Qwen/Qwen3-TTS-Tokenizer-12Hz) | Apache-2.0 for the named code/models. Local generation, no required hosted service. | Design original voices, approve and freeze references, reuse them per character. Consent/reference rights and native-language listening review are separate. Generate assets offline. |
| Speech mouth timing | [Rhubarb Lip Sync](https://github.com/DanielSWolf/rhubarb-lip-sync), optional preview; Blender-authored face rigs | MIT for Rhubarb. | English recognizer and less-precise language-independent phonetic mode. Not a Japanese-quality guarantee or full face-acting system. Re-time each final language take. |
| Music composition | [LMMS](https://github.com/LMMS/lmms) | GPL-2.0 application. | Compose original music with cleared instruments/samples. Plugin support is not a license for every external plugin or sample pack. |
| Sound editing and ambience | [Audacity](https://www.audacityteam.org/faq/), original recording/procedural synthesis | Free GPL application. | Use cleared recordings. Preserve the original advance cue from PR #71; do not rip Genshin/Pokemon sound. |
| Localization authoring | Existing UTF-8 JSON/Python workflow; [Poedit open-source build](https://github.com/vslavik/poedit/blob/master/COPYING), optional | MIT source for Poedit; exclude Pro/cloud translation features. | Japanese/English localization needs native review, context and pronunciation dictionaries, not just machine translation. |
| Optional local coding/writing assistance | [llama.cpp](https://github.com/ggml-org/llama.cpp) with [Qwen3-8B](https://huggingface.co/Qwen/Qwen3-8B) as an example | MIT runtime; named weights Apache-2.0. | No claim this matches a premium coding agent or is the best model. Optional offline assistance, not live dialogue-generation dependency. |
| Reference/starter assets | [Poly Haven](https://polyhaven.com/license), [Kenney asset packs](https://kenney.nl/support) | CC0 for the stated asset libraries. | Keep source IDs and style/quality checks. Site APIs, non-asset products and third-party uploads may have different terms. Originals remain necessary for signature characters/world art. |
| Trailers and QA capture | [OBS Studio](https://github.com/obsproject/obs-studio), [Kdenlive](https://github.com/KDE/kdenlive) | GPL-2.0; GPL-3.0 applications. | Capture our own builds, retain codec/plugin notices, and clear music/footage rights. |
| Hosted collaboration/CI | Existing public repo, [standard GitHub Actions runners](https://docs.github.com/en/billing/concepts/product-billing/github-actions) | Public standard-runner usage has a free route; other services and quotas differ. | Larger runners, private overages and storage may cost money. Billing/quotas were not inspected or changed. Keep paid usage disabled or use an approved local fallback. |

## What remains custom engineering

Selecting free libraries does not supply the following finished systems. These
must be implemented and accepted as Astral features rather than claimed from
upstream feature lists:

- Resource lifetime, scene hierarchy/prefabs, serialization and hot reload;
  importer/cooker/cache with safe malformed-input handling and versioned assets.
- GPU scene rendering, toon/PBR materials, shader variants, shadows, reflections,
  anti-aliasing, post-processing, transparency, particles and debug views.
- Terrain, foliage, weather, water, asynchronous streaming, LOD/culling and large
  world coordinates. Advanced virtualized geometry and global illumination remain
  later engineering/research, not features conferred by bgfx or meshoptimizer.
- Animation graphs, state transitions, root motion, IK/retargeting and blendshape
  expression/viseme/gaze layers. A skeleton sampler is only one component.
- Physics/animation/gameplay synchronization, combat, precise weak-point hits,
  defense timing, AI actions, quests, interactions, inventory, equipment and saves.
- Usable scene editing, gizmos, undo/redo, save/reopen, import controls, inspectors,
  asset management, console/debugger, templates, tutorial and packaged builds.
- Game dialogue pacing, safe skip/recap/history, multilingual voice-pack playback,
  responsive input, controller support, accessibility and narrative transactions.

Retain one playable protagonist. NPCs and AI companions remain distinct voiced
characters, not a switchable gacha roster. Optional gacha is equipment/cosmetics
only; do not activate payments, spending systems or remote accounts through a tool
selection. The game still needs original writing, balancing, art direction and
playtests. No model license proves professional creative quality.

## Changes to earlier recommendations

Earlier [creative research](../Planning/CREATIVE-PRODUCTION-2026-09-21.md) mentioned
hosted image generation, Meshy, Substance and Cascadeur as comparisons. They are
NOT required in this free-only baseline. Use local image models, Blender and
Material Maker instead. Treat any optional paid tool as out of scope unless Lucas
later changes the constraint; do not assume a trial/student license solves it.

ElevenLabs was an optional comparison in [voice PR #71](https://github.com/LucasKazaki/AnimeRPG/pull/71).
It is removed from the proposed production baseline. Its [publishing policy](https://help.elevenlabs.io/hc/en-us/articles/13313564601361-Can-I-publish-the-content-I-generate-on-the-platform)
distinguishes free and paid output permissions; a free allowance is not the same
as cleared commercial production. Use locally generated, reviewed Qwen voice takes.

Do not adopt MusicGen merely because AudioCraft source is MIT. The official
[AudioCraft license statement](https://github.com/facebookresearch/audiocraft)
and [MusicGen model card](https://huggingface.co/facebook/musicgen-small) give
CC-BY-NC-4.0 for the weights. Exclude it from the potentially commercial baseline;
use original LMMS composition. Likewise, the Apache license of FLUX klein 4B does
not apply to its differently licensed 9B sibling or every community fine-tune.

## Dependency and asset admission gate

Before a new tool/model is admitted, record exact version/commit, source URL and
hash, license text/SPDX identifier, fee conditions, intended role (authoring tool
versus distributed runtime), transitive components, output/reference rights,
required notices and hardware needs. Prefer permissively licensed runtime
libraries. Use GPL art tools as standalone authoring programs; do not copy their
code into the engine without a compatible redistribution plan. Font, voice,
texture, training-reference and plugin rights need their own records.

A candidate stays unapproved until the exact build and terms are reviewed. No
paid account, external API requirement, card-on-file dependency, watermark-removal
purchase, noncommercial-only production model or unknown asset license may slip
into the baseline. Unknown means pending, not free. Select only necessary
features; pin and review upgrades. Add a future build check for the real dependency
manifest once integration establishes it. This documentation is NOT a scanner of
all installed software, nor an enforceable billing lock.

Admit each subsystem through one owned task, one visible in-engine demonstration,
regression tests and native evidence. Coordinate with existing workers and the
sole authorized workstation executor. No second scheduler or automatic
installation is introduced by this report.
