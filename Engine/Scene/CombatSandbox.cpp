#include "Engine/Scene/CombatSandbox.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Astral::Scene {
namespace {
std::int64_t SaturatingAdd(std::int64_t left, std::int64_t right) {
    if (right <= 0) return left;
    const std::int64_t maximum = std::numeric_limits<std::int64_t>::max();
    return left > maximum - right ? maximum : left + right;
}

void SaturatingIncrement(int& value) {
    if (value < std::numeric_limits<int>::max()) ++value;
}
}

CombatSandbox::CombatSandbox() = default;

std::int64_t CombatSandbox::SecondsToMicros(float seconds) {
    return static_cast<std::int64_t>(std::llround(
        static_cast<double>(seconds) * static_cast<double>(MicrosPerSecond)));
}

std::int64_t CombatSandbox::CurrentMicros() const {
    return static_cast<std::int64_t>(std::llround(
        elapsedSecondsPrecise_ * static_cast<double>(MicrosPerSecond)));
}

void CombatSandbox::AdvanceTime(float deltaSeconds) {
    if (deltaSeconds <= 0.0f || !std::isfinite(deltaSeconds)) return;

    elapsedSecondsPrecise_ += static_cast<double>(deltaSeconds);
    const std::int64_t now = CurrentMicros();

    if (comboCount_ > 0
        && now - lastComboHitMicros_ > SecondsToMicros(ComboWindowSeconds)) {
        comboCount_ = 0;
    }
    if (techniqueChain_ > 0
        && now - lastTechniqueMicros_ > SecondsToMicros(TechniqueChainWindowSeconds)) {
        techniqueChain_ = 0;
        lastTechniqueType_ = TechniqueType::None;
    }

    if (staggerEndMicros_ > 0) {
        if (now >= staggerEndMicros_) {
            staggerEndMicros_ = 0;
            dummy_.posture = 0;
            postureAtRecoveryStart_ = 0;
        }
        return;
    }

    if (dummy_.posture > 0) {
        const std::int64_t recoveryMicros = std::max<std::int64_t>(0,
            now - lastPostureHitMicros_ - SecondsToMicros(PostureRecoveryDelaySeconds));
        const std::int64_t recovered = static_cast<std::int64_t>(std::floor(
            static_cast<double>(PostureRecoveryPerSecond)
                * static_cast<double>(recoveryMicros)
                / static_cast<double>(MicrosPerSecond)));
        dummy_.posture = std::max(0,
            postureAtRecoveryStart_ - static_cast<int>(recovered));
        if (dummy_.posture == 0) postureAtRecoveryStart_ = 0;
    }
}

AttackReport CombatSandbox::TryAttack(AttackType type, const Math::Vec3& attackerPosition) {
    lastAttack_ = {type, AttackResult::OutOfRange, 0, comboCount_, false, false, false};
    if (dummy_.IsDefeated()) {
        lastAttack_.result = AttackResult::TargetDefeated;
        return lastAttack_;
    }
    if (CurrentMicros() < nextAttackMicros_) {
        lastAttack_.result = AttackResult::Cooldown;
        return lastAttack_;
    }

    const AttackDefinition& attack = Definition(type);
    const float deltaX = dummy_.position.x - attackerPosition.x;
    const float deltaY = dummy_.position.y - attackerPosition.y;
    if (!std::isfinite(deltaX) || !std::isfinite(deltaY)
        || deltaX * deltaX + deltaY * deltaY > attack.range * attack.range) {
        return lastAttack_;
    }

    lastAttack_.result = AttackResult::Hit;
    const int requestedDamage = AdjustDirectAttackDamage(type, attack.damage,
        lastAttack_.resistanceApplied, lastAttack_.staggerBonusApplied);
    lastAttack_.damageApplied = ApplyDamage(requestedDamage);
    RegisterComboHit();
    lastAttack_.comboCount = comboCount_;
    if (!dummy_.IsDefeated()) {
        lastAttack_.staggerTriggered = ApplyPostureDamage(attack.postureDamage);
    }
    nextAttackMicros_ = CurrentMicros() + SecondsToMicros(attack.cooldownSeconds);
    return lastAttack_;
}

