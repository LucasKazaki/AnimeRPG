# Localized damage: tactical anatomy, not just more health bars

Status: researched proposal + portable C++17 foundation, September 24, 2026.
NOT connected to AstralGame. Start with [the task](../../Tasks/LOCALIZED-DAMAGE-2026-09-24.md),
[engine contract](../Engine/LOCALIZED-DAMAGE-CONTRACT.md),
[game encounters](../Game/LOCALIZED-DAMAGE-ENCOUNTERS.md), and
[machine-readable backlog](localized-damage-backlog.json).

## Direction
Lucas proposed selectable boss weak points and War Thunder-like functional damage
across bosses, ordinary enemies, the player and friendlies. Implement the same
underlying anatomy rules with different, authored recovery/severity policies.
Targeting should answer: "Do I shorten this fight, stop its dangerous attack, slow
its pursuit, remove protection, or set up my next character?" Not simply "Which
spot has the largest multiplier?" Preserve Astral Engine and the original
supernatural Washington, DC setting. Neither Unreal migration nor copied content
is authorized by this design.

## Research and what to borrow
Sources checked September 24, 2026. These are design references, not code or asset
licenses. Historical developer descriptions are not promises about current tuning.

| Reference | Supported mechanic | Adaptation, not a claim about AnimeRPG today |
|---|---|---|
| War Thunder developer damage-model diary [1] | Damage to engines, movement and aiming systems has distinct functional consequences; destruction can follow critical component loss. | Map parts to capabilities; permit vital-part defeat instead of a mandatory global HP drain. |
| Current War Thunder module documentation [2] | Different modules degrade continuously or only at failure; damage can disable firing, aiming, mobility or electronics; repair can restore function. | Author different threshold curves, dependencies, repair policies and fallback behavior. The official-hosted wiki is descriptive, not source code. |
| Guerrilla's Forbidden West gameplay reveal [3] | Specialized ammunition strips armor to expose weak spots. | Separate armor removal from direct health damage; make setup attacks useful to teammates. |
| Capcom's Wilds Focus Mode description [4] | Focus Mode aims attacks/guard and highlights wounds and weak points. | Explicit part selection and optional readable highlights. Page body was access-blocked here; only the indexed official description was available. Do not infer detailed wound formulas. |
| Motive's Dead Space developer interview [5] | Layered damage provides visible feedback and supports different weapon roles. | Show damage through cracked armor, broken runes, sparks and posture before requiring gore or complex mesh surgery. |
| Guerrilla's combat/animation discussion [6] | Readable silhouettes, telegraphs, responses and audiovisual cues support tactical combat. | Every meaningful impairment needs a visible behavior change and clear feedback, not a hidden spreadsheet penalty. |
| Epic's Gameplay Ability System documentation [7] | Attributes, effects and abilities can be separated into reusable gameplay infrastructure. | Keep engine damage rules generic and game ability/AI decisions outside them; no GAS/Unreal dependency is added. |

## How far customization can go
These tiers are proposed scope, not completion claims or measured budgets.

| Tier | Customization | Admission |
|---|---|---|
| 1: regions | Authored hit regions, part integrity, weakness/resistance by damage type, separate vitality transfer, explicit defeat modes. | Portable foundation now; collision integration next. |
| 2: functional anatomy | Armor, cover/exposure chains, impairment thresholds, disabled capabilities, repair, player/ally severity differences. | A bounded subset is in the foundation; real ability consumers and animations remain next. |
| 3: tactical systems | Temporary wounds, phase-specific exposure, stance-dependent armor, coordinated team setups, interruptible regeneration, redundant organs/limbs and detachable equipment. | Dependency-ordered follow-ups; no silent support implied. |
| 4: simulation-heavy | Multi-layer penetration paths, directional protection, heat, bleeding, internal dependency graphs, structural failure and physical detachment. | Optional later engine feature set, requiring separate performance, art and playability gates. |
| 5: arbitrary anatomy | Procedural creatures, arbitrary mesh cutting, regrown topology and generalized physically constrained locomotion. | Research track, not a launch requirement or automatic consequence of part HP. |

An engine can expose many parameters without asking players to manage all of them.
Proposed authoring budgets: ordinary enemies 2-4 meaningful regions, elites 4-8,
bosses 8-16. The current reference component deliberately caps profiles at 32 parts
for bounded processing; this is not a measured hardware ceiling. Both initial
humanoid and boss examples have six. Favor fewer mechanically distinct parts over
many cosmetic weak spots. Once artist-authored anatomy is stable, share damage
state variants across skeletal regions instead of building every combination as a
unique mesh. Four visual states over eight independent parts already imply
4^8 = 65,536 combinations; compositional animation/material layers are necessary.

