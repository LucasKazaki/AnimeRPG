#pragma once

#include "Engine/Math/Math.h"
#include "Engine/Scene/CombatSandbox.h"

namespace Astral::Scene {

enum class ShadowActionType {
    None,
    Dash,
    FatalStrike,
    Guard,
};

enum class ShadowActionResult {
    Ready,
    Activated,
    Guarding,
    GuardedConflict,
    Cooldown,
    InsufficientResource,
    OutOfRange,
    TargetDefeated,
};

struct ShadowActionReport {
    ShadowActionType type{ShadowActionType::None};
    ShadowActionResult result{ShadowActionResult::Ready};
    int damageApplied{};
    Math::Vec3 dashDestination{};
};

class ShadowbladeActions {
public:
    static constexpr float MaximumResource = 100.0f;
    static constexpr float ResourceRegenerationPerSecond = 15.0f;
    static constexpr float DashCost = 25.0f;
    static constexpr float DashCooldownSeconds = 1.0f;
    static constexpr float DashDistance = 6.0f;
    static constexpr float FatalStrikeCost = 50.0f;
    static constexpr float FatalStrikeCooldownSeconds = 2.0f;
    static constexpr float FatalStrikeRange = 3.5f;
    static constexpr int FatalStrikeDamage = 80;

    void AdvanceTime(float deltaSeconds);
    void SetGuarding(bool guarding);
    ShadowActionReport TryDash(const Math::Vec3& position);
    ShadowActionReport TryFatalStrike(const Math::Vec3& position, CombatSandbox& combatSandbox);

    float Resource() const { return resource_; }
    float DashCooldownRemaining() const { return dashCooldownRemaining_; }
    float FatalStrikeCooldownRemaining() const { return fatalStrikeCooldownRemaining_; }
    bool IsGuarding() const { return guarding_; }
    const ShadowActionReport& LastAction() const { return lastAction_; }

private:
    float resource_{MaximumResource};
    float dashCooldownRemaining_{};
    float fatalStrikeCooldownRemaining_{};
    bool guarding_{};
    ShadowActionReport lastAction_{};
};

} // namespace Astral::Scene
