#pragma once

#include "Engine/Math/Math.h"
#include "Engine/Scene/CombatSandbox.h"
#include "Engine/Scene/ShadowbladeLoadout.h"
#include "Engine/Scene/ShadowbladeLoadoutWorkbench.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

namespace Astral::Scene {

enum class ShadowActionType {
    None,
    Dash,
    FatalStrike,
    Guard,
};

enum class ShadowActionResult {
    Ready,
    Activated,
    Guarding,
    GuardedConflict,
    Cooldown,
    InsufficientResource,
    OutOfRange,
    TargetDefeated,
};

struct ShadowActionReport {
    ShadowActionType type{ShadowActionType::None};
    ShadowActionResult result{ShadowActionResult::Ready};
    int damageApplied{};
    Math::Vec3 dashDestination{};
    bool followUp{};
    float resourceSpent{};
};

enum class DefenseInput {
    Guard,
    Dodge,
};

enum class DefenseTimingPreset {
    Standard,
    Forgiving,
};

enum class DefenseResult {
    None,
    ThreatQueued,
    NoThreat,
    TooEarly,
    Guarded,
    PerfectGuard,
    GuardBroken,
    Evaded,
    PerfectDodge,
    Hit,
    UnblockableHit,
    InvalidThreat,
    Paused,
};

struct IncomingAttackDefinition {
    float windupSeconds{1.0f};
    int damage{20};
    int guardDamage{30};
    bool blockable{true};
};

struct DefenseReport {
    DefenseResult result{DefenseResult::None};
    int damageTaken{};
    int guardDamageTaken{};
    bool counterGranted{};
    float timeToImpact{};
};

struct ShadowbladeActionTuning {
    int fatalStrikeDamage{};
    float dashDistance{};
    float resourceRegenerationPerSecond{};
    int guardDamageMitigation{};
    int readinessScore{};
    int activeResonanceFamilies{};
};

struct ShadowActionReadiness {
    float resource{};
    float dashCooldownRemaining{};
    float fatalStrikeCooldownRemaining{};
    float dashCooldownNormalized{};
    float fatalStrikeCooldownNormalized{};
    bool dashResourceAffordable{};
    bool fatalStrikeBaseResourceAffordable{};
    bool guarding{};
    int shadowMomentum{};
};

struct ProgressionCombatBriefing {
    int shadowStepTier{};
    int eclipseEdgeTier{};
    int breakerFocusTier{};
    int dashSkillRank{1};
    int fatalStrikeSkillRank{1};
    int defenseSkillRank{1};
    float dashResourceRefund{};
    float dashEffectiveCost{};
    float fatalStrikeResourceRefund{};
    float fatalStrikeEffectiveCost{};
    float defenseCounterResourceRefund{};
    float defenseCounterEffectiveCost{};
    float staggerFollowUpResourceRefund{};
    float staggerFollowUpEffectiveCost{};
    float perfectDefenseResourceRestore{};
};

struct ProgressionShadowActionReport {
    ShadowActionReport action{};
    float resourceRestored{};
    float effectiveResourceSpent{};
};

struct ProgressionDefenseReport {
    DefenseReport defense{};
    float resourceRestored{};
};

class ShadowbladeActions {
public:
    static constexpr float MaximumResource = 100.0f;
    static constexpr float ResourceRegenerationPerSecond = 15.0f;
    static constexpr float DashCost = 25.0f;
    static constexpr float DashCooldownSeconds = 1.0f;
    static constexpr float DashDistance = 6.0f;
    static constexpr float FatalStrikeCost = 50.0f;
    static constexpr float StaggerFollowUpCost = 30.0f;
    static constexpr float DefenseCounterFatalStrikeCost = 35.0f;
    static constexpr float FatalStrikeCooldownSeconds = 2.0f;
    static constexpr float FatalStrikeRange = 3.5f;
    static constexpr int FatalStrikeDamage = 80;
    static constexpr int MaximumPlayerHealth = 100;
    static constexpr int MaximumGuardIntegrity = 100;
    static constexpr float StandardPerfectDefenseWindowSeconds = 0.12f;
    static constexpr float ForgivingPerfectDefenseWindowSeconds = 0.20f;
    static constexpr float DodgeWindowSeconds = 0.35f;
    static constexpr float DefenseCounterWindowSeconds = 0.8f;

    static constexpr int MaximumShadowMomentum = 3;
    static constexpr int MomentumFatalStrikeDamageBonus = 12;
    static constexpr float RiftsteelFollowUpCostReduction = 5.0f;
    static constexpr int CryoEdgePerfectGuardRestore = 20;
    static constexpr float TrainingBladePerfectDodgeDashCooldownReductionSeconds = 0.5f;