## Health and defeat
Do not equate component simulation with "no numbers." The useful distinction is
that damage changes capabilities and can satisfy an explicit defeat condition.
Support three policies: vitality exhaustion; destruction of ANY marked vital
part; or either condition. A UI bar is a presentation choice, not the damage model.
Recommended starting policy: hybrid for most bosses; vitality plus temporary,
bounded impairment for players/allies; module-only for appropriate constructs.
The reference's vital-only mode ignores global vitality damage entirely. More
complex rules such as ALL cores destroyed or TWO of FOUR legs broken are backlog,
not implemented by the current ANY-vital rule.

## Separate channels and consequences
Keep global vitality, part integrity, armor and existing posture/stagger separate.
A broad slash may deal high vitality damage but low precision part damage; a
focused thrust may pressure one region; a breaker may strip armor with little
vitality transfer. These are proposed authored profiles, not finalized balance.
Solar and Umbral susceptibility are included alongside impact, slash and pierce.
Temporary elemental buildup, wounds, break resistance and posture remain distinct
future channels so one damage field cannot accidentally trigger every reward.

Two hits to the same region can have different effects because of its armor,
state, attack kind and exposure. Breaking an arm should remove its authored attack
family; damaging a leg should shorten a charge or widen recovery; damaging a focus
should change spell options; removing armor should expose a high-risk/high-reward
core. Some optional late bosses may exchange a disabled move for a telegraphed
fallback. Never retroactively nullify a player's successful break with an
unannounced phase reset.

## Controls and fairness
Part selection is intent, NOT evidence of a hit. Offer ordinary enemy lock-on,
part-cycle controls, optional focus/slowdown in single-player, and aim assistance.
Show only relevant exposed/reachable parts; retain normal body attacks. Melee
selection guides allowed attack arcs, not teleportation through the target. Ranged
shots must follow their actual trajectory. Fast melee needs swept contact tests.
An area attack receives one per-target damage budget, not full damage for every
collider overlapped. Authored multi-hit moves get explicit sub-hit identities.

Avoid player death spirals: no random permanent arm loss, inability to dodge for a
whole fight, compulsory treatment between every skirmish, or ally micromanagement
by default. The proposed player damage floor preserves 80% function on a disabled
region and the ally floor 90%; these values require playtesting. Injured regions
use 90%/95%. Apply the worst relevant penalty rather than multiplying penalties
from both legs. Ordinary healing restores vitality; explicit treatment restores
part integrity. Rest/checkpoints should restore function in the default game
policy; timed recovery, checkpoints and revive policies are not implemented yet.
Hardcore persistence, friendly fire and harsher anatomical failure are opt-ins.
Use icons, labels, silhouettes, sound and optional highlights rather than color
alone. Never require tiny moving targets for story-mode progress.

## Scope boundary and next decision
The foundation implements integer damage, one armor pool per part, one cover
prerequisite per part, threshold impairment, disabled capability masks, healing,
repair and deduplicated contacts. It does NOT implement wound generation,
penetration physics, region picking, animation, a designer UI, JSON loading,
network replication, save migration, gameplay balancing or runtime integration.
Profiles currently are compiled C++ data. Provide schema/editor authoring only
after the contracts stabilize. The first acceptance target is one playable boss
with three viable choices (core rush, arm disable, casting disable), one humanoid
training target and one recoverable player impairment. No bulk content first.

## Sources
[1] Gaijin, developer diary (historical): https://warthunder.com/en/news/384-Developers-Diaries-Ground-Forces-Damage-Model-Part-2-en
[2] War Thunder Wiki, Ground vehicle modules: https://wiki.warthunder.com/mechanics/4775-ground-vehicle-modules
[3] Guerrilla, May 27, 2021: https://blog.playstation.com/2021/05/27/14-minutes-of-new-gameplay-for-horizon-forbidden-west/
[4] Capcom, Focus Mode (official indexed description; body access blocked): https://www.monsterhunter.com/wilds/en-us/hunting/focus-mode/
[5] Motive, Inside Dead Space #2: https://www.ea.com/en-ca/playtesting/news/inside-dead-space-2-new-necromorph-nightmare
[6] Guerrilla, December 6, 2021: https://blog.playstation.com/2021/12/06/horizon-forbidden-west-outsmart-your-enemies/
[7] Epic, Gameplay Ability System: https://dev.epicgames.com/documentation/unreal-engine/gameplay-ability-system-for-unreal-engine?lang=en-US
