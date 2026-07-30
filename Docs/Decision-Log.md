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

## 2026-07-28 — M3 bounded controller implementation packet

Implemented the smallest custom C++ controller slice in dedicated worktree `task/m3-third-person-controller`: `PlayerController` provides normalized WASD movement scaled by the existing `Clock::Tick()` delta and clamps XY position to an explicit `MovementBounds`; `OrthographicCamera::Follow()` applies a documented target-plus-offset contract; the Win32 loop samples WASD and preserves the existing debug mesh, grid, FPS title, and close-message path.

Evidence: Debug and Release builds passed; CTest passed 2/2 in both configurations; targeted static verification passed. Runtime capture showed the 1264x711 window, live FPS title, grid, and debug triangle. Desktop approval denied WASD injection, so live movement/camera/boundary/clean-close evidence remains outstanding. No merge was performed. See `Docs/QA/MILESTONE-3.md`.

Decision: hold merge for independent runtime QA and Lucas approval; do not reopen the already-resolved M2-1 gate.

## 2026-07-30 — M3 automated runtime substitute accepted

Decision: Lucas explicitly authorized autonomous M3 unblocking and accepted a genuine automated native runtime smoke as the substitute for manual runtime QA. This authorization includes committing the verified M3 branch, merging it into `main`, and creating the isolated M4 worktree without another per-merge prompt.

Evidence: an external CMake build tree produced passing Debug and Release builds and 2/2 CTest results in both configurations; the M3 static verifier passed. The automated Win32 smoke found the process-owned `Astral Engine | M3: WASD Move` window at `1264x681`, used `SendInput` through the real `GetAsyncKeyState` path, observed title position changes from `(0,0)` to bounded `(9,5)`, observed the rendered purple mesh remain effectively screen-stationary under camera follow, and exited through controlled Escape with code 0. See `Docs/QA/MILESTONE-3.md`.

Consequence: the M3 runtime blocker is resolved without claiming manual evidence. M3 may merge after the final scope, build, test, static, and diff gates pass. M4 combat sandbox is the next approved milestone and must begin in a fresh isolated worktree.
