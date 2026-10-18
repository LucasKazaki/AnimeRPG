# Character voices and faster, expressive dialogue

Operator direction: September 24, 2026. Read with
[engine contract](../Engine/DIALOGUE-AUDIO-EXPRESSION-CONTRACT.md),
[voice-tool research](../Research/CHARACTER-VOICE-TOOLS-2026-09-24.md),
[authoring fixture](../../Content/Narrative/voice-dialogue-contract.v1.json), and
[VD-001 task](../../Tasks/VOICE-DIALOGUE-2026-09-24.md).
Status: instructions and tested source fixtures, not runtime implementation.

## One playable character, a full supporting cast
The player controls only the persistent protagonist. Preserve existing avatar,
class and loadout customization; these are variants/builds of that same identity,
not additional playable characters. Do not add character banners, character
pulls, character-unlock tokens, party switching, or temporary playable-NPC story
segments. Named NPCs, friends, rivals, enemies and AI companions may have their
own stories, personalities, dialogue and combat behavior without being playable.
References to team roles or allies in the localized-damage proposal are AI/NPC
roles or protagonist loadout synergies, never permission to switch characters.

An OPTIONAL gacha system may cover armor, tools, weapons and cosmetics only.
This is permission to design that feature, not a request to enable it now. The
source fixture keeps it disabled and does not authorize real-money purchases.
Proposed design guardrails: publish exact pool/odds, explain pity and duplicates,
retain acquisition history, offer duplicate protection/conversion, and preserve
viable deterministic progression. Core story access, voices, languages, dialogue
speed and accessibility settings must never be random rewards or purchases.
Monetization, pricing, timers, paid currencies and jurisdiction-specific release
review require a later explicit decision. Do not invent approved rates or prices.

## AI voice production is a requirement, not a runtime dependency
Plan English (`en-US`) and Japanese (`ja-JP`) voice packs from the first voiced
vertical slice. Voice language and text/UI language are independently selectable.
Every speaking character needs an individual voice identity and personality
brief in each supported locale. The protagonist is included. Do not reuse one
unmodified narrator voice for the entire NPC population. Incidental speakers can
share a production workflow, but persistent character ids need distinct casting.

Maintain a character bible with stable id, role, motivations, values, fear/flaw,
relationships, knowledge limits, humor, vocabulary, sentence rhythm, emotional
range and what the character would NOT say. Maintain a linked voice bible with
register, resonance, texture, articulation, cadence, neutral baseline, expressive
range, pronunciation dictionary and language-specific performance direction.
Character personality is stable; momentary anger or anxiety is not a new voice.
Do not direct a model to impersonate a particular Genshin actor or character.

Audition original designed voices first. Use recordings only with documented
permission covering synthesis and the intended uses. Lock an approved reference
and model revision per character/locale; do not redesign the voice independently
for every line. Cross-language timbre may match when convincing, but a separately
approved Japanese casting is preferable to an unnatural accent. Preserve the
same character's intent and temperament rather than forcing identical acoustics.
Generated speech must receive human listening review and native-language review.
Distinct casting ids alone do NOT prove distinct or consistent sounding voices.

Generate, review and package story dialogue OFFLINE during content production.
The shipping game plays cached audio and must not wait for a TTS server or occupy
a gameplay GPU with a voice model. This also avoids line-to-line voice drift,
runtime generation costs and offline-play failures. No provider is installed or
called by VD-001. Model/provider candidates are conditional on the audition gate.

Each approved take needs line id/revision, localized text hash, character/voice
revision, locale, model/version/settings, reference hash or provider voice id,
source/consent record, generation date, output hash, duration, pronunciation and
performance approvals. Model license alone does not establish voice/recording
rights. New text, voice, translation or delivery invalidates dependent lip-sync
and audio-review records. Never commit private credentials or private consent
recordings into the public repository. Store opaque evidence ids where needed.

