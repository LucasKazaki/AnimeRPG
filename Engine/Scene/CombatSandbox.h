#pragma once

#include "Engine/Math/Math.h"

#include <cstdint>

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

enum class CombatAssistPreset {
    Standard,
    Accessible,
};

class CombatSandbox {
public:
    static constexpr float ComboWindowSeconds = 1.25f;
    static constexpr float StaggerDurationSeconds = 1.5f;
    static constexpr float PostureRecoveryDelaySeconds = 1.5f;
    static constexpr float PostureRecoveryPerSecond = 35.0f;
    static constexpr int StandardComboFinisherHits = 3;
    static constexpr int AccessibleComboFinisherHits = 2;
    static constexpr int ComboFinisherDamage = 45;
    static constexpr int EclipseReactionDamage = 20;

    CombatSandbox();

    void AdvanceTime(float deltaSeconds);
    AttackReport TryAttack(AttackType type, const Math::Vec3& attackerPosition);
    int ApplyDamage(int damage);
    void RegisterSuccessfulAttackHit();
    bool ConsumeStaggerOpening();
    void ResetTrainingSession();
    ComboFinisherReport TryComboFinisher(const Math::Vec3& attackerPosition);
    ManaReactionReport ApplyManaAffinity(ManaAffinity affinity);
    void SetCombatAssistPreset(CombatAssistPreset preset) { assistPreset_ = preset; }

    const TrainingDummy& Dummy() const { return dummy_; }
    const AttackReport& LastAttack() const { return lastAttack_; }
    const TrainingStats& Stats() const { return stats_; }
    float ElapsedSeconds() const;
    float CooldownRemaining() const;
    float StaggerRemaining() const;
    bool IsStaggered() const;
    int ComboCount() const { return comboCount_; }
    bool ComboFinisherReady() const;
    int ComboFinisherRequiredHits() const;
    ManaAffinity TargetAffinity() const { return targetAffinity_; }
    CombatAssistPreset AssistPreset() const { return assistPreset_; }
    float TrainingDps() const;
    const AttackDefinition& Definition(AttackType type) const;

private:
    static constexpr std::int64_t MicrosPerSecond = 1000000;
    static std::int64_t SecondsToMicros(float seconds);
    std::int64_t CurrentMicros() const;
    bool ApplyPostureDamage(int postureDamage);
    void RegisterComboHit();

    TrainingDummy dummy_{};
    AttackDefinition lightAttack_{25, 0.4f, 3.5f, 25};
    AttackDefinition heavyAttack_{60, 1.0f, 3.5f, 70};
    AttackReport lastAttack_{AttackType::Light, AttackResult::Ready, 0, 0, false};
    TrainingStats stats_{};
    double elapsedSecondsPrecise_{};
    std::int64_t nextAttackMicros_{};
    std::int64_t staggerEndMicros_{};
    std::int64_t lastPostureHitMicros_{};
    int postureAtRecoveryStart_{};
    std::int64_t lastComboHitMicros_{-1000000000};
    int comboCount_{};
    ManaAffinity targetAffinity_{ManaAffinity::None};
    CombatAssistPreset assistPreset_{CombatAssistPreset::Standard};
};

} // namespace Astral::Scene
