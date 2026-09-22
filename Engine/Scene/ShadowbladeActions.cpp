#include "Engine/Scene/ShadowbladeActions.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Astral::Scene {

std::int64_t ShadowbladeActions::DefenseSecondsToMicros(double seconds) {
    if (!std::isfinite(seconds) || seconds <= 0.0) return 0;
    constexpr double MaximumSeconds = static_cast<double>(
        std::numeric_limits<std::int64_t>::max())
        / static_cast<double>(DefenseMicrosPerSecond);
    if (seconds >= MaximumSeconds) return std::numeric_limits<std::int64_t>::max();
    return static_cast<std::int64_t>(std::llround(
        seconds * static_cast<double>(DefenseMicrosPerSecond)));
}

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

std::int64_t ShadowbladeActions::DefenseTimingToleranceMicros(
    double deadlineUncertaintySeconds) const {
    constexpr double HalfMicrosecond = 0.5 / static_cast<double>(DefenseMicrosPerSecond);
    const double uncertaintySeconds = defenseElapsedUncertaintySeconds_
        + std::max(0.0, deadlineUncertaintySeconds) + HalfMicrosecond;
    if (!std::isfinite(uncertaintySeconds)) {
        return std::numeric_limits<std::int64_t>::max();
    }
    const double uncertaintyMicros = std::ceil(
        uncertaintySeconds * static_cast<double>(DefenseMicrosPerSecond));
    if (uncertaintyMicros >= static_cast<double>(std::numeric_limits<std::int64_t>::max())) {
        return std::numeric_limits<std::int64_t>::max();
    }
    return std::max<std::int64_t>(1, static_cast<std::int64_t>(uncertaintyMicros));
}

bool ShadowbladeActions::DefenseDeadlineReached(std::int64_t now,
    std::int64_t deadline, std::int64_t toleranceMicros) {
    if (deadline <= 0) return false;
    if (now >= deadline) return true;
    return deadline - now <= toleranceMicros;
}

bool ShadowbladeActions::DefenseWindowContains(std::int64_t remaining,
    std::int64_t window, std::int64_t toleranceMicros) {
    if (remaining <= window) return true;
    return remaining - window <= toleranceMicros;
}

std::int64_t ShadowbladeActions::CurrentDefenseMicros() const {
    return DefenseSecondsToMicros(defenseElapsedSecondsPrecise_);
}

void ShadowbladeActions::RebaseDefenseClock() {
    const std::int64_t now = CurrentDefenseMicros();
    if (defenseCounterEndMicros_ > 0) {
        const std::int64_t tolerance = DefenseTimingToleranceMicros(
            defenseCounterDeadlineUncertaintySeconds_);
        if (DefenseDeadlineReached(now, defenseCounterEndMicros_, tolerance)) {
            defenseCounterEndMicros_ = 0;
            defenseCounterDeadlineUncertaintySeconds_ = 0.0;
        } else {
            defenseCounterEndMicros_ -= now;
            defenseCounterDeadlineUncertaintySeconds_ += defenseElapsedUncertaintySeconds_
                + 0.5 / static_cast<double>(DefenseMicrosPerSecond);
        }
    }
    defenseElapsedSecondsPrecise_ = 0.0;
    defenseElapsedUncertaintySeconds_ = 0.0;
}

std::int64_t ShadowbladeActions::StartDefenseCounterDeadline() {
    RebaseDefenseClock();
    defenseCounterDeadlineUncertaintySeconds_ = FloatHalfUlpSeconds(
        DefenseCounterWindowSeconds);
    return DefenseSecondsToMicros(DefenseCounterWindowSeconds);
}