    static constexpr int BaselineLoadoutAttackBonus = 4;
    static constexpr int BaselineLoadoutGuardBonus = 1;
    static constexpr int BaselineLoadoutResourceRecoveryBonus = 0;
    static constexpr int BaselineLoadoutMobilityBonus = 0;
    static constexpr int LoadoutAttackDamagePerPoint = 2;
    static constexpr int MaximumLoadoutAttackDamageBonus = 40;
    static constexpr float LoadoutMobilityDistancePerPoint = 0.25f;
    static constexpr float MaximumLoadoutDashDistanceBonus = 2.0f;
    static constexpr float LoadoutResourceRegenerationPerPoint = 1.0f;
    static constexpr float MaximumLoadoutResourceRegenerationBonus = 10.0f;
    static constexpr int LoadoutGuardMitigationPerPoint = 2;
    static constexpr int MaximumLoadoutGuardDamageMitigation = 30;

    // Pass 33: bounded progression-to-combat effects. These intentionally alter
    // resource economy rather than generic engine timing or renderer behavior.
    static constexpr float ShadowStepDashRefundPerTier = 2.0f;
    static constexpr float EclipseEdgeCounterRefundPerTier = 2.0f;
    static constexpr float BreakerFocusStaggerRefundPerTier = 2.0f;
    static constexpr float SkillResourceRefundPerBonusRank = 1.0f;
    static constexpr float DefensePerfectRestorePerBonusRank = 2.0f;

    void AdvanceTime(float deltaSeconds);
    float RestoreResource(float amount);
    void SetGuarding(bool guarding);
    ShadowActionReport TryDash(const Math::Vec3& position,
        const Math::Vec3& direction = {0.0f, 1.0f, 0.0f});
    ShadowActionReport TryFatalStrike(const Math::Vec3& position, CombatSandbox& combatSandbox);

    bool BeginIncomingAttack(const IncomingAttackDefinition& attack);
    DefenseReport TryDefend(DefenseInput input);
    void SetDefenseTimingPreset(DefenseTimingPreset preset) { defenseTimingPreset_ = preset; }
    void ResetDefenseState();
    void ResetTransientStatePreservingLoadout();
    bool CancelIncomingAttack() {
        if (!incomingAttackActive_) return false;
        incomingAttackActive_ = false;
        incomingAttackEndSeconds_ = 0.0;
        incomingAttackDeadlineUncertaintySeconds_ = 0.0;
        RebaseDefenseClock();
        return true;
    }

    float Resource() const { return resource_; }
    float DashCooldownRemaining() const { return dashCooldownRemaining_; }
    float FatalStrikeCooldownRemaining() const { return fatalStrikeCooldownRemaining_; }
    bool IsGuarding() const { return guarding_; }
    const ShadowActionReport& LastAction() const { return lastAction_; }
    int PlayerHealth() const { return playerHealth_; }
    int GuardIntegrity() const { return guardIntegrity_; }
    int ShadowMomentum() const { return shadowMomentum_; }
    bool HasIncomingAttack() const { return incomingAttackActive_; }
    std::uint64_t IncomingAttackGeneration() const { return incomingAttackGeneration_; }
    float IncomingAttackRemaining() const;
    bool HasDefenseCounter() const;
    float DefenseCounterRemaining() const;
    float PerfectDefenseWindowSeconds() const;
    bool PerfectDefenseWindowOpen() const {
        if (!incomingAttackActive_) return false;
        const double now = CurrentDefenseSeconds();
        const double remaining = incomingAttackEndSeconds_ > now
            ? incomingAttackEndSeconds_ - now
            : 0.0;
        const float windowSeconds = PerfectDefenseWindowSeconds();
        const double tolerance = DefenseTimingToleranceSeconds(
            incomingAttackDeadlineUncertaintySeconds_ + FloatHalfUlpSeconds(windowSeconds));
        return DefenseWindowContains(remaining, static_cast<double>(windowSeconds), tolerance);
    }
    bool DodgeWindowOpen() const {
        if (!incomingAttackActive_) return false;
        const double now = CurrentDefenseSeconds();
        const double remaining = incomingAttackEndSeconds_ > now
            ? incomingAttackEndSeconds_ - now
            : 0.0;
        const double tolerance = DefenseTimingToleranceSeconds(
            incomingAttackDeadlineUncertaintySeconds_ + FloatHalfUlpSeconds(DodgeWindowSeconds));
        return DefenseWindowContains(
            remaining, static_cast<double>(DodgeWindowSeconds), tolerance);
    }
    const DefenseReport& LastDefense() const { return lastDefense_; }

