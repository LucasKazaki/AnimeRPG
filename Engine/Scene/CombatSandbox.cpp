#include "Engine/Scene/CombatSandbox.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

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
            postureAtRecoveryStart_ = 0;
        }
        return;
    }

    timeSincePostureHit_ += static_cast<double>(deltaSeconds);
    if (dummy_.posture > 0) {
        // Frame deltas arrive as floats, so mathematically equal frame splits can
        // accumulate a few tens of nanoseconds apart. Quantize the total elapsed
        // posture timeline once, at microsecond precision, before applying the
        // integer recovery threshold. This preserves sub-frame precision while
        // making exact threshold crossings independent of common frame splits.
        constexpr std::int64_t MicrosPerSecond = 1000000;
        const std::int64_t elapsedMicros = static_cast<std::int64_t>(std::llround(
            timeSincePostureHit_ * static_cast<double>(MicrosPerSecond)));
        const std::int64_t delayMicros = static_cast<std::int64_t>(std::llround(
            static_cast<double>(PostureRecoveryDelaySeconds)
                * static_cast<double>(MicrosPerSecond)));
        const std::int64_t recoveryMicros = std::max<std::int64_t>(
            0, elapsedMicros - delayMicros);
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
        postureAtRecoveryStart_ = 0;
    }
    return applied;
}

void CombatSandbox::RegisterSuccessfulAttackHit() {
    RegisterComboHit();
}

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
    lastAttack_ = {AttackType::Light, AttackResult::Ready, 0, 0, false};
    stats_ = {};
    elapsedSeconds_ = 0.0f;
    nextAttackTime_ = 0.0f;
    staggerRemaining_ = 0.0f;
    timeSincePostureHit_ = 0.0;
    postureAtRecoveryStart_ = 0;
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
    timeSincePostureHit_ = 0.0;
    postureAtRecoveryStart_ = dummy_.posture;
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
