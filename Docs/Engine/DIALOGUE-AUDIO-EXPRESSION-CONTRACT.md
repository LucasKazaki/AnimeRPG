# Astral dialogue, localized audio and expression contract

VD-001, September 24, 2026. This is an implementation contract, NOT implemented
runtime support. Preserve custom C++17 Astral Engine, genuine 2D and 3D targets.
Read [game direction](../Game/CHARACTERS-VOICES-DIALOGUE.md) and
[task/allowed paths](../../Tasks/VOICE-DIALOGUE-2026-09-24.md).

## Existing integration boundary
At inspected main `2958741188279a0b438dd489ad00cac546012c25`,
`Engine/Scene/LandmarkDialogue.h` already owns dialogue response ids, trust,
knowledge/history and choice outcomes. Its pass-9 task explicitly says it does
not deliver a rendered conversation screen or voice playback. VD-001 must layer
presentation over existing narrative ownership, not create a second quest state.
Inspect current `LandmarkDialogueContinuity` and `LandmarkInteraction` before
integration. Never invent a full working audio/face system from those data types.

Engine owns portable presentation state, localization lookup, audio lifecycle,
input routing, expression/viseme blending and rendering adapters. Game owns cast,
voice/performance direction, translated lines, choices, narrative transactions,
facial intent, gacha policy and the single playable protagonist. Do not hard-code
Genshin names, voice providers or game-specific player restrictions into a generic
engine. Provide the restrictions through the game policy layer.

## Planned line/performance asset boundary
A cooked line references stable scene/node/line/speaker ids, localized text,
locale-specific approved voice asset, character voice revision, expression beat
track and per-take viseme track. Keep audio hash, duration, decoded sample rate,
channel count and generation/rights evidence in the asset manifest. Do not place
provider credentials, prompts with private material, or raw cloning references in
runtime assets. UI locale, subtitle locale and voice locale are independent.

Treat a locale switch as a new playback generation. Stop/fade the previous take,
reset its viseme clock and start the selected take or show explicit text-only
fallback. Never silently play a random wrong-language take or mark missing speech
as reviewed. Scene/actor/line generation ids bind asynchronous callbacks; stale
completion after skip, scene teardown, replay or language switch must do nothing.

## Required presentation state machine
States: Inactive, Revealing (optional), AwaitingAdvance, AwaitingChoice, Closed.
Asset preparation must not create an indefinite input lock. Default instant text
enters AwaitingAdvance immediately; choices use AwaitingChoice. A finite failure
path displays text even when audio is missing. Manual input never waits on audio,
animation, inferred emotion, TTS, camera interpolation or a minimum read timer.

A typewriter reveal press only reveals text; it does not spend a second advance
or apply a choice. A subsequent advance requests one next-line transition.
Edge-trigger input, consume one action per input generation and do not let a held
confirm button auto-select the first choice that appears. Manual transition wins
over simultaneous auto-completion, so one frame cannot skip two nodes. Auto mode
uses both current-locale voice completion and reading timing; manual bypasses both.
Pause/history freeze auto progression and the active audio/viseme clocks together.

Skip is a semantic narrative operation: seek an authored safe checkpoint, stop at
unresolved meaningful choices, apply permitted checkpoint effects exactly once,
show a recap and update objectives. Do not advance through every hidden line,
emit a burst of UI cues, or use Choose to manufacture decisions. Read-only history
and replay must never alter trust, clues, outcomes, inventory or rewards. Existing
preview/advance-screening history remains non-committing. Save/load/re-entry needs
an explicit transaction identity at the narrative owner, not a UI-only seen flag.

## Audio event and starter content
Semantic event: `ui.dialogue.advance`. Default asset id:
`astral.ui.dialogue.advance.v1`. Same id for the engine tutorial and AnimeRPG.
Build the sample from `Tools/DialoguePresentation/generate_advance_cue.py`; verify
`Content/Starter/Audio/UI/expected-manifest.json`. This patch adds the generator
and pin, not a WAV decoder, mixer, content cooker or runtime event registration.

Production adapter requirements: preload this tiny cue; play once after a
successful user-visible line transition; do not cue initial reveal or failed
advance. Use a short configurable sound-only cooldown to avoid click spam without
dropping legitimate input or narrative transitions. UI bus volume and mute are
independent from dialogue/music; no voice model runs to synthesize a button cue.
Voice interrupt uses a short bounded fade (proposal 20-40 ms), but begins the new
line immediately rather than waiting for the fade. Do not let fades accumulate
voice instances or compete with accessibility narration. Limit/measure voices.
Do not normalize the whole mix solely from an isolated cue's amplitude.

The authored source asset is 48 kHz mono PCM16, 5,280 frames / 110 ms. Output hash
and exact bytes are pinned. These numbers describe the source asset, not measured
latency, loudness compliance, engine playback or user approval of its sound.

## Facial performance adapters
Keep semantic expression ids independent of a face rig. Map the ids to authored
2D portrait variants or 3D blendshape/bone controls. Use separate layers: base face,
story expression, visemes, blink, gaze, and additive head/body reaction. Define
masking/weights so lip sync cannot erase an intended smile or fully override eyes.
Clamp blend weights, interpolate transitions and restore a safe neutral state on
teardown. Stale line events must not animate a removed NPC or the next speaker.

Drive mouth timing from actual decoded audio playback, not elapsed wall time or
frame count. Resampling, pausing, skips and optional voice-speed changes must keep
the clock consistent. Rebuild visemes for each language and final take. Mouth
shapes are not an emotion model. Author emotion changes using semantic anchors;
auto-generated suggestions require review. Preserve a text/portrait fallback
until the 3D face/animation pipeline has native acceptance. Unreal/MetaHuman is a
research reference only, not a new runtime dependency or export-rights assumption.

## Dependency-ordered worker tasks
VD-E02: inspect current decoder/mixer, localization and UI primitives; record gaps
and agree interfaces with the game worker. Do not start another competing backend.
VD-E03: implement interruptible localized playback, generation-bound callbacks,
voice/text fallback and independently controlled buses; bind the original cue.
VD-E04: implement line presentation, instant/typewriter/manual/auto modes, safe
choice focus, history pause and atomic narrative-owner skip transactions.
VD-E05: implement semantic expression adapters and per-take viseme playback for
accepted 2D/3D assets; neutral reset, layering and language-switch behavior.
VD-G02: cast/audition each speaking character in both locales and record rights,
pronunciation and listening approvals. No massive batch before a pilot succeeds.
VD-G03: connect one existing narrative scene without changing its outcome logic;
add concise text, recaps and meaningful choices, then run native playtests.
VD-ART02: author approved expression/mouth variants and performance clips.

## Required regression/native matrix
Test missing/corrupt audio, missing locale, duplicate/stale completion, skipping
while loading, repeated confirm, auto/manual collision, pause/history, text-size
changes, controller focus, locale switch, replay and scene destruction. Verify no
repeated choices/rewards/trust/clues. Compare original narrative outcomes before
and after presentation integration. Test mismatched take/viseme hashes, absent
face targets, out-of-range curves and neutral restoration. Measure real input-to-
text/cue delay and audio/animation synchronization under load in Windows Debug and
Release; proposed responsiveness target is next visible frame without an imposed
wait, not a claim of zero latency. Confirm native 2D/3D rendering and actual sound.
Register native tests through astral_add_test in a coordinated follow-up. Source
Python tests here are not automatically discovered by the root CMake/CI suite.
