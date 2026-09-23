#pragma once

#include "Engine/Math/Math.h"
#include "Engine/Scene/CombatSandbox.h"
#include "Engine/Scene/ShadowbladeLoadout.h"

#include <cstddef>
#include <cstdint>

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
    static ShadowbladeActionTuning ActionTuningForProfile(
        const ShadowbladeLoadoutProfile& profile);
    ShadowbladeActionTuning CurrentLoadoutTuning() const;
    LoadoutActionResult PreviewPresetTuning(std::size_t slot,
        const CharacterProgression& progression, ShadowbladeActionTuning& tuning) const;

private:
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
    float resource_{MaximumResource};
    float dashCooldownRemaining_{};
    float fatalStrikeCooldownRemaining_{};
    bool guarding_{};
    ShadowActionReport lastAction_{};

    int playerHealth_{MaximumPlayerHealth};
    int guardIntegrity_{MaximumGuardIntegrity};
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