int CombatSandbox::ApplyDamage(int damage) {
    if (damage <= 0 || dummy_.IsDefeated()) return 0;

    if (targetMode_ == TrainingTargetMode::Endless) {
        stats_.totalDamage = SaturatingAdd(stats_.totalDamage, damage);
        SaturatingIncrement(stats_.hitCount);
        stats_.peakHit = std::max(stats_.peakHit, damage);
        return damage;
    }

    const int applied = std::min(damage, dummy_.health);
    dummy_.health -= applied;
    if (applied > 0) {
        stats_.totalDamage = SaturatingAdd(stats_.totalDamage, applied);
        SaturatingIncrement(stats_.hitCount);
        stats_.peakHit = std::max(stats_.peakHit, applied);
    }
    if (dummy_.IsDefeated()) {
        if (targetDefeatElapsedSeconds_ < 0.0) {
            targetDefeatElapsedSeconds_ = elapsedSecondsPrecise_;
        }
        dummy_.posture = 0;
        staggerEndMicros_ = 0;
        postureAtRecoveryStart_ = 0;
        targetAffinity_ = ManaAffinity::None;
        eclipseOpening_ = false;
    }
    return applied;
}

void CombatSandbox::RegisterSuccessfulAttackHit() {
    RegisterComboHit();
}

bool CombatSandbox::ConsumeStaggerOpening() {
    if (!IsStaggered()) return false;

    staggerEndMicros_ = 0;
    dummy_.posture = 0;
    lastPostureHitMicros_ = CurrentMicros();
    postureAtRecoveryStart_ = 0;
    return true;
}

void CombatSandbox::ResetTrainingSession() {
    const TrainingEnemyDefinition enemy = CurrentEnemyDefinition();
    dummy_ = TrainingDummy{};
    dummy_.maximumHealth = enemy.maximumHealth;
    dummy_.health = enemy.maximumHealth;
    dummy_.maximumPosture = enemy.maximumPosture;
    lastAttack_ = {AttackType::Light, AttackResult::Ready, 0, 0, false, false, false};
    stats_ = {};
    elapsedSecondsPrecise_ = 0.0;
    targetDefeatElapsedSeconds_ = -1.0;
    nextAttackMicros_ = 0;
    staggerEndMicros_ = 0;
    lastPostureHitMicros_ = 0;
    postureAtRecoveryStart_ = 0;
    lastComboHitMicros_ = -1000000000;
    comboCount_ = 0;
    targetAffinity_ = ManaAffinity::None;
    eclipseOpening_ = false;
    techniqueChain_ = 0;
    lastTechniqueMicros_ = -1000000000;
    lastTechniqueType_ = TechniqueType::None;
}

bool CombatSandbox::SetTrainingEnemyProfile(TrainingEnemyProfile profile) {
    if (profile == enemyProfile_) return false;
    enemyProfile_ = profile;
    ResetTrainingSession();
    return true;
}

bool CombatSandbox::SetTrainingTargetMode(TrainingTargetMode mode) {
    if (mode == targetMode_) return false;
    targetMode_ = mode;
    ResetTrainingSession();
    return true;
}

ComboFinisherReport CombatSandbox::TryComboFinisher(const Math::Vec3& attackerPosition) {
    if (dummy_.IsDefeated()) {
        return {ComboFinisherResult::TargetDefeated, 0, false};
    }
    if (!ComboFinisherReady()) {
        return {ComboFinisherResult::NotReady, 0, false};
    }
    if (!std::isfinite(attackerPosition.x) || !std::isfinite(attackerPosition.y)) {
        return {ComboFinisherResult::OutOfRange, 0, false};
    }

    const float deltaX = dummy_.position.x - attackerPosition.x;
    const float deltaY = dummy_.position.y - attackerPosition.y;
    if (deltaX * deltaX + deltaY * deltaY > lightAttack_.range * lightAttack_.range) {
        return {ComboFinisherResult::OutOfRange, 0, false};
    }

    const bool eclipseFollowUp = eclipseOpening_;
    const int requestedDamage = ComboFinisherDamage
        + (eclipseFollowUp ? EclipseFinisherBonusDamage : 0);
    const int applied = ApplyDamage(requestedDamage);
    if (applied > 0) {
        SaturatingIncrement(stats_.finisherCount);
        RegisterTechnique(TechniqueType::Finisher, FinisherTechniquePoints);
    }
    comboCount_ = 0;
    lastComboHitMicros_ = -1000000000;
    eclipseOpening_ = false;
    return {ComboFinisherResult::Activated, applied, eclipseFollowUp};
}

