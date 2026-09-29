# Game Development Control

## Current operator direction

Engine-first, local-only development is active. Build and stress-test the
custom native C++17 Astral Engine with 3D as the primary target and genuine 2D
support. Preserve the existing native implementation and the registered
worktree, task, evidence, approval, and merge controls.

Game content, combat tuning, encounters, narrative, production art/assets,
audio content, and game playtests remain paused until engine acceptance.
Procedural 3D and 2D fixtures are allowed only for engine verification.

## Required delivery evidence

Each admitted engine task needs exact worktree ownership, allowed paths,
external Debug and Release build roots, a bounded stop condition, and retained
command evidence. Acceptance requires real 3D and 2D runtime checks, measured
frame-time/RAM/VRAM budgets, load/unload and failure-recovery stress evidence,
a 24-hour soak, and independent QA before game-content work resumes.

## Immediate control objective

Reconcile any claimed completed R0 release-candidate evidence with the present
engine scope, then admit exactly one dependency-ordered engine readiness, repair,
or verification task. A historical completion statement is not a verified receipt.
Do not replay historical game-content tasks merely because they are present in
the repository. See `Docs/Project-Status.md` for the September 19 repository audit;
it does not establish the live state of a local Windows host.

## September 22, 2026 operator update: parallel hourly GAME worker

Lucas explicitly requested a separate hourly AnimeRPG game-development worker
and then instructed that cycle to push and merge to `main`. This is a new
operator instruction, not a task packet unpausing itself. For that worker only,
it supersedes the blanket game-content pause above and the older per-merge human
permission requirement. Dependency-ready game research and implementation may
advance in parallel with engine work; do not wait for whole-engine parity before
admitting an independent game slice.

The [hourly game contract](Docs/Agents/ANIMERPG-HOURLY.md) controls this scope:
five distinct reference-game feature increments plus one additional sourced
community improvement are the per-pass implementation target. Report actual
researched/implemented/tested/playable/reviewed/merged counts and carry unmet
work honestly. Existing technical, native, independent implementation-review,
worktree, ownership, architecture and repository-protection gates remain.

The engine worker keeps its engine-first scope and all engine acceptance
requirements above, including the native/performance/stress/soak evidence.
No engine acceptance is inferred from game work. A dependency-specific failed
or missing gate blocks dependent changes, not unrelated independently verifiable
game work. The game worker must coordinate shared engine paths and must not
merge another worker's unreviewed PRs, change architecture, invoke unsafe R0,
start local execution, deploy, or release under this authorization.
Company Runtime remains the sole workstation scheduler/executor. These
operating records themselves are documentation, not playable-game acceptance.

## September 24, 2026 operator update: character voice and presentation scope

Lucas explicitly requests these product constraints and matching engine support:
- Only the persistent main character is playable. A voiced supporting cast and
  AI allies do not authorize party switching or playable NPC story segments.
- AI-generated character voices start in English and Japanese. Each character
  has a distinct personality and stable voice identity across their lines, with
  reviewed language-specific direction. Voice/text languages are independent.
- Optional gacha is limited to armor, tools, weapons and cosmetics, never playable
  characters. Real-money activation, rates and pricing are not decided here.
- Dialogue should be shorter and responsive: no forced voice/gesture waits for
  manual advance, safe skip with recap, meaningful choices and read-only history.
- Facial expressions follow story/performance direction and work alongside
  language-specific lip sync. Voice synthesis belongs to offline asset production.
- The desired Genshin/Pokemon-like advance feedback is represented by an original
  short cue in this packet. No exact reference recording or reuse permission was
  supplied; do not claim an exact match or copy a third-party game asset.

The [game instructions](Docs/Game/CHARACTERS-VOICES-DIALOGUE.md) and
[engine contract](Docs/Engine/DIALOGUE-AUDIO-EXPRESSION-CONTRACT.md) split ownership.
[VD-001](Tasks/VOICE-DIALOGUE-2026-09-24.md) admits instructions, research, strict
source-fixture validation and original audio generation without modifying live
combat/narrative state, runtime backends or another worker's files. This explicit
request permits this independent work despite older blanket content-pause text.
It does not accept the engine, waive native/QA gates, authorize dependencies or
purchases, modify local scheduling, or authorize this cross-lane packet to merge.
