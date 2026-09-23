# GAME pass 20: meaningful dialogue flow

Date: 2026-09-23
Owner: `animerpg-game-hourly`
Target: `main`
Baseline: `a356b4ac9ae30e755962a782d2fc4d74e3b5fc5e`
Branch: `game/2026-09-23-dialogue-flow-pass20`

## Scope and ownership

This is a bounded GAME-domain pass. Preserve Astral Engine and the existing National Mall supernatural story. Do not touch renderer/platform/editor/import/animation/audio/physics, shared CMake/workflows/dependencies, R0, release/deployment, or the open engine/art PR stacks.

Allowed paths:
- `Engine/Scene/LandmarkDialogue.h`
- `Engine/Scene/LandmarkInteraction.h`
- `Tests/LandmarkDialogueFlowPass20Tests.inc`
- `Tests/ThoughtCommandsTests.cpp`, include/call registration only
- this task packet
- `Docs/Agents/animerpg-hourly/RUN-2026-09-23-PASS20.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

## Current repository gap

Pass 9 already made authored dialogue choices produce distinct response IDs, bounded trust, clues/lore, committed outcomes, and an eight-beat history. Pass 14 feeds authoritative landmark/objective state into that dialogue. The live domain still lacks explicit story-topic prerequisites, spoiler-safe early preview, recommended unresolved topic guidance, a compact synopsis for skipped/returning players, quick-advance safety metadata, and a policy that suppresses a choice prompt when every non-exit option collapses to the same response.

## Research mapping, accessed 2026-09-23

Primary current comparator: Zenless Zone Zero Version 3.2 update announcement, published 2026-09-09:
https://zenless.hoyoverse.com/en-us/news/166000?catchSpider=1&lang=en-us&page=news

- `GAME-097` Story-topic prerequisites. ZZZ 3.2 Main Story, Trust Events, Quality Time, and areas document explicit story/progression requirements. Adaptation: Shadow Crypt and Mana Reactor topics become available from original National Mall evidence/objective prerequisites; basic topics remain open.
- `GAME-098` Advance Screening. ZZZ 3.2 explicitly allows Season 3 content to be experienced early through Advance Screening after a bounded prerequisite. Adaptation: an opt-in preview may expose an otherwise locked topic but cannot mutate trust, discussed state, clues, lore, or committed outcome.
- `GAME-099` Recommended unresolved topic. ZZZ 3.2 adds a Highlights indicator for recommended content. Adaptation: deterministic recommendation chooses an available, not-yet-discussed authored topic and only falls back to an explicit preview when screening is enabled.
- `GAME-100` Conversation synopsis. ZZZ 3.2 adds plot synopses for certain skipped cutscenes. Adaptation: expose a compact deterministic dialogue synopsis from authoritative state: discussed-topic/clue/lore counts, trust, outcome, latest response, and retained preview count.
- `GAME-101` Safe quick advance. ZZZ 3.2 optimizes tapping the dialogue box to quickly advance certain plot sequences. Adaptation: mark only non-mutating/redundant beats as safe to fast-forward so a future UI does not skip trust/clue/lore-changing decisions.

Community increment:
- `QOL-021` Suppress fake/single-choice prompts. A ZZZ_Official player post dated 2026-05-06 asks to stop single or fake dialogue choices because they interrupt scene flow; a 2026-01-12 post independently criticizes mandatory single-option clicks during auto dialogue, and a 2026-09-08 post again criticizes functionally identical choices. Counterarguments in community discussions note that prompts can keep players engaged or provide a pause. Adaptation: retain meaningful prompts, but recommend auto-advance only when all currently valid non-exit choices resolve to the same response. A newly available evidence branch restores the prompt. These are player anecdotes/preferences, not a claim of consensus or proof that ZZZ 3.2 still has the same behavior.
  - https://www.reddit.com/r/ZZZ_Official/comments/1t5g5ir/can_we_please_stop_doing_single_or_fake_dialogue/
  - https://www.reddit.com/r/ZZZ_Official/comments/1qb8794/why_give_the_auto_option_then_keep_giving_single/
  - https://www.reddit.com/r/ZenlessZoneZero/comments/1wauszm/ok_i_know_dialogue_choices_are_useless_9_times/

No comparator characters, story, maps, dialogue text, art, audio, code, monetization, party architecture, online requirement, or PvP is imported.

## Acceptance criteria

1. `GAME-097`: locked story topics fail closed without history/trust/clue/lore mutation; authoritative landmark/objective/evidence state unlocks them.
2. `GAME-098`: opt-in preview returns a valid preview beat for a locked topic but does not mark it discussed or change trust/clues/lore/outcome; ordinary access remains locked until prerequisites are met.
3. `GAME-099`: recommendation never returns an already-discussed topic; normal available content beats preview content; no recommendation is returned when everything admissible is exhausted.
4. `GAME-100`: synopsis exactly reflects bounded authoritative dialogue state and the newest retained response; preview count is derived from retained history and cannot overflow.
5. `GAME-101`: quick advance is safe for redundant/non-mutating beats only, and false for a beat that changes trust, unlocks a clue/lore entry, or otherwise represents a substantive first-time choice.
6. `QOL-021`: first-time differentiated choices require a prompt; a repeated exhausted topic with all choices collapsing to `AlreadyDiscussed` does not; a newly eligible evidence response makes the prompt meaningful again.
7. Existing pass-9 dialogue behavior and LandmarkInteraction integration remain regression-clean. Invalid topic/choice values fail closed.

## Verification gates

Use the existing `ThoughtCommandsTests` registration shim for the pass-20 regression include. Do not edit CMake. Required before merge:
- hosted Windows Debug/Release deterministic workflow on the exact final source head,
- hosted Release-manifest integrity workflow on that exact head,
- fresh independent implementation review on the exact head with no unresolved material finding,
- full diff/scope review and re-read of live `main` and PR head immediately before merge.

Native rendered dialogue UI, controller/menu wiring, voice/audio timing, GPU/performance evidence, cross-process story persistence, release, and deployment are outside this packet and must not be claimed.
