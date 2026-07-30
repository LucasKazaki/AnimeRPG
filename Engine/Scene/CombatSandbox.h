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
};

struct AttackReport {
    AttackType type{AttackType::Light};
    AttackResult result{AttackResult::OutOfRange};
    int damageApplied{};
};

struct TrainingDummy {
    Math::Vec3 position{3.0f, 0.0f, 0.0f};
    int maximumHealth{100};
    int health{100};

    bool IsDefeated() const { return health == 0; }
};

class CombatSandbox {
public:
    CombatSandbox();

    void AdvanceTime(float deltaSeconds);
    AttackReport TryAttack(AttackType type, const Math::Vec3& attackerPosition);

    const TrainingDummy& Dummy() const { return dummy_; }
    const AttackReport& LastAttack() const { return lastAttack_; }
    float ElapsedSeconds() const { return elapsedSeconds_; }
    float CooldownRemaining() const;
    const AttackDefinition& Definition(AttackType type) const;

private:
    TrainingDummy dummy_{};
    AttackDefinition lightAttack_{25, 0.4f, 3.5f};
    AttackDefinition heavyAttack_{60, 1.0f, 3.5f};
    AttackReport lastAttack_{AttackType::Light, AttackResult::Ready, 0};
    float elapsedSeconds_{};
    float nextAttackTime_{};
};

} // namespace Astral::Scene