ManaReactionReport CombatSandbox::ApplyManaAffinity(ManaAffinity affinity) {
    ManaReactionReport report{
        affinity, targetAffinity_, targetAffinity_, ManaReaction::None, 0, false};
    if (affinity == ManaAffinity::None || dummy_.IsDefeated()) {
        return report;
    }

    if (targetAffinity_ == ManaAffinity::None || targetAffinity_ == affinity) {
        targetAffinity_ = affinity;
        report.remaining = targetAffinity_;
        return report;
    }

    const TrainingEnemyDefinition enemy = CurrentEnemyDefinition();
    report.weaknessExploited = enemy.weakness != ManaAffinity::None
        && affinity == enemy.weakness;
    const int requestedDamage = EclipseReactionDamage
        + (report.weaknessExploited ? WeaknessReactionBonusDamage : 0);
    targetAffinity_ = ManaAffinity::None;
    report.remaining = ManaAffinity::None;
    report.reaction = ManaReaction::Eclipse;
    report.bonusDamage = ApplyDamage(requestedDamage);
    if (report.bonusDamage > 0) {
        SaturatingIncrement(stats_.reactionCount);
        RegisterTechnique(TechniqueType::Reaction, ReactionTechniquePoints);
        if (!dummy_.IsDefeated()) eclipseOpening_ = true;
    }
    return report;
}

float CombatSandbox::ElapsedSeconds() const {
    return static_cast<float>(elapsedSecondsPrecise_);
}

float CombatSandbox::CooldownRemaining() const {
    const std::int64_t remaining = std::max<std::int64_t>(0,
        nextAttackMicros_ - CurrentMicros());
    return static_cast<float>(remaining) / static_cast<float>(MicrosPerSecond);
}

float CombatSandbox::StaggerRemaining() const {
    const std::int64_t remaining = std::max<std::int64_t>(0,
        staggerEndMicros_ - CurrentMicros());
    return static_cast<float>(remaining) / static_cast<float>(MicrosPerSecond);
}

bool CombatSandbox::IsStaggered() const {
    return !dummy_.IsDefeated() && staggerEndMicros_ > CurrentMicros();
}

bool CombatSandbox::ComboFinisherReady() const {
    return !dummy_.IsDefeated() && comboCount_ >= ComboFinisherRequiredHits();
}

int CombatSandbox::ComboFinisherRequiredHits() const {
    return assistPreset_ == CombatAssistPreset::Accessible
        ? AccessibleComboFinisherHits
        : StandardComboFinisherHits;
}

EnemyPhase CombatSandbox::CurrentEnemyPhase() const {
    if (enemyProfile_ != TrainingEnemyProfile::Boss
        || dummy_.maximumHealth != 320 || dummy_.health <= 0) {
        return EnemyPhase::Normal;
    }
    return dummy_.health * 2 <= dummy_.maximumHealth
        ? EnemyPhase::Pressure
        : EnemyPhase::Normal;
}

TrainingEnemyDefinition CombatSandbox::CurrentEnemyDefinition() const {
    switch (enemyProfile_) {
    case TrainingEnemyProfile::Vanguard:
        return {90, 60, ManaAffinity::Solar, AttackResistance::Heavy};
    case TrainingEnemyProfile::Bulwark:
        return {180, 120, ManaAffinity::Umbral, AttackResistance::None};
    case TrainingEnemyProfile::Boss:
        return {320, 160,
            CurrentEnemyPhase() == EnemyPhase::Pressure
                ? ManaAffinity::Umbral
                : ManaAffinity::Solar,
            AttackResistance::Light};
    case TrainingEnemyProfile::Standard:
    default:
        return {100, 80, ManaAffinity::None, AttackResistance::None};
    }
}

