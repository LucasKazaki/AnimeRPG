# Voice and performance tool research

Sources checked September 24, 2026. Candidate selection, not an installation,
benchmark, purchase, license clearance or final quality approval. No character
speech was generated or auditioned during VD-001. Do not promise human-actor or
Genshin-quality parity from a provider's demonstrations.

## Candidate facts and project decisions
**Qwen3-TTS:** the official repository documents English/Japanese support,
VoiceDesign, reusable design-then-clone prompts, and different Base/CustomVoice
interfaces [1]. The named VoiceDesign model card lists Apache-2.0 [2]. Do not
assume the Base cloning interface supports every emotional-control option of
CustomVoice. Proposed first local pilot: design original character references,
freeze approved identities, then test consistency and emotion across both locales.
This is a recommendation to audition, not a claim it runs within a particular
GPU/VRAM budget. Pin exact model/package revisions when an executor is authorized.

**ElevenLabs:** its model documentation lists expressive v3 and multilingual v2,
including English and Japanese, plus voice-design models [3]. Its publishing
policy distinguishes free-plan output, eligible paid output and beta restrictions
[4]. Proposed optional paid comparison candidate after budget and service-specific
rights review. No credits spent and no accounts/voices connected in this task.
Do not claim a subscription to another AI product pays for this service.

**Rhubarb Lip Sync:** official documentation exposes timed mouth-shape output
and distinguishes an English recognizer from a less-precise language-independent
phonetic path [5]. Candidate for an inexpensive 2D preview only; not a guarantee of
Japanese phoneme accuracy or full facial acting. No binary imported or executed.

**MetaHuman audio-driven animation:** Epic documents offline facial solving,
blink/head controls, mouth-region masking and mood overrides [6]. This supports
our separation of lip motion from directed expression as a design reference.
It does not establish compatibility with Astral's rigs or permission to move
particular MetaHuman assets into another engine. No Unreal dependency is added.

**Accessibility:** Microsoft's XAG 104 covers subtitles/captions [7]. Use it when
reviewing speaker identification, presentation and configurability, not as a claim
that this unrendered fixture complies with the entire guideline.

## Audition gate, authored for this project
Use a small pilot: protagonist and two existing or approved NPCs, both languages,
neutral/joy/concern/anger/whisper/shout plus names and difficult pronunciation.
Freeze source lines before comparing candidates. Record exact model/reference,
script hash, seed/settings where supported, generation time, peak memory, failed
or repeated takes, clipping/silence and reviewer scores. Do not put timing targets
in the game based on provider marketing latency.

Evaluate identity consistency across lines, distinctness between characters,
Japanese accent/reading, English intelligibility, believable emotion and fatigue
from repeated barks. Have a proficient native reviewer check each locale. Emotion
and character identity must survive language switching. Retain all approved takes
as offline assets; do not regenerate during a conversation. Record constraints
and failures, not just the best sample. Expand the cast only after the pilot.

## Sources
[1] Qwen official repository and model/API examples:
https://github.com/QwenLM/Qwen3-TTS
[2] Official VoiceDesign model card:
https://huggingface.co/Qwen/Qwen3-TTS-12Hz-1.7B-VoiceDesign
[3] ElevenLabs model documentation:
https://elevenlabs.io/docs/overview/models
[4] ElevenLabs publishing/commercial-use policy:
https://help.elevenlabs.io/hc/en-us/articles/13313564601361-Can-I-publish-the-content-I-generate-on-the-platform
[5] Rhubarb official repository and recognizer limitations:
https://github.com/DanielSWolf/rhubarb-lip-sync
[6] Epic audio-driven animation documentation:
https://dev.epicgames.com/documentation/en-us/metahuman/audio-driven-animation
[7] Microsoft XAG 104:
https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/104

No exact Genshin dialogue-advance recording was acquired or acoustically compared.
The new cue uses authored oscillator parameters and no outside samples. Its
similarity to the user's remembered sound remains an aesthetic review question,
not a verified measurement. No copied game music, voices or sound assets ship.
