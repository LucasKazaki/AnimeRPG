#pragma once
#include "Engine/Gameplay/LocalizedDamage.h"

// Original proposed tuning, not balanced production content. Engine code has no
// knowledge of these ids, classes, encounters, factions or game-specific rules.
namespace AnimeRPG::Combat {
namespace Damage = Astral::Gameplay::LocalizedDamage;

enum class SentinelPart : Damage::PartId {
    ChestPlate = 1, Core = 2, WeaponArm = 3, Leg = 4, RuneFocus = 5, Head = 6
};
inline Damage::Profile MallSentinelProfile(Damage::DefeatMode mode = Damage::DefeatMode::Either) {
    Damage::Profile p;
    p.maximumVitality = 1200;
    p.defeatMode = mode;
    Damage::PartDefinition plate;
    plate.id = 1; plate.maximumIntegrity = 100; plate.maximumArmor = 50;
    plate.vitalityScale = 0; plate.removedWhenBroken = true;
    plate.affectedCapabilities = Damage::Bit(Damage::Capability::Guard);
    plate.blockedWhenDisabled = plate.affectedCapabilities;
    Damage::PartDefinition core;
    core.id = 2; core.exposedAfterBreak = 1; core.maximumIntegrity = 120;
    core.vitalityScale = 2500; core.vital = true;
    core.susceptibility[static_cast<std::size_t>(Damage::DamageKind::Solar)] = 1500;
    Damage::PartDefinition arm;
    arm.id = 3; arm.maximumIntegrity = 100; arm.vitalityScale = 600;
    arm.affectedCapabilities = Damage::Bit(Damage::Capability::Melee);
    arm.blockedWhenDisabled = arm.affectedCapabilities;
    Damage::PartDefinition leg;
    leg.id = 4; leg.maximumIntegrity = 100; leg.vitalityScale = 500;
    leg.affectedCapabilities = Damage::Bit(Damage::Capability::Mobility);
    leg.impairedEfficiency = 800; leg.disabledEfficiency = 550;
    Damage::PartDefinition rune;
    rune.id = 5; rune.maximumIntegrity = 80; rune.vitalityScale = 700;
    rune.affectedCapabilities = Damage::Bit(Damage::Capability::Casting);
    rune.blockedWhenDisabled = rune.affectedCapabilities;
    Damage::PartDefinition head;
    head.id = 6; head.maximumIntegrity = 80; head.vitalityScale = 1200;
    head.affectedCapabilities = Damage::Bit(Damage::Capability::Ranged);
    head.impairedEfficiency = 750; head.disabledEfficiency = 350;
    p.parts = {plate, core, arm, leg, rune, head};
    return p;
}

enum class HumanoidPart : Damage::PartId {
    Torso = 1, Head = 2, WeaponArm = 3, CastingArm = 4, LeftLeg = 5, RightLeg = 6
};
inline Damage::Profile AdventurerProfile() {
    Damage::Profile p;
    p.maximumVitality = 100;
    p.defeatMode = Damage::DefeatMode::Vitality; // No unexpected headshot instadeath.
    for (Damage::PartId id = 1; id <= 6; ++id) {
        Damage::PartDefinition part;
        part.id = id; part.maximumIntegrity = 75; part.maximumArmor = 10;
        part.impairedEfficiency = 900; part.disabledEfficiency = 800;
        if (id == 1) part.maximumIntegrity = 100;
        if (id == 2) part.vitalityScale = 1250;
        if (id == 3) part.affectedCapabilities = Damage::Bit(Damage::Capability::Melee);
        if (id == 4) part.affectedCapabilities = Damage::Bit(Damage::Capability::Casting);
        if (id >= 5) part.affectedCapabilities = Damage::Bit(Damage::Capability::Mobility);
        p.parts.push_back(part);
    }
    return p;
}
inline Damage::Profile CompanionProfile() {
    auto p = AdventurerProfile();
    p.maximumVitality = 150;
    for (auto& part : p.parts) { part.impairedEfficiency = 950; part.disabledEfficiency = 900; }
    return p;
}

enum class SentinelAction { None, Cleave, RuneBurst, HeadShot, Advance, DesperationPulse };
// Prototype selection hook for a future AI adapter. A fallback always exists for
// a LIVE boss: breaking modules must not leave the AI with an empty attack list.
inline SentinelAction ChooseSentinelAction(const Damage::Body& body) {
    if (!body.Configured() || body.Defeated()) return SentinelAction::None;
    if (body.Efficiency(Damage::Capability::Melee) > 0) return SentinelAction::Cleave;
    if (body.Efficiency(Damage::Capability::Casting) > 0) return SentinelAction::RuneBurst;
    return SentinelAction::DesperationPulse;
}
} // namespace AnimeRPG::Combat