    ShadowbladeLoadout& Loadout() { return loadout_; }
    const ShadowbladeLoadout& Loadout() const { return loadout_; }
    const ShadowbladeLoadoutWorkbench& LoadoutWorkbench() const { return loadoutWorkbench_; }
    LoadoutActionResult ApplyLoadoutPreset(std::size_t slot,
        const CharacterProgression& progression) {
        return loadoutWorkbench_.ApplyPreset(loadout_, slot, progression);
    }
    LoadoutActionResult ReapplyLastLoadoutPreset(const CharacterProgression& progression) {
        return loadoutWorkbench_.ReapplyLastPreset(loadout_, progression);
    }
    bool HasLastAppliedLoadoutPreset() const {
        return loadoutWorkbench_.HasLastAppliedPreset(loadout_);
    }
    std::size_t LastAppliedLoadoutPreset() const {
        return loadoutWorkbench_.LastAppliedPreset(loadout_);
    }
    PresetLabelResult SetLoadoutPresetLabel(std::size_t slot,
        const std::string& label) {
        return loadoutWorkbench_.SetPresetLabel(loadout_, slot, label);
    }
    PresetLabelResult ClearLoadoutPresetLabel(std::size_t slot) {
        return loadoutWorkbench_.ClearPresetLabel(loadout_, slot);
    }
    bool HasLoadoutPresetLabel(std::size_t slot) const {
        return loadoutWorkbench_.HasPresetLabel(loadout_, slot);
    }
    std::string LoadoutPresetLabel(std::size_t slot) const {
        return loadoutWorkbench_.PresetLabel(loadout_, slot);
    }
    static ShadowbladeActionTuning ActionTuningForProfile(
        const ShadowbladeLoadoutProfile& profile);
    ShadowbladeActionTuning CurrentLoadoutTuning() const;
    ShadowActionReadiness CurrentActionReadiness() const;
    LoadoutActionResult PreviewPresetTuning(std::size_t slot,
        const CharacterProgression& progression, ShadowbladeActionTuning& tuning) const;

    ProgressionCombatBriefing CurrentProgressionCombatBriefing(
        const CharacterProgression& progression) const {
        ProgressionCombatBriefing briefing{};
        briefing.shadowStepTier = EquippedTalentTier(progression, CoreTalent::ShadowStep);
        briefing.eclipseEdgeTier = EquippedTalentTier(progression, CoreTalent::EclipseEdge);
        briefing.breakerFocusTier = EquippedTalentTier(progression, CoreTalent::BreakerFocus);
        briefing.dashSkillRank = progression.SkillRank(ShadowSkill::Dash);
        briefing.fatalStrikeSkillRank = progression.SkillRank(ShadowSkill::FatalStrike);
        briefing.defenseSkillRank = progression.SkillRank(ShadowSkill::Defense);

        const int dashBonusRanks = BonusSkillRanks(briefing.dashSkillRank);
        const int fatalBonusRanks = BonusSkillRanks(briefing.fatalStrikeSkillRank);
        const int defenseBonusRanks = BonusSkillRanks(briefing.defenseSkillRank);
        briefing.dashResourceRefund =
            ShadowStepDashRefundPerTier * static_cast<float>(briefing.shadowStepTier)
            + SkillResourceRefundPerBonusRank * static_cast<float>(dashBonusRanks);
        briefing.fatalStrikeResourceRefund =
            SkillResourceRefundPerBonusRank * static_cast<float>(fatalBonusRanks);
        briefing.defenseCounterResourceRefund = briefing.fatalStrikeResourceRefund
            + EclipseEdgeCounterRefundPerTier * static_cast<float>(briefing.eclipseEdgeTier);
        briefing.staggerFollowUpResourceRefund = briefing.fatalStrikeResourceRefund
            + BreakerFocusStaggerRefundPerTier * static_cast<float>(briefing.breakerFocusTier);
        briefing.perfectDefenseResourceRestore =
            DefensePerfectRestorePerBonusRank * static_cast<float>(defenseBonusRanks);

        const float followUpWeaponReduction =
            loadout_.EquippedWeapon() == ShadowbladeWeapon::RiftsteelSabre
                ? RiftsteelFollowUpCostReduction
                : 0.0f;
        briefing.dashEffectiveCost =
            std::max(0.0f, DashCost - briefing.dashResourceRefund);
        briefing.fatalStrikeEffectiveCost =
            std::max(0.0f, FatalStrikeCost - briefing.fatalStrikeResourceRefund);
        briefing.defenseCounterEffectiveCost = std::max(
            0.0f, DefenseCounterFatalStrikeCost - followUpWeaponReduction
                - briefing.defenseCounterResourceRefund);
        briefing.staggerFollowUpEffectiveCost = std::max(
            0.0f, StaggerFollowUpCost - followUpWeaponReduction
                - briefing.staggerFollowUpResourceRefund);
        return briefing;
    }

