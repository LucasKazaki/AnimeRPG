# Astral Engine: localized-damage contract v0.1

Status: portable opt-in component; not production-wired. See
[design/research](../Design/LOCALIZED-DAMAGE.md) and
[LD-001 task](../../Tasks/LOCALIZED-DAMAGE-2026-09-24.md).

## Ownership boundary
`Engine/Gameplay/LocalizedDamage.h` contains no game classes, graphics, input,
physics dependency or faction lookup. `Game/Combat/LocalizedDamageProfiles.h`
contains proposed game anatomy and a prototype boss action-choice hook. The core
can represent an enemy, player, friendly or construct. Severity is profile data.
No code assumes a human skeleton; PartId is a stable authored identifier.

Production baseline inspected at `2958741188279a0b438dd489ad00cac546012c25`:
`CombatSandbox` owns training-dummy health, attack application, posture, reactions,
finishers and training stats. `ShadowbladeActions` owns player health and defense
state. Do NOT bolt a second authoritative Body onto either and apply both systems.
The next integration must either migrate that actor's health to Body or refactor
the resolver to commit through its existing owner. Keep one authoritative write
per hit and derive UI/stats/rewards from its committed result. LD-001 intentionally
does neither migration. Existing gameplay and root CMake remain unchanged.

## Current data model
Profile: maximum vitality, defeat policy, friendly-fire opt-in, 1-32 parts.
Part: id, optional cover-part id, integrity capacity, armor capacity, integrity
and vitality scales, five damage susceptibilities, impairment threshold, impaired
and disabled efficiencies, affected/blocked capability masks, vital and removable
flags. State: integrity, armor and first-break history. Body: owned profile,
fixed-capacity state, vitality, epoch and per-target contact high-water mark.
Configuration copies data; hit resolution does not allocate or call external code.

All amounts are integers bounded to 1,000,000; zero is allowed for armor/scales.
Scales are permille, bounded 0-8,000. Multiplication uses int64 intermediates,
integer division floors, and every scaling stage saturates to 1,000,000. Tiny hits
can round to zero; there is no forced minimum damage. Threshold comparison uses
cross multiplication, not floats. This avoids floating-point ambiguity in this
component but is not proof of whole-game deterministic replay.

## Authoritative contact pipeline
1. Gameplay owner validates actor lifetime, authority, attack readiness/resources,
   defense outcome, range, real collision and faction policy.
2. Resolve a stable PartId from collider/bone metadata. Selection alone cannot set
   hit location. Avoid skeleton indices that can drift with LOD/reimport.
3. Assign a monotonically increasing sequence PER TARGET, NOT per attacker. Body
   receives already ordered contacts on one simulation thread.
4. Validate raw damage, part, enum, nonzero sequence and current epoch. Invalid or
   stale contacts do not change state. Duplicate/out-of-order sequences are denied.
5. A defeated actor rejects mutation. Otherwise a valid contact consumes sequence
   even when friendly-blocked, covered or aimed at an already removed part. This
   prevents a replay after exposure/repair from becoming a new damaging contact.
6. Compute incoming = clamp(floor(raw * susceptibility / 1000)); armor absorbs
   min(armor, incoming); transmitted = incoming - absorbed.
7. Compute independent integrity and vitality reductions from transmitted energy
   and the corresponding scale. Clamp against remaining pools. VitalPart mode
   applies zero vitality damage. Integrity saturation does not erase body damage
   to a non-removable broken region.
8. Emit one break transition on positive -> zero integrity, with a separate
   firstBreak flag once per part/epoch. Evaluate defeat. Consumers then update
   capabilities, attacks, feedback, metrics and rewards from the committed result.

This is game arithmetic, NOT a physical energy-conservation or ballistics model.
Cover is one prerequisite edge, not physical penetration or anatomical attachment.
Unknown references and cycles are invalid. Current profile validation guarantees
structural/range consistency, not full encounter winnability against a loadout.

## Capability and recovery contract
Capabilities are Mobility, Melee, Ranged, Casting, Flight and Guard. Effects use
healthy/impaired/disabled conditions. Among multiple affected parts, the lowest
current efficiency wins; disabled masks OR together. Unknown capabilities and
unconfigured/defeated actors fail closed. Complex redundant systems require a
future explicit quorum/dependency resolver, not misuse of this OR rule.

A Body efficiency is data, not an actual animation, pathfinding speed or damage
multiplier until a game owner consumes it. Gate new attack selection and re-check
queued actions at execution. Define whether a break cancels the old telegraph;
preserve defense generation/identity and timing invariants. No callbacks from
inside Apply; process resulting events after the authoritative mutation.

RepairPart restores integrity only and keeps everBroken. It cannot restore an
already removed part, armor or a defeated actor. HealVitality restores vitality
only and cannot revive. A new epoch resets the entire profile; Configure rejects
zero/equal/older epochs. The world owner must not recycle an actor/encounter epoch
or wrap counters. Persist reward identity at encounter scope before save/load;
firstBreak alone is not durable cross-save anti-farming protection.

## Engine follow-up packages
LD-E02: add confirmed hit-region mapping for 3D and 2D, bone-local proxies, swept
melee, nearest blocking surface and stable id validation. Capabilities alone do
not establish skeleton/collision readiness.
LD-E03: agree one health-owner migration and add a single integration path for
ordinary attacks, FatalStrike, reaction bonus, finisher, defense damage and hazards.
Preserve posture, timings, statistics, retries, mission accounting and generation
checks. Legacy no-anatomy actors must retain exactly their old behavior.
LD-E04: root-test registration coordinated with the engine worker; exact-head
Windows Debug/Release regression and an independently reviewed receipt.
LD-E05: designer authoring, schema versioning, part asset/bone validation, debug
view, missing/duplicate-id diagnostics and read-only damage inspector.
LD-E06: bounded AoE distribution, multi-hit subevents and replay/lifetime fuzzing.
Network authority/replication remains a later task, not a networking change now.

## Native acceptance matrix (not run)
Trace a moving boss region at multiple frame rates; do not hit hidden/far-side
parts through armor. Switch LOD and animation pose without changing ids. Hit a
part with melee, projectile and splash; splash must not multiply body damage by
collider count. Break during a telegraph and during defense resolution. Confirm
one health/stat/reward commit. Test phase changes, reset, replay, loadout, revive,
friendlies and status stacking. Re-run no-anatomy legacy regression. Inspect
keyboard/mouse and controller behavior, readable UI and accessible targeting.
Measure simulation/query and rendering cost separately in representative crowds
and retain real frame-time/RAM/VRAM evidence. No performance threshold is claimed
measured by LD-001's tiny portable tests.
