# Project Guide — AnimeRPG / Astral Engine

## Current objective

Build and stress-test Astral Engine: a native C++17, CMake, Win32 low-level 3D engine that also supports genuine 2D games. The eventual 3D anime action RPG is future product work; gameplay, story, levels, art, and audio content are paused. Existing 2D art is historical placeholder material, not accepted 3D output.

## Mandatory read path

1. `AGENTS.md` — execution, worktree, verification, and review requirements.
2. `GAME_DEVELOPMENT_CONTROL.md` — current operator direction; this wins over older materials.
3. `GOAL_WORK.json` — current criterion and bounded work state.
4. The active task packet in `Tasks/` — allowed files and exact checks.
5. Only then open the smallest relevant implementation contract or source file.

## Project map

| Location | Use |
| --- | --- |
| `Engine/` | Native engine implementation |
| `Game/` | Game-facing layer and historical placeholder assets; do not expand paused content |
| `Tests/` | Native tests and fixtures |
| `Tasks/` | Bounded work packets; `Tasks/README.md` explains the packet workflow |
| `Docs/Architecture/`, `Docs/Planning/` | Architecture decisions and milestones |
| `Docs/Agents/DEVELOPMENT_LOOP.md` | Detailed implementation/review loop |
| `Docs/QA/`, `Docs/Reviews/` | Independent QA and review evidence |
| `Docs/Decision-Log.md` | Durable design decisions |
| `Scripts/` | `doctor.ps1`, `reproduce.ps1`, `verify.ps1`, `smoke-test.ps1`, and `safe-reset.ps1` |
| `Artifacts/` | Generated evidence; inspect only when the task needs a receipt |

## Editing and verification

Use one dedicated worktree per implementation task; edit only the active packet's allowed files. Record changed files, exact commands, outputs, risks, and follow-ups. Completion requires the packet end state, Debug and Release verification, applicable runtime smoke, direct inspection, and an independent QA/review artifact before merge approval. Never infer an engine migration or broaden scope from legacy milestone documents.

## Token-saving rules

Do not recursively scan `Docs/Agents/` or `Artifacts/`; both contain historical heartbeat/status material. Use the authority chain above, then search only the relevant subtree. Treat dated handoffs, prior `GOAL_WORK` copies, and old milestone packets as historical unless the current sources explicitly point to them.
