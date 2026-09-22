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

std::int64_t ShadowbladeActions::SaturatingMicrosAdd(std::int64_t left,
    std::int64_t right) {
    if (left <= 0) return std::max<std::int64_t>(0, right);
    if (right <= 0) return left;
    const std::int64_t maximum = std::numeric_limits<std::int64_t>::max();
    return left > maximum - right ? maximum : left + right;
}

bool ShadowbladeActions::DefenseDeadlineReached(std::int64_t now,
    std::int64_t deadline) {
    if (deadline <= 0) return false;
    if (now >= deadline) return true;
    return deadline - now <= DefenseBoundaryToleranceMicros;
}

bool ShadowbladeActions::DefenseWindowContains(std::int64_t remaining,
    std::int64_t window) {
    if (remaining <= window) return true;
    return remaining - window <= DefenseBoundaryToleranceMicros;
}

std::int64_t ShadowbladeActions::CurrentDefenseMicros() const {
    return DefenseSecondsToMicros(defenseElapsedSecondsPrecise_);
}

std::int64_t ShadowbladeActions::StartDefenseCounterDeadline() {
    const std::int64_t counterMicros = DefenseSecondsToMicros(
        DefenseCounterWindowSeconds);
    std::int64_t now = CurrentDefenseMicros();
    const std::int64_t maximum = std::numeric_limits<std::int64_t>::max();
    if (counterMicros > 0 && now > maximum - counterMicros) {
        defenseElapsedSecondsPrecise_ = 0.0;
        now = 0;
    }
    return SaturatingMicrosAdd(now, counterMicros);
}

void ShadowbladeActions::AdvanceTime(float deltaSeconds) {
    if (deltaSeconds <= 0.0f || !std::isfinite(deltaSeconds)) return;

    resource_ = std::min(MaximumResource,
        resource_ + ResourceRegenerationPerSecond * deltaSeconds);
    dashCooldownRemaining_ = std::max(0.0f, dashCooldownRemaining_ - deltaSeconds);
    fatalStrikeCooldownRemaining_ = std::max(0.0f,
        fatalStrikeCooldownRemaining_ - deltaSeconds);

    defenseElapsedSecondsPrecise_ += static_cast<double>(deltaSeconds);
    if (!std::isfinite(defenseElapsedSecondsPrecise_)) {
        defenseElapsedSecondsPrecise_ = static_cast<double>(
            std::numeric_limits<std::int64_t>::max())
            / static_cast<double>(DefenseMicrosPerSecond);
    }
    const std::int64_t now = CurrentDefenseMicros();
    if (defenseCounterEndMicros_ > 0
        && DefenseDeadlineReached(now, defenseCounterEndMicros_)) {
        defenseCounterEndMicros_ = 0;
    }

    if (incomingAttackActive_
        && DefenseDeadlineReached(now, incomingAttackEndMicros_)) {
        ResolveIncomingHit(DefenseResult::Hit);
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

    std::int64_t now = CurrentDefenseMicros();
    const std::int64_t maximum = std::numeric_limits<std::int64_t>::max();
    if (now > maximum - windupMicros) {
        const std::int64_t counterRemaining = defenseCounterEndMicros_ > now
            ? defenseCounterEndMicros_ - now
            : 0;
        defenseElapsedSecondsPrecise_ = 0.0;
        defenseCounterEndMicros_ = counterRemaining;
        now = 0;
    }

    incomingAttack_ = attack;
    incomingAttackEndMicros_ = now + windupMicros;
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
    const std::int64_t perfectWindowMicros = DefenseSecondsToMicros(
        static_cast<double>(PerfectDefenseWindowSeconds()));

    if (input == DefenseInput::Dodge) {
        if (!DefenseWindowContains(remainingMicros,
                DefenseSecondsToMicros(DodgeWindowSeconds))) {
            lastDefense_ = {DefenseResult::TooEarly, 0, 0, false, remaining};
            return lastDefense_;
        }

        incomingAttackActive_ = false;
        incomingAttackEndMicros_ = 0;
        if (DefenseWindowContains(remainingMicros, perfectWindowMicros)) {
            defenseCounterEndMicros_ = StartDefenseCounterDeadline();
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
    incomingAttackEndMicros_ = 0;
    if (DefenseWindowContains(remainingMicros, perfectWindowMicros)) {
        defenseCounterEndMicros_ = StartDefenseCounterDeadline();
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
    defenseElapsedSecondsPrecise_ = 0.0;
    incomingAttackEndMicros_ = 0;
    defenseCounterEndMicros_ = 0;
    defenseTimingPreset_ = DefenseTimingPreset::Standard;
    lastDefense_ = {};
}

float ShadowbladeActions::IncomingAttackRemaining() const {
    if (!incomingAttackActive_) return 0.0f;
    const std::int64_t now = CurrentDefenseMicros();
    if (DefenseDeadlineReached(now, incomingAttackEndMicros_)) return 0.0f;
    const std::int64_t remaining = incomingAttackEndMicros_ - now;
    return static_cast<float>(remaining) / static_cast<float>(DefenseMicrosPerSecond);
}

bool ShadowbladeActions::HasDefenseCounter() const {
    return defenseCounterEndMicros_ > 0
        && !DefenseDeadlineReached(CurrentDefenseMicros(), defenseCounterEndMicros_);
}

float ShadowbladeActions::DefenseCounterRemaining() const {
    const std::int64_t now = CurrentDefenseMicros();
    if (defenseCounterEndMicros_ <= 0
        || DefenseDeadlineReached(now, defenseCounterEndMicros_)) {
        return 0.0f;
    }
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
    lastDefense_ = {result, previousHealth - playerHealth_, 0, false, 0.0f};
    return lastDefense_;
}

} // namespace Astral::Scene
