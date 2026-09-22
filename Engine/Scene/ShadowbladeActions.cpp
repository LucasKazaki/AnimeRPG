#include "Engine/Scene/ShadowbladeActions.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Astral::Scene {

double ShadowbladeActions::FloatHalfUlpSeconds(float seconds) {
    if (!std::isfinite(seconds) || seconds <= 0.0f) return 0.0;
    const float next = std::nextafter(seconds, std::numeric_limits<float>::infinity());
    if (std::isfinite(next)) {
        return std::max(0.0,
            (static_cast<double>(next) - static_cast<double>(seconds)) * 0.5);
    }
    const float previous = std::nextafter(seconds, 0.0f);
    return std::max(0.0,
        (static_cast<double>(seconds) - static_cast<double>(previous)) * 0.5);
}

double ShadowbladeActions::DefenseTimingToleranceSeconds(
    double deadlineUncertaintySeconds) const {
    const double uncertaintySeconds = defenseElapsedUncertaintySeconds_
        + std::max(0.0, deadlineUncertaintySeconds);
    return std::isfinite(uncertaintySeconds)
        ? uncertaintySeconds
        : std::numeric_limits<double>::infinity();
}

bool ShadowbladeActions::DefenseDeadlineReached(double now,
    double deadline, double toleranceSeconds) {
    if (!(deadline > 0.0) || !std::isfinite(now)) return false;
    if (now >= deadline) return true;
    return deadline - now <= std::max(0.0, toleranceSeconds);
}

bool ShadowbladeActions::DefenseWindowContains(double remaining,
    double window, double toleranceSeconds) {
    if (remaining <= window) return true;
    return remaining - window <= std::max(0.0, toleranceSeconds);
}

void ShadowbladeActions::RebaseDefenseClock() {
    const double now = CurrentDefenseSeconds();
    if (defenseCounterEndSeconds_ > 0.0) {
        const double tolerance = DefenseTimingToleranceSeconds(
            defenseCounterDeadlineUncertaintySeconds_);
        if (DefenseDeadlineReached(now, defenseCounterEndSeconds_, tolerance)) {
            defenseCounterEndSeconds_ = 0.0;
            defenseCounterDeadlineUncertaintySeconds_ = 0.0;
        } else {
            defenseCounterEndSeconds_ -= now;
            defenseCounterDeadlineUncertaintySeconds_ += defenseElapsedUncertaintySeconds_;
        }
    }
    defenseElapsedSecondsPrecise_ = 0.0;
    defenseElapsedUncertaintySeconds_ = 0.0;
}

double ShadowbladeActions::StartDefenseCounterDeadline() {
    RebaseDefenseClock();
    defenseCounterDeadlineUncertaintySeconds_ = FloatHalfUlpSeconds(
        DefenseCounterWindowSeconds);
    return static_cast<double>(DefenseCounterWindowSeconds);
}

