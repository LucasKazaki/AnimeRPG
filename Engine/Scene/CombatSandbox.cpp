#include "Engine/Scene/CombatSandbox.h"

#include <algorithm>
#include <cmath>

namespace Astral::Scene {

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
    lastAttack_ = {type, AttackResult::OutOfRange, 0, comboCount_, false};
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
    nextAttackMicros_ = CurrentMicros() + SecondsToMicros(attack.cooldownSeconds);
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
        staggerEndMicros_ = 0;
        postureAtRecoveryStart_ = 0;
        targetAffinity_ = ManaAffinity::None;
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
    dummy_ = TrainingDummy{};
    lastAttack_ = {AttackType::Light, AttackResult::Ready, 0, 0, false};
    stats_ = {};
    elapsedSecondsPrecise_ = 0.0;
    nextAttackMicros_ = 0;
    staggerEndMicros_ = 0;
    lastPostureHitMicros_ = 0;
    postureAtRecoveryStart_ = 0;
    lastComboHitMicros_ = -1000000000;
    comboCount_ = 0;
    targetAffinity_ = ManaAffinity::None;
}

ComboFinisherReport CombatSandbox::TryComboFinisher(const Math::Vec3& attackerPosition) {
    if (dummy_.IsDefeated()) {
        return {ComboFinisherResult::TargetDefeated, 0};
    }
    if (!ComboFinisherReady()) {
        return {ComboFinisherResult::NotReady, 0};
    }
    if (!std::isfinite(attackerPosition.x) || !std::isfinite(attackerPosition.y)) {
        return {ComboFinisherResult::OutOfRange, 0};
    }

    const float deltaX = dummy_.position.x - attackerPosition.x;
    const float deltaY = dummy_.position.y - attackerPosition.y;
    if (deltaX * deltaX + deltaY * deltaY > lightAttack_.range * lightAttack_.range) {
        return {ComboFinisherResult::OutOfRange, 0};
    }

    const int applied = ApplyDamage(ComboFinisherDamage);
    comboCount_ = 0;
    lastComboHitMicros_ = -1000000000;
    return {ComboFinisherResult::Activated, applied};
}

ManaReactionReport CombatSandbox::ApplyManaAffinity(ManaAffinity affinity) {
    ManaReactionReport report{affinity, targetAffinity_, targetAffinity_, ManaReaction::None, 0};
    if (affinity == ManaAffinity::None || dummy_.IsDefeated()) {
        return report;
    }

    if (targetAffinity_ == ManaAffinity::None || targetAffinity_ == affinity) {
        targetAffinity_ = affinity;
        report.remaining = targetAffinity_;
        return report;
    }

    targetAffinity_ = ManaAffinity::None;
    report.remaining = ManaAffinity::None;
    report.reaction = ManaReaction::Eclipse;
    report.bonusDamage = ApplyDamage(EclipseReactionDamage);
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
        return true;
    }
    return false;
}

void CombatSandbox::RegisterComboHit() {
    const std::int64_t now = CurrentMicros();
    if (comboCount_ > 0
        && now - lastComboHitMicros_ <= SecondsToMicros(ComboWindowSeconds)) {
        ++comboCount_;
    } else {
        comboCount_ = 1;
    }
    lastComboHitMicros_ = now;
    stats_.bestCombo = std::max(stats_.bestCombo, comboCount_);
}

} // namespace Astral::Scene
