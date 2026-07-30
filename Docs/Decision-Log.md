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
