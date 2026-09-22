#include "Engine/Scene/ShadowbladeActions.h"

#include <algorithm>
#include <cmath>

namespace Astral::Scene {

std::int64_t ShadowbladeActions::DefenseSecondsToMicros(double seconds) {
    return static_cast<std::int64_t>(std::llround(
        seconds * static_cast<double>(DefenseMicrosPerSecond)));
}

void ShadowbladeActions::AdvanceTime(float deltaSeconds) {
    if (deltaSeconds <= 0.0f || !std::isfinite(deltaSeconds)) return;

    resource_ = std::min(MaximumResource,
        resource_ + ResourceRegenerationPerSecond * deltaSeconds);
    dashCooldownRemaining_ = std::max(0.0f, dashCooldownRemaining_ - deltaSeconds);
    fatalStrikeCooldownRemaining_ = std::max(0.0f,
        fatalStrikeCooldownRemaining_ - deltaSeconds);

    defenseCounterRemainingPrecise_ = std::max(0.0,
        defenseCounterRemainingPrecise_ - static_cast<double>(deltaSeconds));
    if (DefenseSecondsToMicros(defenseCounterRemainingPrecise_) <= 0) {
        defenseCounterRemainingPrecise_ = 0.0;
    }

    if (incomingAttackActive_) {
        incomingAttackRemainingPrecise_ = std::max(0.0,
            incomingAttackRemainingPrecise_ - static_cast<double>(deltaSeconds));
        if (DefenseSecondsToMicros(incomingAttackRemainingPrecise_) <= 0) {
            incomingAttackRemainingPrecise_ = 0.0;
            ResolveIncomingHit(DefenseResult::Hit);
        }
    }
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

    const bool staggerFollowUp = combatSandbox.IsStaggered();
    const bool defenseCounter = HasDefenseCounter();
    float resourceCost = FatalStrikeCost;
    if (staggerFollowUp) {
        resourceCost = StaggerFollowUpCost;
    } else if (defenseCounter) {
        resourceCost = DefenseCounterFatalStrikeCost;
    }
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
    lastAction_.followUp = staggerFollowUp || defenseCounter;
    lastAction_.resourceSpent = resourceCost;
    if (staggerFollowUp) {
        combatSandbox.ConsumeStaggerOpening();
    }
    if (defenseCounter) {
        defenseCounterRemainingPrecise_ = 0.0;
    }
    lastAction_.damageApplied = combatSandbox.ApplyDamage(FatalStrikeDamage);
    if (lastAction_.damageApplied > 0) {
        combatSandbox.RegisterSuccessfulAttackHit();
    }
    return lastAction_;
}

bool ShadowbladeActions::BeginIncomingAttack(const IncomingAttackDefinition& attack) {
    if (incomingAttackActive_ || playerHealth_ <= 0 || attack.windupSeconds <= 0.0f
        || !std::isfinite(attack.windupSeconds) || attack.damage <= 0
        || attack.guardDamage < 0) {
        lastDefense_ = {DefenseResult::InvalidThreat, 0, 0, false, 0.0f};
        return false;
    }

    incomingAttack_ = attack;
    incomingAttackRemainingPrecise_ = static_cast<double>(attack.windupSeconds);
    incomingAttackActive_ = true;
    lastDefense_ = {DefenseResult::ThreatQueued, 0, 0, false, attack.windupSeconds};
    return true;
}

DefenseReport ShadowbladeActions::TryDefend(DefenseInput input) {
    if (!incomingAttackActive_) {
        lastDefense_ = {DefenseResult::NoThreat, 0, 0, false, 0.0f};
        return lastDefense_;
    }

    const std::int64_t remainingMicros = DefenseSecondsToMicros(
        incomingAttackRemainingPrecise_);
    const float remaining = static_cast<float>(remainingMicros)
        / static_cast<float>(DefenseMicrosPerSecond);
    const std::int64_t perfectWindowMicros = DefenseSecondsToMicros(
        static_cast<double>(PerfectDefenseWindowSeconds()));

    if (input == DefenseInput::Dodge) {
        if (remainingMicros > DefenseSecondsToMicros(DodgeWindowSeconds)) {
            lastDefense_ = {DefenseResult::TooEarly, 0, 0, false, remaining};
            return lastDefense_;
        }

        incomingAttackActive_ = false;
        incomingAttackRemainingPrecise_ = 0.0;
        if (remainingMicros <= perfectWindowMicros) {
            defenseCounterRemainingPrecise_ = DefenseCounterWindowSeconds;
            lastDefense_ = {DefenseResult::PerfectDodge, 0, 0, true, remaining};
        } else {
            lastDefense_ = {DefenseResult::Evaded, 0, 0, false, remaining};
        }
        return lastDefense_;
    }

    if (!incomingAttack_.blockable) {
        return ResolveIncomingHit(DefenseResult::UnblockableHit);
    }

    incomingAttackActive_ = false;
    incomingAttackRemainingPrecise_ = 0.0;
    if (remainingMicros <= perfectWindowMicros) {
        defenseCounterRemainingPrecise_ = DefenseCounterWindowSeconds;
        lastDefense_ = {DefenseResult::PerfectGuard, 0, 0, true, remaining};
        return lastDefense_;
    }

    if (incomingAttack_.guardDamage >= guardIntegrity_) {
        const int guardLost = guardIntegrity_;
        guardIntegrity_ = 0;
        const int previousHealth = playerHealth_;
        playerHealth_ = std::max(0, playerHealth_ - incomingAttack_.damage);
        lastDefense_ = {DefenseResult::GuardBroken,
            previousHealth - playerHealth_, guardLost, false, remaining};
        return lastDefense_;
    }

    guardIntegrity_ -= incomingAttack_.guardDamage;
    lastDefense_ = {DefenseResult::Guarded, 0, incomingAttack_.guardDamage, false, remaining};
    return lastDefense_;
}

void ShadowbladeActions::ResetDefenseState() {
    playerHealth_ = MaximumPlayerHealth;
    guardIntegrity_ = MaximumGuardIntegrity;
    incomingAttackActive_ = false;
    incomingAttackRemainingPrecise_ = 0.0;
    defenseCounterRemainingPrecise_ = 0.0;
    defenseTimingPreset_ = DefenseTimingPreset::Standard;
    lastDefense_ = {};
}

float ShadowbladeActions::IncomingAttackRemaining() const {
    const std::int64_t micros = std::max<std::int64_t>(0,
        DefenseSecondsToMicros(incomingAttackRemainingPrecise_));
    return static_cast<float>(micros) / static_cast<float>(DefenseMicrosPerSecond);
}

bool ShadowbladeActions::HasDefenseCounter() const {
    return DefenseSecondsToMicros(defenseCounterRemainingPrecise_) > 0;
}

float ShadowbladeActions::DefenseCounterRemaining() const {
    const std::int64_t micros = std::max<std::int64_t>(0,
        DefenseSecondsToMicros(defenseCounterRemainingPrecise_));
    return static_cast<float>(micros) / static_cast<float>(DefenseMicrosPerSecond);
}

float ShadowbladeActions::PerfectDefenseWindowSeconds() const {
    return defenseTimingPreset_ == DefenseTimingPreset::Forgiving
        ? ForgivingPerfectDefenseWindowSeconds
        : StandardPerfectDefenseWindowSeconds;
}

DefenseReport ShadowbladeActions::ResolveIncomingHit(DefenseResult result) {
    if (!incomingAttackActive_) {
        lastDefense_ = {DefenseResult::NoThreat, 0, 0, false, 0.0f};
        return lastDefense_;
    }

    const int previousHealth = playerHealth_;
    playerHealth_ = std::max(0, playerHealth_ - incomingAttack_.damage);
    incomingAttackActive_ = false;
    incomingAttackRemainingPrecise_ = 0.0;
    lastDefense_ = {result, previousHealth - playerHealth_, 0, false, 0.0f};
    return lastDefense_;
}

} // namespace Astral::Scene