std::int64_t CombatSandbox::TrainingChallengeScore() const {
    const std::int64_t baseScore = SaturatingAdd(stats_.totalDamage, stats_.techniqueScore);
    if (baseScore <= 0) return 0;

    const double effectiveSeconds = targetDefeatElapsedSeconds_ >= 0.0
        ? targetDefeatElapsedSeconds_
        : elapsedSecondsPrecise_;
    if (effectiveSeconds <= FastChallengeSeconds) {
        return SaturatingAdd(baseScore, baseScore / 4);
    }
    if (effectiveSeconds <= StandardChallengeSeconds) {
        return baseScore;
    }
    const std::int64_t quarter = baseScore / 4;
    const std::int64_t remainder = baseScore % 4;
    return quarter * 3 + remainder * 3 / 4;
}

float CombatSandbox::TrainingDps() const {
    return elapsedSecondsPrecise_ > 0.0
        ? static_cast<float>(static_cast<double>(stats_.totalDamage) / elapsedSecondsPrecise_)
        : 0.0f;
}

const AttackDefinition& CombatSandbox::Definition(AttackType type) const {
    return type == AttackType::Light ? lightAttack_ : heavyAttack_;
}

bool CombatSandbox::ApplyPostureDamage(int postureDamage) {
    if (postureDamage <= 0 || dummy_.IsDefeated() || IsStaggered()) return false;

    dummy_.posture = std::min(dummy_.maximumPosture, dummy_.posture + postureDamage);
    lastPostureHitMicros_ = CurrentMicros();
    postureAtRecoveryStart_ = dummy_.posture;
    if (dummy_.posture >= dummy_.maximumPosture) {
        staggerEndMicros_ = CurrentMicros() + SecondsToMicros(StaggerDurationSeconds);
        SaturatingIncrement(stats_.staggerCount);
        RegisterTechnique(TechniqueType::Stagger, StaggerTechniquePoints);
        return true;
    }
    return false;
}

void CombatSandbox::RegisterComboHit() {
    const std::int64_t now = CurrentMicros();
    if (comboCount_ > 0
        && now - lastComboHitMicros_ <= SecondsToMicros(ComboWindowSeconds)) {
        SaturatingIncrement(comboCount_);
    } else {
        comboCount_ = 1;
    }
    lastComboHitMicros_ = now;
    stats_.bestCombo = std::max(stats_.bestCombo, comboCount_);
}

void CombatSandbox::RegisterTechnique(TechniqueType type, int basePoints) {
    if (type == TechniqueType::None || basePoints <= 0) return;

    const std::int64_t now = CurrentMicros();
    const bool withinWindow = techniqueChain_ > 0
        && now - lastTechniqueMicros_ <= SecondsToMicros(TechniqueChainWindowSeconds);
    if (withinWindow && type != lastTechniqueType_) {
        techniqueChain_ = std::min(MaximumTechniqueChain, techniqueChain_ + 1);
    } else {
        techniqueChain_ = 1;
    }
    lastTechniqueMicros_ = now;
    lastTechniqueType_ = type;
    stats_.bestTechniqueChain = std::max(stats_.bestTechniqueChain, techniqueChain_);
    stats_.techniqueScore = SaturatingAdd(stats_.techniqueScore,
        static_cast<std::int64_t>(basePoints) * techniqueChain_);
}

int CombatSandbox::AdjustDirectAttackDamage(AttackType type, int damage,
    bool& resistanceApplied, bool& staggerBonusApplied) const {
    int adjusted = std::max(0, damage);
    const AttackResistance resistance = CurrentEnemyDefinition().resistance;
    resistanceApplied = (type == AttackType::Light && resistance == AttackResistance::Light)
        || (type == AttackType::Heavy && resistance == AttackResistance::Heavy);
    if (resistanceApplied) {
        adjusted = adjusted * ResistantAttackDamageNumerator
            / ResistantAttackDamageDenominator;
    }

    staggerBonusApplied = IsStaggered();
    if (staggerBonusApplied) {
        adjusted = adjusted * StaggerDamageNumerator / StaggerDamageDenominator;
    }
    return adjusted;
}

} // namespace Astral::Scene