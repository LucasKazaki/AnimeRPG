# Unreal Specialist Handoff — Anime Action RPG PRD

## Source artifacts
- Original PRD: `C:\Users\taol\Downloads\Product Requirements Document.pdf`
- Extracted text (58 pages): `C:\Users\taol\Downloads\Product Requirements Document.extracted.txt`
- Workspace: `C:\AI\projects\AnimeRPG`
- Agent Studio: `C:\AI\projects\LucasAgentStudio`

## User mandate
Take ownership of turning this PRD into a real, playable game. Establish a durable, self-sustaining game-development loop. Spawn as many bounded specialist/reviewer/QA agents as are useful, and install the skills, CLIs, libraries, or applications needed, subject to the safety and approval boundaries below. Begin real work now; do not stop at a generic plan.

## Critical issue to resolve first
The PRD explicitly requires a **custom C++ engine (Astral Engine), not Unreal or Unity**, while this workspace and your registered role are currently Unreal Engine 5-focused. Do not silently reinterpret this contradiction. Treat the PRD as source-of-truth for product requirements, perform a short architecture/scope reconciliation, and record one of:
1. proceed with the custom C++ engine while using the Unreal Specialist as the game-development coordinator;
2. propose an Unreal-based implementation as a deliberate PRD change requiring Lucas's approval; or
3. use Unreal only for throwaway reference/prototyping while production remains custom C++.
If a decision from Lucas is required, create a concise blocking issue with options, recommendation, consequences, and the smallest safe work that can continue meanwhile.

## Immediate objectives
1. Read the complete extracted PRD and existing `AGENTS.md` / `CLAUDE.md`.
2. Inventory the workspace, toolchain, Unreal/custom-engine prerequisites, Git state, disk/hardware constraints, and existing agent/CLI availability. Distinguish installed/configured/authenticated/running/verified.
3. Convert the PRD into a staged milestone backlog and acceptance-test matrix, prioritizing the smallest executable vertical slice.
4. Establish a bounded autonomous loop: plan -> task packet -> isolated implementation -> build/test -> independent review -> QA/playtest evidence -> decision log -> next task. No two coding agents may edit the same working directory.
5. Create/initialize the appropriate project repository and isolated worktrees only if safe and consistent with existing files; do not delete or overwrite unrelated work.
6. Spawn specialist agents only for concrete bounded packets (engine/rendering, gameplay/combat, tools/build, world, AI, art pipeline, audio, QA, docs/review). Keep transcripts and artifacts in the workspace.
7. Install only what is genuinely needed and verify every installation. Prefer project-local/package-manager installs and reputable sources. Never use blanket bypass flags or disable security controls.
8. Start the first real milestone and produce a buildable/runnable artifact or a precise verified blocker—not just scaffolding or prose.

## Approval and safety boundaries
The user's request authorizes ordinary development dependencies, skills, CLIs, and free local applications needed for this project. It does **not** authorize purchases, paid API usage, accepting new licenses/terms on the user's behalf, credential entry, publishing, public deployment, opening firewall ports, weakening security, destructive cleanup, or changing unrelated machine/account settings. Ask Lucas before any of those. Preserve human approval for merges to protected branches and major architecture changes. Do not create an unbounded infinite process; use durable resumable iterations with stop conditions, budgets, logs, and an emergency-stop mechanism.

## Reporting contract
- Keep work local-first under `C:\AI`.
- Log exact agents spawned, skills/apps installed, commands run, files changed, tests/builds executed, failures, and next queued task.
- If blocked, return the exact error and a recommended next action.
- If not blocked, continue through at least one implemented and verified task before reporting.
