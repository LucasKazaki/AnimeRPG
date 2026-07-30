# Decision Log

## 2026-07-21 — Production architecture reconciliation

Decision: Proceed with the PRD's custom C++ Astral Engine (option 1). The Unreal Specialist is the coordinator/reviewer, not the production runtime owner. See `Docs/Architecture/ADR-0001-engine-choice.md`.

Rationale: The PRD explicitly requires custom C++, excludes Unreal/Unity, and the current workspace contains no Unreal project. Switching to Unreal would be a deliberate product change requiring Lucas approval.

Consequence: M1 is implemented as a native Win32/GDI prototype. Runtime verification is blocked until a native C++ compiler and CMake are available; no license-accepting install was performed.

## 2026-07-21 — First implementation gate

Implemented M1 source and static verification gate. `python Scripts/verify_milestone1.py` passes. Native configure/build/runtime tests remain pending on the toolchain blocker. Next queued task is M2 only after M1 runtime evidence.

## 2026-07-21 — M2-1 debug scene foundation

M2-1 implemented the smallest coherent scene/rendering slice: a transform parent-child contract, orthographic camera mapping, disk-loaded text static mesh, and GDI debug grid/triangle rendering. It deliberately does not introduce DirectX/Vulkan, gameplay, physics, ECS, or production asset dependencies.

Evidence: Debug and Release native builds passed; CTest passed 2/2 in both configurations; static verification passed; live Win32 probe found a 1264x681 window, sampled the expected clear color `RGB(12,18,36)` and triangle color `RGB(168,92,255)`, and closed the process with exit code 0. QA details are in `Docs/QA/MILESTONE-2.md`.

Next queued task: M3 third-person controller, pending branch review and Lucas merge approval.

## 2026-07-28 — M2-1 merge decision resolved

Decision: Lucas explicitly approved continuing past the M2-1 merge gate. The already-recorded merge commit `43310cc` is accepted as the current `main` baseline for the next bounded task.

Evidence: `main` points to `43310cc` (`Merge branch 'task/m2-planning'`); Debug and Release builds pass; CTest passes 2/2 in both configurations; Milestone 1 and Milestone 2 static gates pass.

Consequence: M3 third-person controller is authorized to begin in a dedicated worktree. This approval does not authorize destructive cleanup, external downloads, plugin installation, public deployment, history rewriting, or automatic future merges. Each subsequent packet retains its own review, QA, and merge gate.

Next queued task: implement M3 from `Tasks/M3-third-person-controller.md` in `C:/AI/worktrees/AnimeRPG/m3-controller-implementation`, followed by independent review and QA evidence.

## 2026-07-28 — M3 bounded controller implementation packet

Implemented the smallest custom C++ controller slice in dedicated worktree `task/m3-third-person-controller`: `PlayerController` provides normalized WASD movement scaled by the existing `Clock::Tick()` delta and clamps XY position to an explicit `MovementBounds`; `OrthographicCamera::Follow()` applies a documented target-plus-offset contract; the Win32 loop samples WASD and preserves the existing debug mesh, grid, FPS title, and close-message path.

Evidence: Debug and Release builds passed; CTest passed 2/2 in both configurations; targeted static verification passed. Runtime capture showed the 1264x711 window, live FPS title, grid, and debug triangle. Desktop approval denied WASD injection, so live movement/camera/boundary/clean-close evidence remains outstanding. No merge was performed. See `Docs/QA/MILESTONE-3.md`.

Decision: hold merge for independent runtime QA and Lucas approval; do not reopen the already-resolved M2-1 gate.

## 2026-07-30 — M3 automated runtime substitute accepted

Decision: Lucas explicitly authorized autonomous M3 unblocking and accepted a genuine automated native runtime smoke as the substitute for manual runtime QA. This authorization includes committing the verified M3 branch, merging it into `main`, and creating the isolated M4 worktree without another per-merge prompt.

