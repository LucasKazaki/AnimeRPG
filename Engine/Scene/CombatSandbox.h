#pragma once

#include "Engine/Math/Math.h"

namespace Astral::Scene {

enum class AttackType {
    Light,
    Heavy,
};

enum class AttackResult {
    Ready,
    Hit,
    Cooldown,
    OutOfRange,
    TargetDefeated,
};

struct AttackDefinition {
    int damage{};
    float cooldownSeconds{};
    float range{};
    int postureDamage{};
};

struct AttackReport {
    AttackType type{AttackType::Light};
    AttackResult result{AttackResult::OutOfRange};
    int damageApplied{};
    int comboCount{};
    bool staggerTriggered{};
};

struct TrainingDummy {
    Math::Vec3 position{3.0f, 0.0f, 0.0f};
    int maximumHealth{100};
    int health{100};
    int maximumPosture{80};
    int posture{};

    bool IsDefeated() const { return health == 0; }
};

struct TrainingStats {
    int totalDamage{};
    int hitCount{};
    int peakHit{};
    int bestCombo{};
};

class CombatSandbox {
public:
    static constexpr float ComboWindowSeconds = 1.25f;
    static constexpr float StaggerDurationSeconds = 1.5f;
    static constexpr float PostureRecoveryDelaySeconds = 1.5f;
    static constexpr float PostureRecoveryPerSecond = 35.0f;

    CombatSandbox();

    void AdvanceTime(float deltaSeconds);
    AttackReport TryAttack(AttackType type, const Math::Vec3& attackerPosition);
    int ApplyDamage(int damage);
    bool ConsumeStaggerOpening();
    void ResetTrainingSession();

    const TrainingDummy& Dummy() const { return dummy_; }
    const AttackReport& LastAttack() const { return lastAttack_; }
    const TrainingStats& Stats() const { return stats_; }
    float ElapsedSeconds() const { return elapsedSeconds_; }
    float CooldownRemaining() const;
    float StaggerRemaining() const { return staggerRemaining_; }
    bool IsStaggered() const { return staggerRemaining_ > 0.0f && !dummy_.IsDefeated(); }
    int ComboCount() const { return comboCount_; }
    float TrainingDps() const;
    const AttackDefinition& Definition(AttackType type) const;

private:
    bool ApplyPostureDamage(int postureDamage);
    void RegisterComboHit();

    TrainingDummy dummy_{};
    AttackDefinition lightAttack_{25, 0.4f, 3.5f, 25};
    AttackDefinition heavyAttack_{60, 1.0f, 3.5f, 70};
    AttackReport lastAttack_{AttackType::Light, AttackResult::Ready, 0, 0, false};
    TrainingStats stats_{};
    float elapsedSeconds_{};
    float nextAttackTime_{};
    float staggerRemaining_{};
    float timeSincePostureHit_{};
    float postureRecoveryRemainder_{};
    float lastComboHitTime_{-1000.0f};
    int comboCount_{};
};

} // namespace Astral::Scene
