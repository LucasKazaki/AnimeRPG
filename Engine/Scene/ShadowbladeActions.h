#pragma once

#include "Engine/Math/Math.h"
#include "Engine/Scene/CombatSandbox.h"

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

    float Resource() const { return resource_; }
    float DashCooldownRemaining() const { return dashCooldownRemaining_; }
    float FatalStrikeCooldownRemaining() const { return fatalStrikeCooldownRemaining_; }
    bool IsGuarding() const { return guarding_; }
    const ShadowActionReport& LastAction() const { return lastAction_; }
    int PlayerHealth() const { return playerHealth_; }
    int GuardIntegrity() const { return guardIntegrity_; }
    bool HasIncomingAttack() const { return incomingAttackActive_; }
    float IncomingAttackRemaining() const;
    bool HasDefenseCounter() const;
    float DefenseCounterRemaining() const;
    float PerfectDefenseWindowSeconds() const;
    const DefenseReport& LastDefense() const { return lastDefense_; }

private:
    static constexpr std::int64_t DefenseMicrosPerSecond = 1000000;
    static std::int64_t DefenseSecondsToMicros(double seconds);
    static double FloatHalfUlpSeconds(float seconds);
    std::int64_t DefenseTimingToleranceMicros(double deadlineUncertaintySeconds) const;
    static bool DefenseDeadlineReached(std::int64_t now, std::int64_t deadline,
        std::int64_t toleranceMicros);
    static bool DefenseWindowContains(std::int64_t remaining, std::int64_t window,
        std::int64_t toleranceMicros);
    std::int64_t CurrentDefenseMicros() const;
    void RebaseDefenseClock();
    std::int64_t StartDefenseCounterDeadline();
    DefenseReport ResolveIncomingHit(DefenseResult result);

    float resource_{MaximumResource};
    float dashCooldownRemaining_{};
    float fatalStrikeCooldownRemaining_{};
    bool guarding_{};
    ShadowActionReport lastAction_{};

    int playerHealth_{MaximumPlayerHealth};
    int guardIntegrity_{MaximumGuardIntegrity};
    bool incomingAttackActive_{};
    IncomingAttackDefinition incomingAttack_{};
    double defenseElapsedSecondsPrecise_{};
    double defenseElapsedUncertaintySeconds_{};
    std::int64_t incomingAttackEndMicros_{};
    double incomingAttackDeadlineUncertaintySeconds_{};
    std::int64_t defenseCounterEndMicros_{};
    double defenseCounterDeadlineUncertaintySeconds_{};
    DefenseTimingPreset defenseTimingPreset_{DefenseTimingPreset::Standard};
    DefenseReport lastDefense_{};
};

} // namespace Astral::Scene
