#pragma once

#include "Engine/Math/Math.h"

#include <cstddef>
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

enum class AttackResistance {
    None,
    Light,
    Heavy,
};

enum class EnemyPhase {
    Normal,
    Pressure,
};

enum class EnemyAttackPattern {
    QuickCut,
    GuardBreaker,
    RiftBurst,
};

enum class EnemyAggressionPreset {
    Relaxed,
    Standard,
    Aggressive,
};

enum class EnemyAttackOutcome {
    Hit,
    Guarded,
    Evaded,
    PerfectDefense,
    Interrupted,
};

enum class TechniqueType {
    None,
    Reaction,
    Stagger,
    Finisher,
};

enum class TrainingTargetMode {
    Standard,
    Endless,
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
    bool resistanceApplied{};
    bool staggerBonusApplied{};
    bool defensePunishBonusApplied{};
};

struct EnemyAttackPlan {
    EnemyAttackPattern pattern{EnemyAttackPattern::QuickCut};
    float windupSeconds{0.55f};
    int damage{18};
    int guardDamage{20};
    bool blockable{true};
    float recoverySeconds{0.85f};
};

enum class ManaAffinity {
    None,
    Solar,
    Umbral,
};

enum class TrainingEnemyProfile {
    Standard,
    Vanguard,
    Bulwark,
    Boss,
};

struct TrainingEnemyDefinition {
    int maximumHealth{100};
    int maximumPosture{80};
    ManaAffinity weakness{ManaAffinity::None};
    AttackResistance resistance{AttackResistance::None};
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
    std::int64_t totalDamage{};
    int hitCount{};
    int peakHit{};
    int bestCombo{};
    int reactionCount{};
    int finisherCount{};
    int staggerCount{};
    std::int64_t techniqueScore{};
    int bestTechniqueChain{};
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
    bool eclipseFollowUp{};
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
    bool weaknessExploited{};
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
    static constexpr int WeaknessReactionBonusDamage = 10;
    static constexpr int EclipseFinisherBonusDamage = 20;
    static constexpr float TechniqueChainWindowSeconds = 2.0f;
    static constexpr int MaximumTechniqueChain = 4;
    static constexpr int ReactionTechniquePoints = 20;
    static constexpr int StaggerTechniquePoints = 15;
    static constexpr int FinisherTechniquePoints = 25;
    static constexpr int ResistantAttackDamageNumerator = 4;
    static constexpr int ResistantAttackDamageDenominator = 5;
    static constexpr int StaggerDamageNumerator = 5;
    static constexpr int StaggerDamageDenominator = 4;
    static constexpr int DefensePunishDamageNumerator = 5;
    static constexpr int DefensePunishDamageDenominator = 4;
    static constexpr float DefensePunishWindowSeconds = 1.0f;
    static constexpr float RelaxedEnemyRecoverySeconds = 1.25f;
    static constexpr float StandardEnemyRecoverySeconds = 0.85f;
    static constexpr float AggressiveEnemyRecoverySeconds = 0.45f;
    static constexpr double FastChallengeSeconds = 5.0;
    static constexpr double StandardChallengeSeconds = 10.0;

    CombatSandbox();

    void AdvanceTime(float deltaSeconds);
    AttackReport TryAttack(AttackType type, const Math::Vec3& attackerPosition);
    int ApplyDamage(int damage);
    void RegisterSuccessfulAttackHit();
    bool ConsumeStaggerOpening();
    void ResetTrainingSession();
    void ResetTechniqueChain() {
        techniqueChain_ = 0;
        lastTechniqueMicros_ = -1000000000;
        lastTechniqueType_ = TechniqueType::None;
    }
    bool SetTrainingEnemyProfile(TrainingEnemyProfile profile);
    bool SetTrainingTargetMode(TrainingTargetMode mode);
    bool SetEnemyAggressionPreset(EnemyAggressionPreset preset);
    bool QueueNextEnemyAttack();
    bool QueueEnemyAttackPattern(EnemyAttackPattern pattern) {
        if (dummy_.IsDefeated() || IsStaggered() || enemyAttackPending_
            || CurrentMicros() < enemyAttackReadyMicros_) {
            return false;
        }

        const float baseRecovery = EnemyRecoverySeconds();
        switch (pattern) {
        case EnemyAttackPattern::QuickCut:
            pendingEnemyAttack_ = {pattern, 0.55f, 18, 20, true, baseRecovery};
            break;
        case EnemyAttackPattern::GuardBreaker:
            pendingEnemyAttack_ = {pattern, 0.90f, 28, 55, true, baseRecovery + 0.15f};
            break;
        case EnemyAttackPattern::RiftBurst:
            pendingEnemyAttack_ = {pattern, 0.70f, 34, 0, false, baseRecovery + 0.25f};
            break;
        default:
            return false;
        }

        ++enemyAttackGeneration_;
        if (enemyAttackGeneration_ == 0) ++enemyAttackGeneration_;
        enemyAttackPending_ = true;
        return true;
    }
    bool ResolveEnemyAttack(EnemyAttackOutcome outcome);
    bool SetBossPracticePhase(EnemyPhase phase);
    bool ClearBossPracticePhase();
    ComboFinisherReport TryComboFinisher(const Math::Vec3& attackerPosition);
    ManaReactionReport ApplyManaAffinity(ManaAffinity affinity);
    void SetCombatAssistPreset(CombatAssistPreset preset) { assistPreset_ = preset; }