## Pacing: remove friction without flattening the story
Default text appears immediately; optional typewriter mode has a speed setting.
Manual advance never waits for voice completion, a gesture, a camera move, or an
arbitrary minimum reading timer. With typewriter enabled, the first press reveals
the rest of the line; the next press advances. Offer a direct-advance setting.
Voices stay at natural speed by default. Faster text is not a sped-up performance.

Skip goes to the next meaningful decision or the scene's authored endpoint,
with a concise recap and objective update. Do not simulate unseen dialogue choices
by repeatedly invoking Choose. Preserve quest/trust/reward outcomes exactly once.
Remove choices that only say 'okay' or force a click between halves of a sentence.
Optional lore belongs behind a clearly marked question, not in every critical
conversation. A read-only history panel can replay an already heard line without
reapplying its effects. History pauses auto-advance and restores it deliberately.

Proposed EDITORIAL targets, not measured improvements or hard truncation rules:
ordinary exchanges roughly 30-60 seconds; most lines one thought in one or two
sentences; typical English lines around 12-24 words. Evaluate Japanese by natural
phrasing and listening time, not English word counts. Major scenes may be longer
when they earn the time, while remaining skippable. Keep a 'what changed and what
do I do next?' summary. Preserve necessary story meaning and emotional pauses;
cut redundant explanation, repeated agreement and unnecessary camera delays.

Auto mode waits for both the selected voice take and a user-adjustable localized
reading interval, plus an adjustable gap. Missing audio falls back to readable
text timing. Never use auto-mode timing to lock manual advance. Test captions,
font size, speaker labels, backgrounds and independent volume controls against
accessibility guidance referenced in the research document.

## Story-directed faces, not only moving mouths
Author an expression and intensity for each significant story beat. Baseline
palette: neutral, warm, concerned, skeptical, determined, amused, sad, angry,
afraid and surprised. Use eyes, brows, eyelids, gaze, subtle head motion and body
posture, not just a mouth swap. Listening characters react as well as speakers.
Sarcasm can have a calm voice and skeptical expression; narration or an automatic
sentiment label must not override deliberate performance direction.

Separate the authored emotional layer from speech mouth shapes (visemes), blinking
and gaze. Build language-specific mouth timing from each FINAL audio take; English
mouth timings cannot be copied onto a Japanese take. Re-time expression beats by
semantic phrase anchors when translations change length. A 2D portrait/mouth-swap
fallback and a later 3D blendshape adapter can use the same semantic expression
ids. VD-001 does not deliver a face rig, lip-sync solver or rendered expressions.

## Sound direction
Use event `ui.dialogue.advance` for one committed line transition, not for every
letter, button-down repeat, audio callback or skipped intermediate line. Starter
source is [the original cue](../../Content/Starter/Audio/UI/README.md). Both the
engine tutorial/default conversation and the game should bind that event to the
same asset id, with a game-level override and separate UI volume/mute controls.
The desired interaction is a short, gentle, crisp confirmation with a retro-RPG
feel. The proposed cue is original procedural audio, NOT Genshin/Pokemon audio.
Exact matching is unverified because no reference recording was supplied. An
exact third-party asset needs a separate documented permission/provenance path;
do not rip, decompile or scrape game audio for this feature.

## Sample and first playable acceptance
The JSON has THREE original audition briefs and THREE short bilingual sample
lines. Only `protagonist` is playable; `rift_researcher` and `field_quartermaster`
are proposed role fixtures, not finalized lore or a complete cast. Their six
locale casting slots are unassigned, all takes are not generated, and Japanese
text is a draft requiring native review. Do not rename existing characters to
these placeholder roles without a game-authoring decision.

First vertical slice: one real scene with protagonist plus two NPCs, approved
English/Japanese takes, expressions, correct lip sync, instant/manual/auto/skip
controls, original advance sound, and safe choice/history integration. Measure
interaction delay and scene duration on a real build. Do not call it shipped
because this source fixture or a default CI job passes.
