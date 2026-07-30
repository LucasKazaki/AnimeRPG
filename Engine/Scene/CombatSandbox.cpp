#include "Engine/Scene/CombatSandbox.h"

#include <algorithm>

namespace Astral::Scene {

CombatSandbox::CombatSandbox() = default;

void CombatSandbox::AdvanceTime(float deltaSeconds) {
    if (deltaSeconds > 0.0f) {
        elapsedSeconds_ += deltaSeconds;
    }
}

AttackReport CombatSandbox::TryAttack(AttackType type, const Math::Vec3& attackerPosition) {
    lastAttack_ = {type, AttackResult::OutOfRange, 0};
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
    nextAttackTime_ = elapsedSeconds_ + attack.cooldownSeconds;
    return lastAttack_;
}

int CombatSandbox::ApplyDamage(int damage) {
    if (damage <= 0 || dummy_.IsDefeated()) return 0;
    const int applied = std::min(damage, dummy_.health);
    dummy_.health -= applied;
    return applied;
}

float CombatSandbox::CooldownRemaining() const {
    return std::max(0.0f, nextAttackTime_ - elapsedSeconds_);
}

const AttackDefinition& CombatSandbox::Definition(AttackType type) const {
    return type == AttackType::Light ? lightAttack_ : heavyAttack_;
}

} // namespace Astral::Scene
