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

struct PlayerCombatState {
    int maximumHealth{100};
    int health{100};

    bool IsDefeated() const { return health == 0; }
};

struct TrainingStats {
    int totalDamage{};
    int hitCount{};
    int peakHit{};
    int bestCombo{};
};

enum class EnemyAttackResult {
    None,
    Scheduled,
    TooEarly,
    PerfectDodged,
    PerfectGuarded,
    Hit,
    NoAttack,
};

struct EnemyAttackReport {
    EnemyAttackResult result{EnemyAttackResult::None};
    int damageApplied{};
    bool counterGranted{};
    bool parryStaggerTriggered{};
};

enum class CounterAttackResult {
    NotReady,
    OutOfRange,
    TargetDefeated,
    Activated,
};

struct CounterAttackReport {
    CounterAttackResult result{CounterAttackResult::NotReady};
    int damageApplied{};
};

enum class ComboFinisherResult {
    NotReady,
    OutOfRange,
    TargetDefeated,
    Activated,
};

struct ComboFinisherReport {
    ComboFinisherResult result{ComboFinisherResult::NotReady};
    int damageApplied{};
};

enum class ManaAffinity {
    None,
    Solar,
    Umbral,
};

enum class ManaReaction {
    None,
    Eclipse,
};

struct ManaReactionReport {
    ManaAffinity applied{ManaAffinity::None};
    ManaAffinity previous{ManaAffinity::None};
    ManaAffinity remaining{ManaAffinity::None};
    ManaReaction reaction{ManaReaction::None};
    int bonusDamage{};
};

class CombatSandbox {
public:
    static constexpr float ComboWindowSeconds = 1.25f;
    static constexpr float StaggerDurationSeconds = 1.5f;
    static constexpr float PostureRecoveryDelaySeconds = 1.5f;
    static constexpr float PostureRecoveryPerSecond = 35.0f;
    static constexpr float PerfectDodgeWindowSeconds = 0.25f;
    static constexpr float PerfectGuardWindowSeconds = 0.15f;
    static constexpr float CounterWindowSeconds = 1.0f;
    static constexpr float CounterRange = 3.5f;
    static constexpr int CounterDamage = 35;
    static constexpr int PerfectGuardPostureDamage = 40;
    static constexpr int ComboFinisherThreshold = 3;
    static constexpr float ComboFinisherRange = 3.5f;
    static constexpr int ComboFinisherDamage = 25;
    static constexpr int ManaReactionDamage = 12;

    CombatSandbox();

    void AdvanceTime(float deltaSeconds);
    AttackReport TryAttack(AttackType type, const Math::Vec3& attackerPosition);
    int ApplyDamage(int damage);
    bool ConsumeStaggerOpening();
    void ResetTrainingSession();

    bool ScheduleEnemyAttack(float windupSeconds, int damage);
    EnemyAttackReport TryPerfectDodge();
    EnemyAttackReport TryPerfectGuard();
    CounterAttackReport TryCounterAttack(const Math::Vec3& attackerPosition);
    ComboFinisherReport TryComboFinisher(const Math::Vec3& attackerPosition);
    ManaReactionReport ApplyManaAffinity(ManaAffinity affinity);

    const TrainingDummy& Dummy() const { return dummy_; }
    const PlayerCombatState& Player() const { return player_; }
    const AttackReport& LastAttack() const { return lastAttack_; }
    const EnemyAttackReport& LastEnemyAttack() const { return lastEnemyAttack_; }
    const TrainingStats& Stats() const { return stats_; }
    float ElapsedSeconds() const { return elapsedSeconds_; }
    float CooldownRemaining() const;
    float StaggerRemaining() const { return staggerRemaining_; }
    bool IsStaggered() const { return staggerRemaining_ > 0.0f && !dummy_.IsDefeated(); }
    int ComboCount() const { return comboCount_; }
    bool ComboFinisherReady() const { return comboFinisherReady_; }
    bool CounterReady() const { return counterWindowRemaining_ > 0.0; }
    float CounterWindowRemaining() const {
        return static_cast<float>(counterWindowRemaining_);
    }
    bool EnemyAttackActive() const { return enemyAttackActive_; }
    float EnemyAttackRemaining() const {
        return static_cast<float>(enemyAttackRemaining_);
    }
    ManaAffinity TargetAffinity() const { return targetAffinity_; }
    float TrainingDps() const;
    const AttackDefinition& Definition(AttackType type) const;

private:
    bool ApplyPostureDamage(int postureDamage);
    int ApplyPlayerDamage(int damage);
    void RegisterComboHit();
    void ResolveEnemyAttackHit();

    TrainingDummy dummy_{};
    PlayerCombatState player_{};
    AttackDefinition lightAttack_{25, 0.4f, 3.5f, 25};
    AttackDefinition heavyAttack_{60, 1.0f, 3.5f, 70};
    AttackReport lastAttack_{AttackType::Light, AttackResult::Ready, 0, 0, false};
    EnemyAttackReport lastEnemyAttack_{};
    TrainingStats stats_{};
    float elapsedSeconds_{};
    float nextAttackTime_{};
    float staggerRemaining_{};
    double timeSincePostureHit_{};
    int postureAtRecoveryStart_{};
    float lastComboHitTime_{-1000.0f};
    int comboCount_{};
    bool comboFinisherReady_{};
    bool enemyAttackActive_{};
    double enemyAttackRemaining_{};
    int enemyAttackDamage_{};
    double counterWindowRemaining_{};
    ManaAffinity targetAffinity_{ManaAffinity::None};
};

} // namespace Astral::Scene