void ShadowbladeActions::AdvanceTime(float deltaSeconds) {
    if (deltaSeconds <= 0.0f || !std::isfinite(deltaSeconds)) return;

    resource_ = std::min(MaximumResource,
        resource_ + ResourceRegenerationPerSecond * deltaSeconds);
    dashCooldownRemaining_ = std::max(0.0f, dashCooldownRemaining_ - deltaSeconds);
    fatalStrikeCooldownRemaining_ = std::max(0.0f,
        fatalStrikeCooldownRemaining_ - deltaSeconds);

    if (!incomingAttackActive_ && defenseCounterEndSeconds_ <= 0.0) {
        defenseElapsedSecondsPrecise_ = 0.0;
        defenseElapsedUncertaintySeconds_ = 0.0;
        return;
    }

    defenseElapsedSecondsPrecise_ += static_cast<double>(deltaSeconds);
    defenseElapsedUncertaintySeconds_ += FloatHalfUlpSeconds(deltaSeconds);

    const double now = CurrentDefenseSeconds();
    if (defenseCounterEndSeconds_ > 0.0) {
        const double counterTolerance = DefenseTimingToleranceSeconds(
            defenseCounterDeadlineUncertaintySeconds_);
        if (DefenseDeadlineReached(now, defenseCounterEndSeconds_, counterTolerance)) {
            defenseCounterEndSeconds_ = 0.0;
            defenseCounterDeadlineUncertaintySeconds_ = 0.0;
        }
    }

    if (incomingAttackActive_) {
        const double attackTolerance = DefenseTimingToleranceSeconds(
            incomingAttackDeadlineUncertaintySeconds_);
        if (DefenseDeadlineReached(now, incomingAttackEndSeconds_, attackTolerance)) {
            ResolveIncomingHit(DefenseResult::Hit);
        }
    }

    if (!incomingAttackActive_ && defenseCounterEndSeconds_ <= 0.0) {
        defenseElapsedSecondsPrecise_ = 0.0;
        defenseElapsedUncertaintySeconds_ = 0.0;
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
        defenseCounterEndSeconds_ = 0.0;
        defenseCounterDeadlineUncertaintySeconds_ = 0.0;
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

    ++incomingAttackGeneration_;
    if (incomingAttackGeneration_ == 0) ++incomingAttackGeneration_;
    RebaseDefenseClock();
    incomingAttack_ = attack;
    incomingAttackEndSeconds_ = static_cast<double>(attack.windupSeconds);
    incomingAttackDeadlineUncertaintySeconds_ = FloatHalfUlpSeconds(attack.windupSeconds);
    incomingAttackActive_ = true;
    lastDefense_ = {DefenseResult::ThreatQueued, 0, 0, false, attack.windupSeconds};
    return true;
}

DefenseReport ShadowbladeActions::TryDefend(DefenseInput input) {
    if (!incomingAttackActive_) {
        lastDefense_ = {DefenseResult::NoThreat, 0, 0, false, 0.0f};
        return lastDefense_;
    }

    const double now = CurrentDefenseSeconds();
    const double remainingSeconds = incomingAttackEndSeconds_ > now
        ? incomingAttackEndSeconds_ - now
        : 0.0;
    const float remaining = static_cast<float>(remainingSeconds);
    const float perfectWindowSeconds = PerfectDefenseWindowSeconds();
    const double perfectWindow = static_cast<double>(perfectWindowSeconds);
    const double perfectTolerance = DefenseTimingToleranceSeconds(
        incomingAttackDeadlineUncertaintySeconds_ + FloatHalfUlpSeconds(perfectWindowSeconds));

    if (input == DefenseInput::Dodge) {
        const double dodgeWindow = static_cast<double>(DodgeWindowSeconds);
        const double dodgeTolerance = DefenseTimingToleranceSeconds(
            incomingAttackDeadlineUncertaintySeconds_ + FloatHalfUlpSeconds(DodgeWindowSeconds));
        if (!DefenseWindowContains(remainingSeconds, dodgeWindow, dodgeTolerance)) {
            lastDefense_ = {DefenseResult::TooEarly, 0, 0, false, remaining};
            return lastDefense_;
        }

        incomingAttackActive_ = false;
        incomingAttackEndSeconds_ = 0.0;
        incomingAttackDeadlineUncertaintySeconds_ = 0.0;
        if (DefenseWindowContains(remainingSeconds, perfectWindow, perfectTolerance)) {
            defenseCounterEndSeconds_ = StartDefenseCounterDeadline();
            lastDefense_ = {DefenseResult::PerfectDodge, 0, 0, true, remaining};
        } else {
            RebaseDefenseClock();
            lastDefense_ = {DefenseResult::Evaded, 0, 0, false, remaining};
        }
        return lastDefense_;
    }

    if (!incomingAttack_.blockable) {
        return ResolveIncomingHit(DefenseResult::UnblockableHit);
    }

    incomingAttackActive_ = false;
    incomingAttackEndSeconds_ = 0.0;
    incomingAttackDeadlineUncertaintySeconds_ = 0.0;
    if (DefenseWindowContains(remainingSeconds, perfectWindow, perfectTolerance)) {
        defenseCounterEndSeconds_ = StartDefenseCounterDeadline();
        lastDefense_ = {DefenseResult::PerfectGuard, 0, 0, true, remaining};
        return lastDefense_;
    }

    if (incomingAttack_.guardDamage >= guardIntegrity_) {
        const int guardLost = guardIntegrity_;
        guardIntegrity_ = 0;
        const int previousHealth = playerHealth_;
        playerHealth_ = std::max(0, playerHealth_ - incomingAttack_.damage);
        RebaseDefenseClock();
        lastDefense_ = {DefenseResult::GuardBroken,
            previousHealth - playerHealth_, guardLost, false, remaining};
        return lastDefense_;
    }

    guardIntegrity_ -= incomingAttack_.guardDamage;
    RebaseDefenseClock();
    lastDefense_ = {DefenseResult::Guarded, 0, incomingAttack_.guardDamage, false, remaining};
    return lastDefense_;
}

void ShadowbladeActions::ResetDefenseState() {
    playerHealth_ = MaximumPlayerHealth;
    guardIntegrity_ = MaximumGuardIntegrity;
    incomingAttackActive_ = false;
    defenseElapsedSecondsPrecise_ = 0.0;
    defenseElapsedUncertaintySeconds_ = 0.0;
    incomingAttackEndSeconds_ = 0.0;
    incomingAttackDeadlineUncertaintySeconds_ = 0.0;
    defenseCounterEndSeconds_ = 0.0;
    defenseCounterDeadlineUncertaintySeconds_ = 0.0;
    defenseTimingPreset_ = DefenseTimingPreset::Standard;
    lastDefense_ = {};
}

float ShadowbladeActions::IncomingAttackRemaining() const {
    if (!incomingAttackActive_) return 0.0f;
    const double now = CurrentDefenseSeconds();
    const double tolerance = DefenseTimingToleranceSeconds(
        incomingAttackDeadlineUncertaintySeconds_);
    if (DefenseDeadlineReached(now, incomingAttackEndSeconds_, tolerance)) return 0.0f;
    return static_cast<float>(incomingAttackEndSeconds_ - now);
}

bool ShadowbladeActions::HasDefenseCounter() const {
    if (defenseCounterEndSeconds_ <= 0.0) return false;
    const double tolerance = DefenseTimingToleranceSeconds(
        defenseCounterDeadlineUncertaintySeconds_);
    return !DefenseDeadlineReached(CurrentDefenseSeconds(), defenseCounterEndSeconds_, tolerance);
}

float ShadowbladeActions::DefenseCounterRemaining() const {
    if (defenseCounterEndSeconds_ <= 0.0) return 0.0f;
    const double now = CurrentDefenseSeconds();
    const double tolerance = DefenseTimingToleranceSeconds(
        defenseCounterDeadlineUncertaintySeconds_);
    if (DefenseDeadlineReached(now, defenseCounterEndSeconds_, tolerance)) return 0.0f;
    return static_cast<float>(defenseCounterEndSeconds_ - now);
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
    incomingAttackEndSeconds_ = 0.0;
    incomingAttackDeadlineUncertaintySeconds_ = 0.0;
    RebaseDefenseClock();
    lastDefense_ = {result, previousHealth - playerHealth_, 0, false, 0.0f};
    return lastDefense_;
}

} // namespace Astral::Scene
