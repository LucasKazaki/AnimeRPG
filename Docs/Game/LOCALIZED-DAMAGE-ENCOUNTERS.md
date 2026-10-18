# AnimeRPG: localized damage encounter and player plan

Status: original proposals + C++ profile examples, not playable content.
See [shared design](../Design/LOCALIZED-DAMAGE.md),
[engine integration contract](../Engine/LOCALIZED-DAMAGE-CONTRACT.md), and
[backlog](../Design/localized-damage-backlog.json).

## Proposed first boss: Mall Sentinel
An original supernatural construct for the retained DC setting. No real landmark
geometry, third-party character or existing encounter is replaced by this packet.
Six authored regions form the first behavior fixture:

| Part | Integrity / armor | Vitality transfer | Functional result |
|---|---|---|---|
| Chest plate | 100 / 50 | 0x | Break removes plate, disables its guard capability and exposes core. |
| Sealed core | 120 / 0 | 2.5x | Covered until plate breaks; vital-part defeat under hybrid/module-only policy. Solar susceptibility is 1.5x. |
| Weapon arm | 100 / 0 | 0.6x | 50% remaining integrity impairs; break disables authored melee family. |
| Support leg | 100 / 0 | 0.5x | Impaired movement efficiency 80%; broken 55%; no permanent immobility. |
| Rune focus | 80 / 0 | 0.7x | Break disables casting capability. |
| Head/sensor | 80 / 0 | 1.2x | Ranged efficiency becomes 75% impaired and 35% disabled. |

Boss vitality is 1,200. These are transparent fixture numbers, NOT balanced combat.
Default defeat is vitality exhausted OR any vital part broken. Alternate
VitalPart mode has no global-vitality damage. The action-choice fixture chooses
Cleave, then RuneBurst after melee loss, then DesperationPulse after casting loss.
Unconfigured/defeated bodies choose None. This helper only demonstrates state
consumption; it does not make the existing boss AI perform these moves.

Expected tactical routes for playtesting: core rush (exposure risk, short finish),
arm-first (remove melee pressure, retain other threats), or rune-first (remove
casting pressure). Leg targeting should help a slower team but cost direct finish
speed. Ensure each route changes what the player does next. The final fallback
must remain dodgeable, well telegraphed and weaker than the capabilities removed;
otherwise breaking parts feels like a punishment. Prevent infinite topple chains
with explicit stagger/knockdown recovery rules in a later game packet.

## Characters, normal enemies and friendlies
Use common anatomy but severity policies suited to the actor. The prototype
adventurer and companion profiles have torso, head, weapon arm, casting arm and
two legs. Vitality: 100 player / 150 ally. Each region has 10 armor. Head transfers
1.25x vitality damage; it is not an instant-death vital part. Limb integrity is
75 and torso 100. Player efficiencies are 90% impaired/80% disabled; allies
95%/90%. No limb hard-blocks a player/companion capability in these profiles.
Friendly fire is off unless explicitly enabled by the actor's authored policy.

A regular enemy can share the same core but use a shorter two-to-four-region
profile and stronger disable effects. Elite variants can add a shield generator
or focus. Healers should have meaningful choices between restoring vitality and
repairing a limb; tanks could protect a threatened region; a melee breaker could
expose a core for a ranged partner. These class synergies are backlog, not shipped.
Default recovery should prevent an injury-management chore. Rest/checkpoint
recovery, auto-treatment and persistent/hardcore injury are separate policies.

## Game follow-ups
LD-G02: production training adapter after LD-E02/E03. Read the actual committed
hit result and capability state; never independently subtract health or recompute
scores. Keep ShadowbladeActions, CombatSandbox and their existing generation
bindings authoritative until the agreed migration. Coordinate with active game
work on ShadowCryptSkirmish; do not patch another worker's live files here.
LD-G03: one boss encounter, real telegraphs and disabled-move fallbacks, phase
transition preservation and interrupt rules. Existing posture remains separate.
LD-G04: enemy lock-on plus part-cycle focus, text/icon state display, optional
highlights, keyboard/controller support and melee reach constraints. Ordinary
body attacks remain viable. An impossible/hidden part cannot be auto-hit.
LD-G05: recovery and ally fairness; apply effects to real actions, not just a HUD.
LD-G06: evaluate kill time, avoidable damage, targeting success, chosen route,
incapacitation time, recovery burden, accessibility and strategy diversity. Set
targets only after baseline measurement, rather than inventing proof of fun.

## Art/audio handoff
Use readable armor cracks, extinguished runes, staggered posture, restrained sparks
and distinct break sounds. No gore is required. For the first boss, supply region
proxies and stable ids, intact/broken plate variants, a clearly exposed core and
three damage-state poses. Do not produce every possible limb combination as a
separate full model. Native art/animation readiness, source rights, import and
rendering are independent acceptance gates. No generated or copied assets ship
with the reference component.
