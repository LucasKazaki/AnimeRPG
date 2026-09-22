#include "Engine/Scene/CombatSandbox.h"

#include <algorithm>
#include <cmath>

namespace Astral::Scene {

CombatSandbox::CombatSandbox() = default;

void CombatSandbox::AdvanceTime(float deltaSeconds) {
    if (deltaSeconds <= 0.0f || !std::isfinite(deltaSeconds)) return;
    elapsedSeconds_ += deltaSeconds;
    if (comboCount_ > 0 && elapsedSeconds_ - lastComboHitTime_ > ComboWindowSeconds) {
        comboCount_ = 0;
        comboFinisherReady_ = false;
    }
    counterWindowRemaining_ = std::max(0.0,
        counterWindowRemaining_ - static_cast<double>(deltaSeconds));
    if (enemyAttackActive_) {
        enemyAttackRemaining_ -= static_cast<double>(deltaSeconds);
        if (enemyAttackRemaining_ <= 0.0) ResolveEnemyAttackHit();
    }
    if (staggerRemaining_ > 0.0f) {
        staggerRemaining_ = std::max(0.0f, staggerRemaining_ - deltaSeconds);
        if (staggerRemaining_ == 0.0f) {
            dummy_.posture = 0;
            postureAtRecoveryStart_ = 0;
        }
        return;
    }
    timeSincePostureHit_ += static_cast<double>(deltaSeconds);
    const double recoveryDuration = std::max(0.0,
        timeSincePostureHit_ - static_cast<double>(PostureRecoveryDelaySeconds));
    if (recoveryDuration > 0.0 && dummy_.posture > 0) {
        const double recoveryAmount = static_cast<double>(PostureRecoveryPerSecond)
            * recoveryDuration;
        constexpr double RecoveryBoundaryEpsilon = 1e-6;
        const int recovered = static_cast<int>(std::floor(recoveryAmount + RecoveryBoundaryEpsilon));
        dummy_.posture = std::max(0, postureAtRecoveryStart_ - recovered);
        if (dummy_.posture == 0) postureAtRecoveryStart_ = 0;
    }
}

AttackReport CombatSandbox::TryAttack(AttackType type, const Math::Vec3& attackerPosition) {
    lastAttack_ = {type, AttackResult::OutOfRange, 0, comboCount_, false};
    if (dummy_.IsDefeated()) { lastAttack_.result = AttackResult::TargetDefeated; return lastAttack_; }
    if (elapsedSeconds_ < nextAttackTime_) { lastAttack_.result = AttackResult::Cooldown; return lastAttack_; }
    const AttackDefinition& attack = Definition(type);
    const float deltaX = dummy_.position.x - attackerPosition.x;
    const float deltaY = dummy_.position.y - attackerPosition.y;
    if (deltaX * deltaX + deltaY * deltaY > attack.range * attack.range) return lastAttack_;
    lastAttack_.result = AttackResult::Hit;
    lastAttack_.damageApplied = ApplyDamage(attack.damage);
    RegisterComboHit();
    lastAttack_.comboCount = comboCount_;
    if (!dummy_.IsDefeated()) lastAttack_.staggerTriggered = ApplyPostureDamage(attack.postureDamage);
    nextAttackTime_ = elapsedSeconds_ + attack.cooldownSeconds;
    return lastAttack_;
}

int CombatSandbox::ApplyDamage(int damage) {
    if (damage <= 0 || dummy_.IsDefeated()) return 0;
    const int applied = std::min(damage, dummy_.health);
    dummy_.health -= applied;
    if (applied > 0) {
        stats_.totalDamage += applied;
        ++stats_.hitCount;
        stats_.peakHit = std::max(stats_.peakHit, applied);
    }
    if (dummy_.IsDefeated()) {
        dummy_.posture = 0;
        staggerRemaining_ = 0.0f;
        postureAtRecoveryStart_ = 0;
        comboFinisherReady_ = false;
    }
    return applied;
}

void CombatSandbox::RegisterSuccessfulAttackHit() { RegisterComboHit(); }

bool CombatSandbox::ConsumeStaggerOpening() {
    if (!IsStaggered()) return false;
    staggerRemaining_ = 0.0f;
    dummy_.posture = 0;
    timeSincePostureHit_ = 0.0;
    postureAtRecoveryStart_ = 0;
    return true;
}

void CombatSandbox::ResetTrainingSession() {
    dummy_ = TrainingDummy{};
    player_ = PlayerCombatState{};
    lastAttack_ = {AttackType::Light, AttackResult::Ready, 0, 0, false};
    lastEnemyAttack_ = {};
    stats_ = {};
    elapsedSeconds_ = 0.0f;
    nextAttackTime_ = 0.0f;
    staggerRemaining_ = 0.0f;
    timeSincePostureHit_ = 0.0;
    postureAtRecoveryStart_ = 0;
    lastComboHitTime_ = -1000.0f;
    comboCount_ = 0;
    comboFinisherReady_ = false;
    enemyAttackActive_ = false;
    enemyAttackRemaining_ = 0.0;
    enemyAttackDamage_ = 0;
    counterWindowRemaining_ = 0.0;
    targetAffinity_ = ManaAffinity::None;
}

bool CombatSandbox::ScheduleEnemyAttack(float windupSeconds, int damage) {
    if (enemyAttackActive_ || player_.IsDefeated() || damage <= 0
        || windupSeconds <= 0.0f || !std::isfinite(windupSeconds)) return false;
    enemyAttackActive_ = true;
    enemyAttackRemaining_ = static_cast<double>(windupSeconds);
    enemyAttackDamage_ = damage;
    lastEnemyAttack_ = {EnemyAttackResult::Scheduled, 0, false, false};
    return true;
}

EnemyAttackReport CombatSandbox::TryPerfectDodge() {
    if (!enemyAttackActive_) { lastEnemyAttack_ = {EnemyAttackResult::NoAttack, 0, false, false}; return lastEnemyAttack_; }
    if (enemyAttackRemaining_ > static_cast<double>(PerfectDodgeWindowSeconds)) {
        lastEnemyAttack_ = {EnemyAttackResult::TooEarly, 0, false, false}; return lastEnemyAttack_;
    }
    enemyAttackActive_ = false; enemyAttackRemaining_ = 0.0; enemyAttackDamage_ = 0;
    counterWindowRemaining_ = static_cast<double>(CounterWindowSeconds);
    lastEnemyAttack_ = {EnemyAttackResult::PerfectDodged, 0, true, false};
    return lastEnemyAttack_;
}

EnemyAttackReport CombatSandbox::TryPerfectGuard() {
    if (!enemyAttackActive_) { lastEnemyAttack_ = {EnemyAttackResult::NoAttack, 0, false, false}; return lastEnemyAttack_; }
    if (enemyAttackRemaining_ > static_cast<double>(PerfectGuardWindowSeconds)) {
        lastEnemyAttack_ = {EnemyAttackResult::TooEarly, 0, false, false}; return lastEnemyAttack_;
    }
    enemyAttackActive_ = false; enemyAttackRemaining_ = 0.0; enemyAttackDamage_ = 0;
    const bool staggerTriggered = ApplyPostureDamage(PerfectGuardPostureDamage);
    lastEnemyAttack_ = {EnemyAttackResult::PerfectGuarded, 0, false, staggerTriggered};
    return lastEnemyAttack_;
}

CounterAttackReport CombatSandbox::TryCounterAttack(const Math::Vec3& attackerPosition) {
    if (!CounterReady()) return {CounterAttackResult::NotReady, 0};
    if (dummy_.IsDefeated()) return {CounterAttackResult::TargetDefeated, 0};
    const float dx = dummy_.position.x - attackerPosition.x;
    const float dy = dummy_.position.y - attackerPosition.y;
    if (dx * dx + dy * dy > CounterRange * CounterRange) return {CounterAttackResult::OutOfRange, 0};
    counterWindowRemaining_ = 0.0;
    return {CounterAttackResult::Activated, ApplyDamage(CounterDamage)};
}

ComboFinisherReport CombatSandbox::TryComboFinisher(const Math::Vec3& attackerPosition) {
    if (!comboFinisherReady_) return {ComboFinisherResult::NotReady, 0};
    if (dummy_.IsDefeated()) return {ComboFinisherResult::TargetDefeated, 0};
    const float dx = dummy_.position.x - attackerPosition.x;
    const float dy = dummy_.position.y - attackerPosition.y;
    if (dx * dx + dy * dy > ComboFinisherRange * ComboFinisherRange) return {ComboFinisherResult::OutOfRange, 0};
    comboFinisherReady_ = false;
    comboCount_ = 0;
    return {ComboFinisherResult::Activated, ApplyDamage(ComboFinisherDamage)};
}

ManaReactionReport CombatSandbox::ApplyManaAffinity(ManaAffinity affinity) {
    ManaReactionReport report{affinity, targetAffinity_, targetAffinity_, ManaReaction::None, 0};
    if (affinity == ManaAffinity::None || dummy_.IsDefeated()) return report;
    if (targetAffinity_ == ManaAffinity::None || targetAffinity_ == affinity) {
        targetAffinity_ = affinity; report.remaining = targetAffinity_; return report;
    }
    targetAffinity_ = ManaAffinity::None;
    report.remaining = ManaAffinity::None;
    report.reaction = ManaReaction::Eclipse;
    report.bonusDamage = ApplyDamage(ManaReactionDamage);
    return report;
}

float CombatSandbox::CooldownRemaining() const { return std::max(0.0f, nextAttackTime_ - elapsedSeconds_); }
float CombatSandbox::TrainingDps() const { return elapsedSeconds_ > 0.0f ? static_cast<float>(stats_.totalDamage) / elapsedSeconds_ : 0.0f; }
const AttackDefinition& CombatSandbox::Definition(AttackType type) const { return type == AttackType::Light ? lightAttack_ : heavyAttack_; }

bool CombatSandbox::ApplyPostureDamage(int postureDamage) {
    if (postureDamage <= 0 || dummy_.IsDefeated() || IsStaggered()) return false;
    dummy_.posture = std::min(dummy_.maximumPosture, dummy_.posture + postureDamage);
    timeSincePostureHit_ = 0.0;
    postureAtRecoveryStart_ = dummy_.posture;
    if (dummy_.posture >= dummy_.maximumPosture) { staggerRemaining_ = StaggerDurationSeconds; return true; }
    return false;
}

int CombatSandbox::ApplyPlayerDamage(int damage) {
    if (damage <= 0 || player_.IsDefeated()) return 0;
    const int applied = std::min(damage, player_.health);
    player_.health -= applied;
    return applied;
}

void CombatSandbox::RegisterComboHit() {
    if (comboCount_ > 0 && elapsedSeconds_ - lastComboHitTime_ <= ComboWindowSeconds) ++comboCount_;
    else comboCount_ = 1;
    lastComboHitTime_ = elapsedSeconds_;
    comboFinisherReady_ = comboCount_ >= ComboFinisherThreshold;
    stats_.bestCombo = std::max(stats_.bestCombo, comboCount_);
}

void CombatSandbox::ResolveEnemyAttackHit() {
    if (!enemyAttackActive_) return;
    enemyAttackActive_ = false;
    enemyAttackRemaining_ = 0.0;
    const int damage = enemyAttackDamage_;
    enemyAttackDamage_ = 0;
    lastEnemyAttack_ = {EnemyAttackResult::Hit, ApplyPlayerDamage(damage), false, false};
}

} // namespace Astral::Scene
