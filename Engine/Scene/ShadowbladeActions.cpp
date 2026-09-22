#include "Engine/Scene/ShadowbladeActions.h"

#include <algorithm>
#include <cmath>

namespace Astral::Scene {

void ShadowbladeActions::AdvanceTime(float deltaSeconds) {
    if (deltaSeconds <= 0.0f || !std::isfinite(deltaSeconds)) return;

    resource_ = std::min(MaximumResource,
        resource_ + ResourceRegenerationPerSecond * deltaSeconds);
    dashCooldownRemaining_ = std::max(0.0f, dashCooldownRemaining_ - deltaSeconds);
    fatalStrikeCooldownRemaining_ = std::max(0.0f,
        fatalStrikeCooldownRemaining_ - deltaSeconds);
}

float ShadowbladeActions::RestoreResource(float amount) {
    if (amount <= 0.0f || !std::isfinite(amount)) return 0.0f;
    const float previous = resource_;
    resource_ = std::min(MaximumResource, resource_ + amount);
    return resource_ - previous;
}

void ShadowbladeActions::SetGuarding(bool guarding) {
    if (guarding_ == guarding) return;
    guarding_ = guarding;
    lastAction_ = {ShadowActionType::Guard,
        guarding ? ShadowActionResult::Guarding : ShadowActionResult::Ready, 0, {}};
}

ShadowActionReport ShadowbladeActions::TryDash(const Math::Vec3& position,
    const Math::Vec3& direction) {
    lastAction_ = {ShadowActionType::Dash, ShadowActionResult::Ready, 0, position};
    if (guarding_) {
        lastAction_.result = ShadowActionResult::GuardedConflict;
        return lastAction_;
    }
    if (dashCooldownRemaining_ > 0.0f) {
        lastAction_.result = ShadowActionResult::Cooldown;
        return lastAction_;
    }
    if (resource_ < DashCost) {
        lastAction_.result = ShadowActionResult::InsufficientResource;
        return lastAction_;
    }

    float directionX = direction.x;
    float directionY = direction.y;
    const float lengthSquared = directionX * directionX + directionY * directionY;
    if (!std::isfinite(directionX) || !std::isfinite(directionY)
        || !std::isfinite(lengthSquared) || lengthSquared <= 0.000001f) {
        directionX = 0.0f;
        directionY = 1.0f;
    } else {
        const float inverseLength = 1.0f / std::sqrt(lengthSquared);
        directionX *= inverseLength;
        directionY *= inverseLength;
    }

    resource_ -= DashCost;
    dashCooldownRemaining_ = DashCooldownSeconds;
    lastAction_.result = ShadowActionResult::Activated;
    lastAction_.resourceSpent = DashCost;
    lastAction_.dashDestination.x += directionX * DashDistance;
    lastAction_.dashDestination.y += directionY * DashDistance;
    return lastAction_;
}

ShadowActionReport ShadowbladeActions::TryFatalStrike(const Math::Vec3& position,
    CombatSandbox& combatSandbox) {
    lastAction_ = {ShadowActionType::FatalStrike, ShadowActionResult::Ready, 0, position};
    if (guarding_) {
        lastAction_.result = ShadowActionResult::GuardedConflict;
        return lastAction_;
    }
    if (combatSandbox.Dummy().IsDefeated()) {
        lastAction_.result = ShadowActionResult::TargetDefeated;
        return lastAction_;
    }
    if (fatalStrikeCooldownRemaining_ > 0.0f) {
        lastAction_.result = ShadowActionResult::Cooldown;
        return lastAction_;
    }

    const bool followUp = combatSandbox.IsStaggered();
    const float resourceCost = followUp ? StaggerFollowUpCost : FatalStrikeCost;
    if (resource_ < resourceCost) {
        lastAction_.result = ShadowActionResult::InsufficientResource;
        return lastAction_;
    }

    const Math::Vec3 target = combatSandbox.Dummy().position;
    const float deltaX = target.x - position.x;
    const float deltaY = target.y - position.y;
    if (deltaX * deltaX + deltaY * deltaY > FatalStrikeRange * FatalStrikeRange) {
        lastAction_.result = ShadowActionResult::OutOfRange;
        return lastAction_;
    }

    resource_ -= resourceCost;
    fatalStrikeCooldownRemaining_ = FatalStrikeCooldownSeconds;
    lastAction_.result = ShadowActionResult::Activated;
    lastAction_.followUp = followUp;
    lastAction_.resourceSpent = resourceCost;
    if (followUp) {
        combatSandbox.ConsumeStaggerOpening();
    }
    lastAction_.damageApplied = combatSandbox.ApplyDamage(FatalStrikeDamage);
    return lastAction_;
}

} // namespace Astral::Scene