    const TrainingDummy& Dummy() const { return dummy_; }
    const AttackReport& LastAttack() const { return lastAttack_; }
    const TrainingStats& Stats() const { return stats_; }
    float ElapsedSeconds() const;
    double ElapsedSecondsPrecise() const { return elapsedSecondsPrecise_; }
    float CooldownRemaining() const;
    float StaggerRemaining() const;
    bool IsStaggered() const;
    int ComboCount() const { return comboCount_; }
    bool ComboFinisherReady() const;
    int ComboFinisherRequiredHits() const;
    ManaAffinity TargetAffinity() const { return targetAffinity_; }
    CombatAssistPreset AssistPreset() const { return assistPreset_; }
    TrainingEnemyProfile EnemyProfile() const { return enemyProfile_; }
    TrainingTargetMode TargetMode() const { return targetMode_; }
    EnemyPhase CurrentEnemyPhase() const;
    TrainingEnemyDefinition CurrentEnemyDefinition() const;
    EnemyAggressionPreset AggressionPreset() const { return enemyAggressionPreset_; }
    bool HasPendingEnemyAttack() const { return enemyAttackPending_; }
    const EnemyAttackPlan& PendingEnemyAttack() const { return pendingEnemyAttack_; }
    std::uint64_t EnemyAttackGeneration() const { return enemyAttackGeneration_; }
    float EnemyAttackReadyInSeconds() const;
    bool DefensePunishOpeningReady() const;
    bool BossPracticePhaseLocked() const { return bossPracticePhaseLocked_; }
    EnemyPhase BossPracticePhase() const { return bossPracticePhase_; }
    bool EclipseOpeningReady() const { return eclipseOpening_; }
    int TechniqueChain() const { return techniqueChain_; }
    TechniqueType LastTechniqueType() const { return lastTechniqueType_; }
    std::int64_t TrainingChallengeScore() const;
    float TrainingDps() const;
    const AttackDefinition& Definition(AttackType type) const;

private:
    static constexpr std::int64_t MicrosPerSecond = 1000000;
    static std::int64_t SecondsToMicros(float seconds);
    std::int64_t CurrentMicros() const;
    bool ApplyPostureDamage(int postureDamage);
    void RegisterComboHit();
    void RegisterTechnique(TechniqueType type, int basePoints);
    int AdjustDirectAttackDamage(AttackType type, int damage,
        bool& resistanceApplied, bool& staggerBonusApplied) const;
    EnemyAttackPlan BuildEnemyAttackPlan(std::size_t sequenceIndex) const;
    float EnemyRecoverySeconds() const;
    void ClearEnemyAttackState();

    TrainingDummy dummy_{};
    AttackDefinition lightAttack_{25, 0.4f, 3.5f, 25};
    AttackDefinition heavyAttack_{60, 1.0f, 3.5f, 70};
    AttackReport lastAttack_{AttackType::Light, AttackResult::Ready, 0, 0, false, false, false, false};
    TrainingStats stats_{};
    double elapsedSecondsPrecise_{};
    double targetDefeatElapsedSeconds_{-1.0};
    std::int64_t nextAttackMicros_{};
    std::int64_t staggerEndMicros_{};
    std::int64_t lastPostureHitMicros_{};
    int postureAtRecoveryStart_{};
    std::int64_t lastComboHitMicros_{-1000000000};
    int comboCount_{};
    ManaAffinity targetAffinity_{ManaAffinity::None};
    bool eclipseOpening_{};
    int techniqueChain_{};
    std::int64_t lastTechniqueMicros_{-1000000000};
    TechniqueType lastTechniqueType_{TechniqueType::None};
    TrainingEnemyProfile enemyProfile_{TrainingEnemyProfile::Standard};
    TrainingTargetMode targetMode_{TrainingTargetMode::Standard};
    CombatAssistPreset assistPreset_{CombatAssistPreset::Standard};

    EnemyAggressionPreset enemyAggressionPreset_{EnemyAggressionPreset::Standard};
    std::size_t enemyAttackSequenceIndex_{};
    std::uint64_t enemyAttackGeneration_{};
    bool enemyAttackPending_{};
    EnemyAttackPlan pendingEnemyAttack_{};
    std::int64_t enemyAttackReadyMicros_{};
    bool defensePunishOpening_{};
    std::int64_t defensePunishEndMicros_{};
    bool bossPracticePhaseLocked_{};
    EnemyPhase bossPracticePhase_{EnemyPhase::Normal};
};

} // namespace Astral::Scene