void ShadowbladeActions::AdvanceTime(float deltaSeconds) {
    if (deltaSeconds <= 0.0f || !std::isfinite(deltaSeconds)) return;

    resource_ = std::min(MaximumResource,
        resource_ + ResourceRegenerationPerSecond * deltaSeconds);
    dashCooldownRemaining_ = std::max(0.0f, dashCooldownRemaining_ - deltaSeconds);
    fatalStrikeCooldownRemaining_ = std::max(0.0f,
        fatalStrikeCooldownRemaining_ - deltaSeconds);

    if (!incomingAttackActive_ && defenseCounterEndMicros_ <= 0) {
        defenseElapsedSecondsPrecise_ = 0.0;
        defenseElapsedUncertaintySeconds_ = 0.0;
        return;
    }

    defenseElapsedSecondsPrecise_ += static_cast<double>(deltaSeconds);
    defenseElapsedUncertaintySeconds_ += FloatHalfUlpSeconds(deltaSeconds);
    if (!std::isfinite(defenseElapsedSecondsPrecise_)) {
        defenseElapsedSecondsPrecise_ = static_cast<double>(
            std::numeric_limits<std::int64_t>::max())
            / static_cast<double>(DefenseMicrosPerSecond);
    }

    const std::int64_t now = CurrentDefenseMicros();
    if (defenseCounterEndMicros_ > 0) {
        const std::int64_t counterTolerance = DefenseTimingToleranceMicros(
            defenseCounterDeadlineUncertaintySeconds_);
        if (DefenseDeadlineReached(now, defenseCounterEndMicros_, counterTolerance)) {
            defenseCounterEndMicros_ = 0;
            defenseCounterDeadlineUncertaintySeconds_ = 0.0;
        }
    }

    if (incomingAttackActive_) {
        const std::int64_t attackTolerance = DefenseTimingToleranceMicros(
            incomingAttackDeadlineUncertaintySeconds_);
        if (DefenseDeadlineReached(now, incomingAttackEndMicros_, attackTolerance)) {
            ResolveIncomingHit(DefenseResult::Hit);
        }
    }

    if (!incomingAttackActive_ && defenseCounterEndMicros_ <= 0) {
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
        defenseCounterEndMicros_ = 0;
        defenseCounterDeadlineUncertaintySeconds_ = 0.0;
    }
    lastAction_.damageApplied = combatSandbox.ApplyDamage(FatalStrikeDamage);
    if (lastAction_.damageApplied > 0) {
        combatSandbox.RegisterSuccessfulAttackHit();
    }
    return lastAction_;
}

bool ShadowbladeActions::BeginIncomingAttack(const IncomingAttackDefinition& attack) {
    const std::int64_t windupMicros = DefenseSecondsToMicros(
        static_cast<double>(attack.windupSeconds));
    if (incomingAttackActive_ || playerHealth_ <= 0 || attack.windupSeconds <= 0.0f
        || !std::isfinite(attack.windupSeconds) || windupMicros <= 0 || attack.damage <= 0
        || attack.guardDamage < 0) {
        lastDefense_ = {DefenseResult::InvalidThreat, 0, 0, false, 0.0f};
        return false;
    }

    RebaseDefenseClock();
    incomingAttack_ = attack;
    incomingAttackEndMicros_ = windupMicros;
    incomingAttackDeadlineUncertaintySeconds_ = FloatHalfUlpSeconds(attack.windupSeconds);
    incomingAttackActive_ = true;
    lastDefense_ = {DefenseResult::ThreatQueued, 0, 0, false,
        static_cast<float>(windupMicros) / static_cast<float>(DefenseMicrosPerSecond)};
    return true;
}