    ProgressionShadowActionReport TryProgressionDash(
        const Math::Vec3& position, const Math::Vec3& direction,
        const CharacterProgression& progression) {
        ProgressionShadowActionReport report{};
        report.action = TryDash(position, direction);
        report.effectiveResourceSpent = report.action.resourceSpent;
        if (report.action.result != ShadowActionResult::Activated) return report;

        const ProgressionCombatBriefing briefing =
            CurrentProgressionCombatBriefing(progression);
        report.resourceRestored = RestoreResource(std::min(
            report.action.resourceSpent, briefing.dashResourceRefund));
        report.effectiveResourceSpent =
            std::max(0.0f, report.action.resourceSpent - report.resourceRestored);
        return report;
    }

    ProgressionShadowActionReport TryProgressionFatalStrike(
        const Math::Vec3& position, CombatSandbox& combatSandbox,
        const CharacterProgression& progression) {
        const bool staggerOpening = combatSandbox.IsStaggered();
        const bool counterOpening = !staggerOpening && HasDefenseCounter();

        ProgressionShadowActionReport report{};
        report.action = TryFatalStrike(position, combatSandbox);
        report.effectiveResourceSpent = report.action.resourceSpent;
        if (report.action.result != ShadowActionResult::Activated) return report;

        const ProgressionCombatBriefing briefing =
            CurrentProgressionCombatBriefing(progression);
        float requestedRefund = briefing.fatalStrikeResourceRefund;
        if (staggerOpening) {
            requestedRefund = briefing.staggerFollowUpResourceRefund;
        } else if (counterOpening) {
            requestedRefund = briefing.defenseCounterResourceRefund;
        }
        report.resourceRestored = RestoreResource(std::min(
            report.action.resourceSpent, requestedRefund));
        report.effectiveResourceSpent =
            std::max(0.0f, report.action.resourceSpent - report.resourceRestored);
        return report;
    }

    ProgressionDefenseReport TryProgressionDefend(
        DefenseInput input, const CharacterProgression& progression) {
        ProgressionDefenseReport report{};
        report.defense = TryDefend(input);
        if (report.defense.result != DefenseResult::PerfectGuard
            && report.defense.result != DefenseResult::PerfectDodge) {
            return report;
        }

        const ProgressionCombatBriefing briefing =
            CurrentProgressionCombatBriefing(progression);
        report.resourceRestored = RestoreResource(briefing.perfectDefenseResourceRestore);
        return report;
    }

private:
    static int EquippedTalentTier(
        const CharacterProgression& progression, CoreTalent talent) {
        for (std::size_t slot = 0; slot < CharacterProgression::EquippedTalentSlots; ++slot) {
            if (progression.TalentSlotOccupied(slot)
                && progression.EquippedTalent(slot) == talent) {
                return std::max(0, progression.TalentTier(talent));
            }
        }
        return 0;
    }

    static int BonusSkillRanks(int rank) {
        return std::max(0, rank - 1);
    }

    static double FloatHalfUlpSeconds(float seconds);
    double DefenseTimingToleranceSeconds(double deadlineUncertaintySeconds) const;
    static bool DefenseDeadlineReached(double now, double deadline, double toleranceSeconds);
    static bool DefenseWindowContains(double remaining, double window, double toleranceSeconds);
    static ShadowbladeActionTuning BuildLoadoutTuning(const ShadowbladeLoadout& loadout);
    double CurrentDefenseSeconds() const { return defenseElapsedSecondsPrecise_; }
    void RebaseDefenseClock();
    double StartDefenseCounterDeadline();
    DefenseReport ResolveIncomingHit(DefenseResult result);

    ShadowbladeLoadout loadout_{};
    ShadowbladeLoadoutWorkbench loadoutWorkbench_{};
    float resource_{MaximumResource};
    float dashCooldownRemaining_{};
    float fatalStrikeCooldownRemaining_{};
    bool guarding_{};
    ShadowActionReport lastAction_{};

    int playerHealth_{MaximumPlayerHealth};
    int guardIntegrity_{MaximumGuardIntegrity};
    int shadowMomentum_{};
    bool incomingAttackActive_{};
    std::uint64_t incomingAttackGeneration_{};
    IncomingAttackDefinition incomingAttack_{};
    double defenseElapsedSecondsPrecise_{};
    double defenseElapsedUncertaintySeconds_{};
    double incomingAttackEndSeconds_{};
    double incomingAttackDeadlineUncertaintySeconds_{};
    double defenseCounterEndSeconds_{};
    double defenseCounterDeadlineUncertaintySeconds_{};
    DefenseTimingPreset defenseTimingPreset_{DefenseTimingPreset::Standard};
    DefenseReport lastDefense_{};
};

} // namespace Astral::Scene
