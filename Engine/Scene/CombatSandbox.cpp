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
    }

    if (staggerRemaining_ > 0.0f) {
        staggerRemaining_ = std::max(0.0f, staggerRemaining_ - deltaSeconds);
        if (staggerRemaining_ == 0.0f) {
            dummy_.posture = 0;
            postureRecoveryRemainder_ = 0.0f;
        }
        return;
    }

    const float previousIdle = timeSincePostureHit_;
    timeSincePostureHit_ += deltaSeconds;
    const float previousRecovery = std::max(0.0f,
        previousIdle - PostureRecoveryDelaySeconds);
    const float currentRecovery = std::max(0.0f,
        timeSincePostureHit_ - PostureRecoveryDelaySeconds);
    const float recoveryDuration = currentRecovery - previousRecovery;
    if (recoveryDuration > 0.0f && dummy_.posture > 0) {
        postureRecoveryRemainder_ += PostureRecoveryPerSecond * recoveryDuration;
        const int recovered = static_cast<int>(std::floor(postureRecoveryRemainder_));
        if (recovered > 0) {
            dummy_.posture = std::max(0, dummy_.posture - recovered);
            postureRecoveryRemainder_ -= static_cast<float>(recovered);
            if (dummy_.posture == 0) postureRecoveryRemainder_ = 0.0f;
        }
    }
}

AttackReport CombatSandbox::TryAttack(AttackType type, const Math::Vec3& attackerPosition) {
    lastAttack_ = {type, AttackResult::OutOfRange, 0, comboCount_, false};
    if (dummy_.IsDefeated()) {
        lastAttack_.result = AttackResult::TargetDefeated;
        return lastAttack_;
    }
    if (elapsedSeconds_ < nextAttackTime_) {
        lastAttack_.result = AttackResult::Cooldown;
        return lastAttack_;
    }

    const AttackDefinition& attack = Definition(type);
    const float deltaX = dummy_.position.x - attackerPosition.x;
    const float deltaY = dummy_.position.y - attackerPosition.y;
    if (deltaX * deltaX + deltaY * deltaY > attack.range * attack.range) {
        return lastAttack_;
    }

    lastAttack_.result = AttackResult::Hit;
    lastAttack_.damageApplied = ApplyDamage(attack.damage);
    RegisterComboHit();
    lastAttack_.comboCount = comboCount_;
    if (!dummy_.IsDefeated()) {
        lastAttack_.staggerTriggered = ApplyPostureDamage(attack.postureDamage);
    }
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
        postureRecoveryRemainder_ = 0.0f;
    }
    return applied;
}

bool CombatSandbox::ConsumeStaggerOpening() {
    if (!IsStaggered()) return false;

    staggerRemaining_ = 0.0f;
    dummy_.posture = 0;
    timeSincePostureHit_ = 0.0f;
    postureRecoveryRemainder_ = 0.0f;
    return true;
}

void CombatSandbox::ResetTrainingSession() {
    dummy_ = TrainingDummy{};
    lastAttack_ = {AttackType::Light, AttackResult::Ready, 0, 0, false};
    stats_ = {};
    elapsedSeconds_ = 0.0f;
    nextAttackTime_ = 0.0f;
    staggerRemaining_ = 0.0f;
    timeSincePostureHit_ = 0.0f;
    postureRecoveryRemainder_ = 0.0f;
    lastComboHitTime_ = -1000.0f;
    comboCount_ = 0;
}

float CombatSandbox::CooldownRemaining() const {
    return std::max(0.0f, nextAttackTime_ - elapsedSeconds_);
}

float CombatSandbox::TrainingDps() const {
    return elapsedSeconds_ > 0.0f
        ? static_cast<float>(stats_.totalDamage) / elapsedSeconds_
        : 0.0f;
}

const AttackDefinition& CombatSandbox::Definition(AttackType type) const {
    return type == AttackType::Light ? lightAttack_ : heavyAttack_;
}

bool CombatSandbox::ApplyPostureDamage(int postureDamage) {
    if (postureDamage <= 0 || dummy_.IsDefeated() || IsStaggered()) return false;

    dummy_.posture = std::min(dummy_.maximumPosture, dummy_.posture + postureDamage);
    timeSincePostureHit_ = 0.0f;
    postureRecoveryRemainder_ = 0.0f;
    if (dummy_.posture >= dummy_.maximumPosture) {
        staggerRemaining_ = StaggerDurationSeconds;
        return true;
    }
    return false;
}

void CombatSandbox::RegisterComboHit() {
    if (comboCount_ > 0 && elapsedSeconds_ - lastComboHitTime_ <= ComboWindowSeconds) {
        ++comboCount_;
    } else {
        comboCount_ = 1;
    }
    lastComboHitTime_ = elapsedSeconds_;
    stats_.bestCombo = std::max(stats_.bestCombo, comboCount_);
}

} // namespace Astral::Scene