DefenseReport ShadowbladeActions::TryDefend(DefenseInput input) {
    if (!incomingAttackActive_) {
        lastDefense_ = {DefenseResult::NoThreat, 0, 0, false, 0.0f};
        return lastDefense_;
    }

    const std::int64_t now = CurrentDefenseMicros();
    const std::int64_t remainingMicros = incomingAttackEndMicros_ > now
        ? incomingAttackEndMicros_ - now
        : 0;
    const float remaining = static_cast<float>(remainingMicros)
        / static_cast<float>(DefenseMicrosPerSecond);
    const float perfectWindowSeconds = PerfectDefenseWindowSeconds();
    const std::int64_t perfectWindowMicros = DefenseSecondsToMicros(
        static_cast<double>(perfectWindowSeconds));
    const std::int64_t perfectTolerance = DefenseTimingToleranceMicros(
        incomingAttackDeadlineUncertaintySeconds_ + FloatHalfUlpSeconds(perfectWindowSeconds));

    if (input == DefenseInput::Dodge) {
        const std::int64_t dodgeTolerance = DefenseTimingToleranceMicros(
            incomingAttackDeadlineUncertaintySeconds_ + FloatHalfUlpSeconds(DodgeWindowSeconds));
        if (!DefenseWindowContains(remainingMicros,
                DefenseSecondsToMicros(DodgeWindowSeconds), dodgeTolerance)) {
            lastDefense_ = {DefenseResult::TooEarly, 0, 0, false, remaining};
            return lastDefense_;
        }

        incomingAttackActive_ = false;
        incomingAttackEndMicros_ = 0;
        incomingAttackDeadlineUncertaintySeconds_ = 0.0;
        if (DefenseWindowContains(remainingMicros, perfectWindowMicros, perfectTolerance)) {
            defenseCounterEndMicros_ = StartDefenseCounterDeadline();
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
    incomingAttackEndMicros_ = 0;
    incomingAttackDeadlineUncertaintySeconds_ = 0.0;
    if (DefenseWindowContains(remainingMicros, perfectWindowMicros, perfectTolerance)) {
        defenseCounterEndMicros_ = StartDefenseCounterDeadline();
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
    incomingAttackEndMicros_ = 0;
    incomingAttackDeadlineUncertaintySeconds_ = 0.0;
    defenseCounterEndMicros_ = 0;
    defenseCounterDeadlineUncertaintySeconds_ = 0.0;
    defenseTimingPreset_ = DefenseTimingPreset::Standard;
    lastDefense_ = {};
}

float ShadowbladeActions::IncomingAttackRemaining() const {
    if (!incomingAttackActive_) return 0.0f;
    const std::int64_t now = CurrentDefenseMicros();
    const std::int64_t tolerance = DefenseTimingToleranceMicros(
        incomingAttackDeadlineUncertaintySeconds_);
    if (DefenseDeadlineReached(now, incomingAttackEndMicros_, tolerance)) return 0.0f;
    const std::int64_t remaining = incomingAttackEndMicros_ - now;
    return static_cast<float>(remaining) / static_cast<float>(DefenseMicrosPerSecond);
}

bool ShadowbladeActions::HasDefenseCounter() const {
    if (defenseCounterEndMicros_ <= 0) return false;
    const std::int64_t tolerance = DefenseTimingToleranceMicros(
        defenseCounterDeadlineUncertaintySeconds_);
    return !DefenseDeadlineReached(CurrentDefenseMicros(), defenseCounterEndMicros_, tolerance);
}

float ShadowbladeActions::DefenseCounterRemaining() const {
    if (defenseCounterEndMicros_ <= 0) return 0.0f;
    const std::int64_t now = CurrentDefenseMicros();
    const std::int64_t tolerance = DefenseTimingToleranceMicros(
        defenseCounterDeadlineUncertaintySeconds_);
    if (DefenseDeadlineReached(now, defenseCounterEndMicros_, tolerance)) return 0.0f;
    const std::int64_t remaining = defenseCounterEndMicros_ - now;
    return static_cast<float>(remaining) / static_cast<float>(DefenseMicrosPerSecond);
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
    incomingAttackEndMicros_ = 0;
    incomingAttackDeadlineUncertaintySeconds_ = 0.0;
    RebaseDefenseClock();
    lastDefense_ = {result, previousHealth - playerHealth_, 0, false, 0.0f};
    return lastDefense_;
}

} // namespace Astral::Scene