Evidence: an external CMake build tree produced passing Debug and Release builds and 2/2 CTest results in both configurations; the M3 static verifier passed. The automated Win32 smoke found the process-owned `Astral Engine | M3: WASD Move` window at `1264x681`, used `SendInput` through the real `GetAsyncKeyState` path, observed title position changes from `(0,0)` to bounded `(9,5)`, observed the rendered purple mesh remain effectively screen-stationary under camera follow, and exited through controlled Escape with code 0. See `Docs/QA/MILESTONE-3.md`.

Consequence: the M3 runtime blocker is resolved without claiming manual evidence. M3 may merge after the final scope, build, test, static, and diff gates pass. M4 combat sandbox is the next approved milestone and must begin in a fresh isolated worktree.

## 2026-07-30 — M4 bounded combat sandbox verified

Decision: accept the packet-bounded deterministic combat domain and native runtime evidence for M4. `CombatSandbox` owns one fixed training dummy, range/damage/cooldown rules, and terminal defeat state independently of Win32. The existing application loop maps edge-triggered `J`/`K` input to that domain, while the existing GDI renderer and title expose dummy and attack state.

Evidence: out-of-source Debug and Release builds passed; CTest passed 4/4 in both configurations. Focused domain tests cover light/heavy damage, out-of-range and cooldown rejection, defeat, and post-defeat immunity. The automated native runtime smoke launched each configuration of `AstralGame`, observed title transitions for light hit, cooldown rejection, heavy hit, defeat, and post-defeat rejection through real `SendInput`/`GetAsyncKeyState`, then exited through Escape with code 0. The M3 verifier, scope audit, and `git diff --check` also passed. See `Docs/QA/MILESTONE-4.md`.

Consequence: M4 satisfies its autonomous merge gate. No evidence requires reprioritization away from the staged P0 sequence; M5 National Mall blockout remains the highest-value next bounded task and should begin in a fresh isolated worktree.

## 2026-07-30 — M5 perspective wireframe world verified

Decision: accept M5's bounded C++17/GDI perspective world capability. `PerspectiveCamera` supplies deterministic positive-near-plane projection and player follow; `WorldBlockout` supplies a traversable X/Z grid and deterministic Lincoln Memorial, Reflecting Pool, and Washington Monument wire proxies at distinct depths. The live renderer uses those objects rather than translating the old debug triangle and preserves the player marker, dummy, J/K combat, title state, and clean exit.

Evidence: out-of-source Debug and Release builds passed and CTest passed 6/6 in both configurations. Release-active focused tests cover projection center, depth scaling, vertical orientation, near rejection, camera follow, deterministic landmark data, valid dimensions, distinct depth, and ground mapping. The native M5 smoke launched the actual game, observed M5/title position state, verified a real light hit, moved through the real W input path, captured exact GDI colors for the grid, all three landmarks, player, and dummy, observed the rendered frame hash change, and exited through Escape with code 0. See `Docs/QA/MILESTONE-5.md`.

Consequence: M5 satisfies its autonomous merge gate. Dynamic selection from the updated backlog favors M7 Shadowblade (P0) over M6 destruction (P1); create a clean isolated M7 worktree after the M5 merge, but do not implement it without a bounded task packet.

## 2026-07-30 — M7 Shadowblade action kit verified

Decision: accept M7's bounded C++17 Shadowblade gameplay layer. `ShadowbladeActions` owns deterministic resource, cooldown, guard, dash, and fatal-strike outcomes; the live application routes dash through the bounded controller and fatal damage through the existing combat domain while preserving the M5 perspective world.

Evidence: canonical external Debug and Release builds passed and CTest passed 8/8 in both configurations. Focused tests cover resource cap/cost/rejection, positive-finite delta handling, dash and fatal cooldowns, fatal range/damage/defeat, and guard conflicts. The native M7 smoke launched the actual game, observed held guard and rendered guard pixels, verified guard-blocked Q, applied an 80-damage L fatal strike, moved from Z 0 to Z 6 with Q, observed repeated-Q cooldown rejection, captured the resource bar, and exited through Escape with code 0. Existing M4/M5 runtime smokes remained green. See `Docs/QA/MILESTONE-7.md`.

Consequence: M7 satisfies its delegated merge gate. Dynamic selection favors the next P0 player capability, M8 Thought Commands, over P1 M6 destruction; create a fresh bounded M8 packet and isolated worktree after integration.
