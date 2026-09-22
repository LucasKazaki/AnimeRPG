# GAME Pass 9: landmark dialogue, lore, clues, and readable history

Date: 2026-09-22  
Owner: `animerpg-game-hourly`  
Base: `main` at `96646c67b034982faf7051f1fb869806a90fe9bd`  
Working branch: `game/2026-09-22-dialogue-lore-pass9`

## Scope

This packet deepens the original National Mall narrative loop around the persistent protagonist. It adds deterministic game-domain dialogue state that reacts to exploration/objective progress, remembers discussed topics, unlocks original project lore and investigation clues, resolves one bounded side-objective outcome, and retains a short recent dialogue history. It does not add a dialogue renderer, voice/audio, networking, gacha/party switching, external model calls, engine infrastructure, editor/renderer changes, dependencies, save-file formats, deployment, or release.

Allowed production paths:
- `Engine/Scene/LandmarkDialogue.h`
- `Engine/Scene/LandmarkInteraction.h`

Allowed verification/records:
- this task
- `Docs/Agents/animerpg-hourly/RUN-2026-09-22-PASS9.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

Shared CMake, renderer, platform, editor, import, audio, physics, and workflow files are intentionally unchanged. Open engine PR #13 and art PR #23 were inspected and left untouched.

## Research mapping

Sources were read on 2026-09-22. Comparator mechanics are design references only. All response IDs, lore concepts, clue names, side-objective logic, and state rules below are original to AnimeRPG's established supernatural Washington DC premise.

1. **GAME-042, choice-specific landmark dialogue responses.** Wuthering Waves describes a story-rich open world where player choice matters, while the Granblue Relink developers discuss interactive dialogue as part of character portrayal. Adaptation: five bounded dialogue topics support distinct response IDs for direct, reassuring, challenging, and evidence-sharing choices instead of generic repeat text.
   - https://apps.apple.com/us/app/wuthering-waves/id6475033368
   - https://blog.playstation.com/2024/01/30/granblue-fantasy-relink-devs-discuss-crafting-an-immersive-rpg-world-for-ps5-ps4-out-feb-1/

2. **GAME-043, trust and remembered topics.** Zenless Zone Zero's developer description emphasizes talking with New Eridu residents, learning their experiences, and becoming friends/allies. Adaptation: dialogue choices change a tightly bounded local trust score, and first-time topic state prevents repeat interactions from farming trust or narrative unlocks.
   - https://apps.apple.com/us/app/zenless-zone-zero/id1606356401

3. **GAME-044, unlockable lore journal entries.** Granblue Relink explicitly points players to Lyria's journal for people, places, and history, alongside side quests and Fate Episodes. Adaptation: discussing relevant topics unlocks three original knowledge entries: National Mall resonance, the quantum-cooling anomaly, and a Shadow Crypt rumor.
   - https://www.playstation.com/en-us/games/granblue-fantasy-relink/

4. **GAME-045, evidence-sensitive investigation clues.** Genshin's official PlayStation description centers free exploration, strange mechanisms, discoveries, and mysteries; Wuthering Waves likewise emphasizes hidden truths. Adaptation: evidence-sharing can unlock Rift Residue after sufficient landmark exploration, Cooling Anomaly after objective completion, and a Crypt Sigil after trust is earned. Dialogue receives exploration state from the existing `LandmarkInteraction` production path instead of duplicating world state.
   - https://www.playstation.com/en-us/games/genshin-impact/
   - https://apps.apple.com/us/app/wuthering-waves/id6475033368

5. **GAME-046, one-time side-objective outcome.** Granblue Relink highlights side quests and character backstory episodes, while Wuthering Waves presents player choice as part of its story framing. Adaptation: the conversation can resolve once to `AidCivilians` when sufficient trust plus the safety topic are present, otherwise to `PursueRift` when at least two investigation clues are established. Repeated commits are idempotent.
   - https://www.playstation.com/en-us/games/granblue-fantasy-relink/
   - https://apps.apple.com/us/app/wuthering-waves/id6475033368

6. **QOL-010, bounded recent dialogue history.** Current player discussions report timed/moving-platform dialogue advancing too quickly to read and ask for a practical way to review what was just said. A 2026-08-22 Genshin thread says some Snezhnaya dialogue advances with no way to go back; a 2026-08-17 report specifically says the dialogue history could not be used to pause/recover the conversation. Other commenters argue some forced timing is intentional on moving platforms, so this is treated as player feedback, not consensus or proof of a universal current defect. Adaptation: retain the last eight valid dialogue beats locally, newest-first, with deterministic eviction and no effect on quest rewards or trust.
   - https://www.reddit.com/r/Genshin_Impact/comments/1vuze9j/genshin_isnt_even_letting_us_read_anymore/ (published 2026-08-22)
   - https://www.reddit.com/r/GenshinImpact/comments/1vqf3cu/auto_mode_was_turned_off_but_i_still_couldnt/ (published 2026-08-17)
   - Current resolution: unverified. Search results also show 2026-08-24 reports, but no official universal history/rewind fix was established in this pass.

## Acceptance

- Distinct first-time choices for the same topic yield distinct authored response IDs where specified.
- Invalid topic/choice values fail closed and do not enter history or mutate trust/lore/clues.
- Trust remains between -3 and +3 and records the actual applied delta after clamping. Repeated ordinary choices return `AlreadyDiscussed` without changing trust or duplicating lore.
- A topic discussed before its evidence prerequisite becomes true does not permanently lose that clue: a later `ShareEvidence` may unlock the newly eligible clue and contextual response, but it still cannot reapply trust or lore rewards.
- Lore unlocks are one-time and bounded to three original entries.
- Evidence clues require their exploration/objective/trust prerequisites and are one-time.
- Dialogue context comes from the actual `LandmarkInteraction::VisitedCount()` and `ObjectiveComplete()` state.
- `CommitDialogueOutcome()` is idempotent and can produce both intended branches under their documented prerequisites.
- Recent dialogue history retains at most eight valid beats, newest-first, with deterministic oldest-entry eviction.
- Pure dialogue behavior is covered by a C++17 compile-time regression contract in `LandmarkDialogue.h`; warning-clean GCC and Clang ASan/UBSan exact-source runtime fixtures pass before push. Hosted Windows Debug/Release builds must compile the same contract on the final PR head.
- Independent review on the exact final head must have no unresolved major finding before merge.

## Explicit limits

This packet is game-domain narrative state, not a rendered dialogue screen. No native keyboard/controller interaction, subtitles, voice acting, scrolling history panel, cross-process save persistence, or interactive playtest is claimed. The recent-history store is session-local and intentionally bounded. UI integration is a later game packet once the required shared engine/UI interface is available and owned.