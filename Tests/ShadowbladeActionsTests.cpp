#include "Engine/Scene/ShadowbladeActions.h"
#include "Engine/Scene/CombatDefenseTraining.h"

#include <cmath>
#include <iostream>
#include <limits>

namespace {
int failures = 0;

void Expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

bool Near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.001f;
}

void TestDashCostCooldownAndRegenerationCap() {
    Astral::Scene::ShadowbladeActions actions;
    const auto activated = actions.TryDash({2.0f, 4.0f, 0.0f});
    Expect(activated.result == Astral::Scene::ShadowActionResult::Activated,
        "dash activates when ready");
    Expect(Near(activated.dashDestination.x, 2.0f)
            && Near(activated.dashDestination.y, 10.0f),
        "neutral dash direction preserves deterministic forward fallback");
    Expect(Near(activated.resourceSpent, Astral::Scene::ShadowbladeActions::DashCost),
        "dash report records its resource spend");
    Expect(Near(actions.Resource(), 75.0f), "dash consumes its defined resource cost");
    Expect(Near(actions.DashCooldownRemaining(), 1.0f), "dash starts its cooldown");

    const auto rejected = actions.TryDash({});
    Expect(rejected.result == Astral::Scene::ShadowActionResult::Cooldown,
        "dash rejects activation during cooldown");
    Expect(Near(actions.Resource(), 75.0f), "rejected dash consumes no resource");

    actions.AdvanceTime(0.5f);
    Expect(Near(actions.Resource(), 82.5f), "positive delta regenerates resource");
    Expect(Near(actions.DashCooldownRemaining(), 0.5f), "positive delta counts cooldown down");
    actions.AdvanceTime(100.0f);
    Expect(Near(actions.Resource(), 100.0f), "resource regeneration respects the cap");
    Expect(Near(actions.DashCooldownRemaining(), 0.0f), "cooldown clamps at zero");
}

void TestDirectionalDashNormalizationAndFallback() {
    using Astral::Math::Vec3;
    using Astral::Scene::ShadowActionResult;
    using Astral::Scene::ShadowbladeActions;

    ShadowbladeActions cardinal;
    const auto right = cardinal.TryDash({2.0f, 4.0f, 0.0f}, {1.0f, 0.0f, 0.0f});
    Expect(right.result == ShadowActionResult::Activated
            && Near(right.dashDestination.x, 8.0f)
            && Near(right.dashDestination.y, 4.0f),
        "cardinal dash follows the requested direction");

    ShadowbladeActions diagonal;
    const auto diagonalReport = diagonal.TryDash({}, {1.0f, 1.0f, 0.0f});
    const float diagonalDistance = std::sqrt(
        diagonalReport.dashDestination.x * diagonalReport.dashDestination.x
        + diagonalReport.dashDestination.y * diagonalReport.dashDestination.y);
    Expect(diagonalReport.result == ShadowActionResult::Activated
            && Near(diagonalDistance, ShadowbladeActions::DashDistance),
        "diagonal dash is normalized to the same distance as cardinal movement");

    ShadowbladeActions invalid;
    const float infinity = std::numeric_limits<float>::infinity();
    const auto fallback = invalid.TryDash({1.0f, 2.0f, 0.0f}, {infinity, 1.0f, 0.0f});
    Expect(fallback.result == ShadowActionResult::Activated
            && Near(fallback.dashDestination.x, 1.0f)
            && Near(fallback.dashDestination.y, 8.0f),
        "nonfinite dash direction safely falls back to deterministic forward movement");
}

void TestInvalidDeltaDoesNotMutateState() {
    Astral::Scene::ShadowbladeActions actions;
    actions.TryDash({});
    const float resource = actions.Resource();
    const float cooldown = actions.DashCooldownRemaining();
    actions.AdvanceTime(0.0f);
    actions.AdvanceTime(-1.0f);
    actions.AdvanceTime(std::numeric_limits<float>::quiet_NaN());
    actions.AdvanceTime(std::numeric_limits<float>::infinity());
    Expect(Near(actions.Resource(), resource), "invalid delta does not regenerate resource");
    Expect(Near(actions.DashCooldownRemaining(), cooldown),
        "invalid delta does not alter cooldown");
}

void TestFatalStrikeRangeDamageAndCooldown() {
    Astral::Scene::ShadowbladeActions actions;
    Astral::Scene::CombatSandbox combat;
    const auto outOfRange = actions.TryFatalStrike({-10.0f, 0.0f, 0.0f}, combat);
    Expect(outOfRange.result == Astral::Scene::ShadowActionResult::OutOfRange,
        "fatal strike rejects an out-of-range target");
    Expect(combat.Dummy().health == 100 && Near(actions.Resource(), 100.0f),
        "out-of-range fatal strike changes no health or resource");

    const auto hit = actions.TryFatalStrike({0.0f, 0.0f, 0.0f}, combat);
    Expect(hit.result == Astral::Scene::ShadowActionResult::Activated,
        "fatal strike activates in range");
    Expect(!hit.followUp && Near(hit.resourceSpent, Astral::Scene::ShadowbladeActions::FatalStrikeCost),
        "ordinary fatal strike keeps its normal resource cost");
    Expect(hit.damageApplied == 80 && combat.Dummy().health == 20,
        "fatal strike applies distinct high damage through combat domain");
    Expect(combat.ComboCount() == 1 && combat.Stats().bestCombo == 1,
        "successful fatal strike participates in the shared attack combo");
    Expect(Near(actions.Resource(), 50.0f), "fatal strike consumes its defined cost");
    Expect(Near(actions.FatalStrikeCooldownRemaining(), 2.0f),
        "fatal strike starts its cooldown");

    const auto cooldown = actions.TryFatalStrike({0.0f, 0.0f, 0.0f}, combat);
    Expect(cooldown.result == Astral::Scene::ShadowActionResult::Cooldown,
        "fatal strike rejects during cooldown");
    actions.AdvanceTime(2.0f);
    combat.AdvanceTime(2.0f);
    const auto defeat = actions.TryFatalStrike({0.0f, 0.0f, 0.0f}, combat);
    Expect(defeat.damageApplied == 20 && combat.Dummy().IsDefeated(),
        "fatal strike damage is capped to remaining dummy health");
    actions.AdvanceTime(2.0f);
    const auto defeated = actions.TryFatalStrike({0.0f, 0.0f, 0.0f}, combat);
    Expect(defeated.result == Astral::Scene::ShadowActionResult::TargetDefeated,
        "fatal strike rejects a defeated target");
}

void TestMixedAttackComboIncludesFatalStrike() {
    using namespace Astral::Scene;

    CombatSandbox combat;
    ShadowbladeActions actions;
    const auto light = combat.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    Expect(light.result == AttackResult::Hit && combat.ComboCount() == 1,
        "light attack starts the shared combo");
    combat.AdvanceTime(0.4f);
    const auto fatal = actions.TryFatalStrike({0.0f, 0.0f, 0.0f}, combat);
    Expect(fatal.result == ShadowActionResult::Activated && fatal.damageApplied == 75,
        "fatal strike succeeds inside the active combo window");
    Expect(combat.ComboCount() == 2 && combat.Stats().bestCombo == 2
            && combat.Stats().hitCount == 2,
        "light plus fatal strike records one coherent two-hit combo");
}

void TestStaggerFollowUpCostAndConsumption() {
    using namespace Astral::Scene;

    CombatSandbox combat;
    ShadowbladeActions actions;
    combat.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    combat.AdvanceTime(0.4f);
    const auto heavy = combat.TryAttack(AttackType::Heavy, {0.0f, 0.0f, 0.0f});
    Expect(heavy.staggerTriggered && combat.IsStaggered(),
        "light-heavy sequence creates one stagger opening");

    const float resourceBeforeRangeCheck = actions.Resource();
    const auto outOfRange = actions.TryFatalStrike({-10.0f, 0.0f, 0.0f}, combat);
    Expect(outOfRange.result == ShadowActionResult::OutOfRange && combat.IsStaggered(),
        "out-of-range follow-up attempt preserves the earned stagger opening");
    Expect(Near(actions.Resource(), resourceBeforeRangeCheck),
        "out-of-range follow-up attempt spends no resource");

    const auto followUp = actions.TryFatalStrike({0.0f, 0.0f, 0.0f}, combat);
    Expect(followUp.result == ShadowActionResult::Activated && followUp.followUp,
        "fatal strike recognizes a stagger follow-up");
    Expect(Near(followUp.resourceSpent, ShadowbladeActions::StaggerFollowUpCost)
            && Near(actions.Resource(), 70.0f),
        "stagger follow-up uses the reduced earned-opening cost");
    Expect(!combat.IsStaggered(), "successful follow-up consumes the stagger opening exactly once");
    Expect(followUp.damageApplied == 15 && combat.Dummy().IsDefeated(),
        "follow-up damage remains capped to target health");
    Expect(combat.ComboCount() == 3 && combat.Stats().bestCombo == 3,
        "stagger follow-up extends the successful light-heavy combo");
}

void TestGuardConflicts() {
    Astral::Scene::ShadowbladeActions actions;
    Astral::Scene::CombatSandbox combat;
    actions.SetGuarding(true);
    Expect(actions.IsGuarding(), "guard held state is active");
    Expect(actions.LastAction().type == Astral::Scene::ShadowActionType::Guard
            && actions.LastAction().result == Astral::Scene::ShadowActionResult::Guarding,
        "guard activation is visible in deterministic state");
    Expect(actions.TryDash({}).result == Astral::Scene::ShadowActionResult::GuardedConflict,
        "guard blocks dash activation");
    Expect(actions.TryFatalStrike({}, combat).result
            == Astral::Scene::ShadowActionResult::GuardedConflict,
        "guard blocks fatal strike activation");
    Expect(Near(actions.Resource(), 100.0f) && combat.Dummy().health == 100,
        "guard conflicts consume no resource or health");
    actions.SetGuarding(false);
    Expect(!actions.IsGuarding() && actions.TryDash({}).result
            == Astral::Scene::ShadowActionResult::Activated,
        "releasing guard permits activation");
}

void TestInsufficientResourceRejection() {
    Astral::Scene::ShadowbladeActions actions;
    for (int activation = 0; activation < 8; ++activation) {
        const auto report = actions.TryDash({});
        Expect(report.result == Astral::Scene::ShadowActionResult::Activated,
            "resource setup dash activates");
        actions.AdvanceTime(1.0f);
    }
    Expect(Near(actions.Resource(), 20.0f), "resource setup reaches below dash cost");
    const auto rejected = actions.TryDash({});
    Expect(rejected.result == Astral::Scene::ShadowActionResult::InsufficientResource,
        "dash deterministically rejects insufficient resource");
    Expect(Near(actions.Resource(), 20.0f), "resource rejection has no cost");
    Astral::Scene::CombatSandbox combat;
    const auto fatalRejected = actions.TryFatalStrike({}, combat);
    Expect(fatalRejected.result == Astral::Scene::ShadowActionResult::InsufficientResource,
        "fatal strike deterministically rejects insufficient resource");
    Expect(Near(actions.Resource(), 20.0f) && combat.Dummy().health == 100,
        "fatal resource rejection changes no resource or health");
}

void TestThreatDodgeAndDefenseCounter() {
    using namespace Astral::Scene;

    ShadowbladeActions actions;
    Expect(actions.BeginIncomingAttack({1.0f, 20, 30, true}),
        "valid incoming attack enters the telegraph state");
    const DefenseReport tooEarly = actions.TryDefend(DefenseInput::Dodge);
    Expect(tooEarly.result == DefenseResult::TooEarly && actions.HasIncomingAttack(),
        "early dodge does not erase the pending attack");

    actions.AdvanceTime(0.70f);
    const DefenseReport evaded = actions.TryDefend(DefenseInput::Dodge);
    Expect(evaded.result == DefenseResult::Evaded && !evaded.counterGranted,
        "dodge inside the evade window avoids damage without an automatic counter");
    Expect(actions.PlayerHealth() == ShadowbladeActions::MaximumPlayerHealth,
        "ordinary evade takes no damage");

    Expect(actions.BeginIncomingAttack({1.0f, 20, 30, true}),
        "second attack can be queued after an evade");
    actions.AdvanceTime(0.90f);
    const DefenseReport perfect = actions.TryDefend(DefenseInput::Dodge);
    Expect(perfect.result == DefenseResult::PerfectDodge && perfect.counterGranted
            && actions.HasDefenseCounter(),
        "late dodge inside the perfect window grants one bounded counter");

    CombatSandbox combat;
    const float resourceBeforeRejectedCounter = actions.Resource();
    const auto outOfRange = actions.TryFatalStrike({-10.0f, 0.0f, 0.0f}, combat);
    Expect(outOfRange.result == ShadowActionResult::OutOfRange && actions.HasDefenseCounter(),
        "rejected counter strike preserves the earned counter window");
    Expect(Near(actions.Resource(), resourceBeforeRejectedCounter),
        "rejected counter strike spends no resource");

    const auto counter = actions.TryFatalStrike({0.0f, 0.0f, 0.0f}, combat);
    Expect(counter.result == ShadowActionResult::Activated && counter.followUp,
        "successful fatal strike consumes the defense-earned follow-up");
    Expect(Near(counter.resourceSpent, ShadowbladeActions::DefenseCounterFatalStrikeCost),
        "defense counter uses its reduced resource cost");
    Expect(!actions.HasDefenseCounter(), "successful counter is single-use");
}

void TestGuardIntegrityBreakAndUnblockableHit() {
    using namespace Astral::Scene;

    ShadowbladeActions actions;
    Expect(actions.BeginIncomingAttack({1.0f, 20, 30, true}),
        "guard test queues a blockable hit");
    const DefenseReport guarded = actions.TryDefend(DefenseInput::Guard);
    Expect(guarded.result == DefenseResult::Guarded && guarded.damageTaken == 0,
        "ordinary guard prevents health damage");
    Expect(actions.GuardIntegrity() == 70 && actions.PlayerHealth() == 100,
        "ordinary guard consumes bounded guard integrity");

    Expect(actions.BeginIncomingAttack({1.0f, 20, 80, true}),
        "guard-break attack queues after prior guard");
    const DefenseReport broken = actions.TryDefend(DefenseInput::Guard);
    Expect(broken.result == DefenseResult::GuardBroken && actions.GuardIntegrity() == 0,
        "excess guard damage produces an explicit guard break");
    Expect(actions.PlayerHealth() == 80 && broken.damageTaken == 20,
        "guard break applies the attack's bounded health damage once");

    actions.ResetDefenseState();
    Expect(actions.BeginIncomingAttack({1.0f, 20, 30, false}),
        "unblockable attack is accepted as a threat");
    const DefenseReport unblockable = actions.TryDefend(DefenseInput::Guard);
    Expect(unblockable.result == DefenseResult::UnblockableHit
            && actions.PlayerHealth() == 80,
        "unblockable attacks bypass ordinary guard without double damage");
}

void TestPerfectGuardTimingPresetAndAutomaticHit() {
    using namespace Astral::Scene;

    ShadowbladeActions standard;
    Expect(standard.BeginIncomingAttack({0.18f, 20, 30, true}),
        "standard-window attack queues");
    const DefenseReport standardGuard = standard.TryDefend(DefenseInput::Guard);
    Expect(standardGuard.result == DefenseResult::Guarded && !standardGuard.counterGranted,
        "0.18 second guard is ordinary under the standard timing preset");

    ShadowbladeActions forgiving;
    forgiving.SetDefenseTimingPreset(DefenseTimingPreset::Forgiving);
    Expect(forgiving.BeginIncomingAttack({0.18f, 20, 30, true}),
        "forgiving-window attack queues");
    const DefenseReport forgivingGuard = forgiving.TryDefend(DefenseInput::Guard);
    Expect(forgivingGuard.result == DefenseResult::PerfectGuard
            && forgivingGuard.counterGranted && forgiving.GuardIntegrity() == 100,
        "forgiving preset widens the perfect window without reducing guard integrity");

    ShadowbladeActions automaticHit;
    Expect(automaticHit.BeginIncomingAttack({0.20f, 20, 30, true}),
        "automatic-hit threat queues");
    for (int step = 0; step < 4; ++step) automaticHit.AdvanceTime(0.05f);
    Expect(!automaticHit.HasIncomingAttack()
            && automaticHit.LastDefense().result == DefenseResult::Hit
            && automaticHit.PlayerHealth() == 80,
        "expired telegraph resolves one deterministic hit across split frame steps");

    ShadowbladeActions invalid;
    Expect(!invalid.BeginIncomingAttack({std::numeric_limits<float>::quiet_NaN(), 20, 30, true}),
        "nonfinite windup is rejected");
    Expect(!invalid.BeginIncomingAttack({1.0f, 0, 30, true}),
        "nonpositive damage is rejected");
    Expect(invalid.LastDefense().result == DefenseResult::InvalidThreat,
        "invalid threat rejection is visible in deterministic state");
}

void TestDefenseCounterExpiryIsSplitStable() {
    using namespace Astral::Scene;

    ShadowbladeActions oneStep;
    ShadowbladeActions split;
    oneStep.BeginIncomingAttack({0.10f, 20, 30, true});
    split.BeginIncomingAttack({0.10f, 20, 30, true});
    Expect(oneStep.TryDefend(DefenseInput::Dodge).result == DefenseResult::PerfectDodge,
        "one-step setup grants a counter");
    Expect(split.TryDefend(DefenseInput::Dodge).result == DefenseResult::PerfectDodge,
        "split-step setup grants a counter");
    oneStep.AdvanceTime(ShadowbladeActions::DefenseCounterWindowSeconds);
    for (int step = 0; step < 8; ++step) split.AdvanceTime(0.1f);
    Expect(!oneStep.HasDefenseCounter() && !split.HasDefenseCounter(),
        "counter expiration is stable across equivalent frame splits");
}

void TestIncomingAttackTimingIsSplitStable() {
    using namespace Astral::Scene;

    ShadowbladeActions oneStep;
    ShadowbladeActions split;
    Expect(oneStep.BeginIncomingAttack({8.06f, 20, 30, true})
            && split.BeginIncomingAttack({8.06f, 20, 30, true}),
        "long split-stability threats queue");
    oneStep.AdvanceTime(7.94f);
    for (int step = 0; step < 794; ++step) split.AdvanceTime(0.01f);
    const DefenseReport oneStepGuard = oneStep.TryDefend(DefenseInput::Guard);
    const DefenseReport splitGuard = split.TryDefend(DefenseInput::Guard);
    Expect(oneStepGuard.result == DefenseResult::PerfectGuard
            && splitGuard.result == DefenseResult::PerfectGuard,
        "equivalent elapsed time reaches the same perfect-guard boundary");
    Expect(Near(oneStepGuard.timeToImpact, 0.12f)
            && Near(splitGuard.timeToImpact, 0.12f),
        "equivalent elapsed time reports the same quantized impact time");

    ShadowbladeActions fractionalOneStep;
    ShadowbladeActions fractionalSplit;
    Expect(fractionalOneStep.BeginIncomingAttack({1.12001f, 20, 30, true})
            && fractionalSplit.BeginIncomingAttack({1.12001f, 20, 30, true}),
        "fractional-microsecond split-stability threats queue");
    fractionalOneStep.AdvanceTime(1.0f);
    for (int step = 0; step < 60; ++step) {
        fractionalSplit.AdvanceTime(1.0f / 60.0f);
    }
    const DefenseReport fractionalOneStepGuard = fractionalOneStep.TryDefend(DefenseInput::Guard);
    const DefenseReport fractionalSplitGuard = fractionalSplit.TryDefend(DefenseInput::Guard);
    Expect(fractionalOneStepGuard.result == DefenseResult::Guarded
            && fractionalSplitGuard.result == DefenseResult::Guarded,
        "fractional frame splits do not round into a different perfect-guard outcome");

    ShadowbladeActions oneStepExpiry;
    ShadowbladeActions splitExpiry;
    Expect(oneStepExpiry.BeginIncomingAttack({8.06f, 20, 30, true})
            && splitExpiry.BeginIncomingAttack({8.06f, 20, 30, true}),
        "expiry split-stability threats queue");
    oneStepExpiry.AdvanceTime(8.06f);
    for (int step = 0; step < 806; ++step) splitExpiry.AdvanceTime(0.01f);
    Expect(!oneStepExpiry.HasIncomingAttack() && !splitExpiry.HasIncomingAttack(),
        "equivalent elapsed time expires the telegraph on the same boundary");
    Expect(oneStepExpiry.LastDefense().result == DefenseResult::Hit
            && splitExpiry.LastDefense().result == DefenseResult::Hit
            && oneStepExpiry.PlayerHealth() == 80 && splitExpiry.PlayerHealth() == 80,
        "equivalent expiry applies exactly one identical incoming hit");

    ShadowbladeActions fractionalOneStepExpiry;
    ShadowbladeActions fractionalSplitExpiry;
    Expect(fractionalOneStepExpiry.BeginIncomingAttack({1.00001f, 20, 30, true})
            && fractionalSplitExpiry.BeginIncomingAttack({1.00001f, 20, 30, true}),
        "fractional expiry split-stability threats queue");
    fractionalOneStepExpiry.AdvanceTime(1.0f);
    for (int step = 0; step < 60; ++step) {
        fractionalSplitExpiry.AdvanceTime(1.0f / 60.0f);
    }
    Expect(fractionalOneStepExpiry.HasIncomingAttack()
            && fractionalSplitExpiry.HasIncomingAttack(),
        "fractional equivalent elapsed time preserves the same pre-expiry state");
    fractionalOneStepExpiry.AdvanceTime(0.00002f);
    fractionalSplitExpiry.AdvanceTime(0.00002f);
    Expect(!fractionalOneStepExpiry.HasIncomingAttack()
            && !fractionalSplitExpiry.HasIncomingAttack()
            && fractionalOneStepExpiry.LastDefense().result == DefenseResult::Hit
            && fractionalSplitExpiry.LastDefense().result == DefenseResult::Hit,
        "fractional equivalent elapsed time crosses expiry identically");
}

void TestSixtyHzBoundaryToleranceIsSplitStable() {
    using namespace Astral::Scene;

    const float frame = 1.0f / 60.0f;
    const float windup = frame * 122.0f;
    ShadowbladeActions oneStep;
    ShadowbladeActions split;
    Expect(oneStep.BeginIncomingAttack({windup, 20, 30, true})
            && split.BeginIncomingAttack({windup, 20, 30, true}),
        "60 Hz boundary threats queue");
    oneStep.AdvanceTime(windup);
    for (int step = 0; step < 122; ++step) split.AdvanceTime(frame);
    Expect(!oneStep.HasIncomingAttack() && !split.HasIncomingAttack(),
        "ordinary 60 Hz frame splitting reaches the same attack deadline");
    Expect(oneStep.LastDefense().result == DefenseResult::Hit
            && split.LastDefense().result == DefenseResult::Hit
            && oneStep.PlayerHealth() == 80 && split.PlayerHealth() == 80,
        "60 Hz split and single-step expiry apply the same one-time hit");
}

void TestLongSixtyHzBoundaryToleranceIsSplitStable() {
    using namespace Astral::Scene;

    const float frame = 1.0f / 60.0f;
    const float windup = frame * 3842.0f;
    ShadowbladeActions oneStep;
    ShadowbladeActions split;
    Expect(oneStep.BeginIncomingAttack({windup, 20, 30, true})
            && split.BeginIncomingAttack({windup, 20, 30, true}),
        "long 60 Hz boundary threats queue");
    oneStep.AdvanceTime(windup);
    for (int step = 0; step < 3842; ++step) split.AdvanceTime(frame);
    Expect(!oneStep.HasIncomingAttack() && !split.HasIncomingAttack(),
        "long 60 Hz split and one-step timing reach the same deadline");
    Expect(oneStep.LastDefense().result == DefenseResult::Hit
            && split.LastDefense().result == DefenseResult::Hit,
        "long 60 Hz split and one-step timing resolve the same hit");
}

void TestMinimumThreatAndWindowBoundaryPrecision() {
    using namespace Astral::Scene;

    ShadowbladeActions tinyThreat;
    Expect(tinyThreat.BeginIncomingAttack({0.000001f, 20, 30, true}),
        "one-microsecond threat is accepted");
    Expect(tinyThreat.HasIncomingAttack() && tinyThreat.IncomingAttackRemaining() > 0.0f,
        "one-microsecond threat retains positive time immediately after queueing");
    tinyThreat.AdvanceTime(0.0000001f);
    Expect(tinyThreat.HasIncomingAttack(),
        "sub-microsecond progress does not consume a one-microsecond threat early");
    tinyThreat.AdvanceTime(0.0000009f);
    Expect(!tinyThreat.HasIncomingAttack() && tinyThreat.LastDefense().result == DefenseResult::Hit,
        "one-microsecond threat resolves once its full duration elapses");

    ShadowbladeActions outsidePerfectWindow;
    Expect(outsidePerfectWindow.BeginIncomingAttack({0.120001f, 20, 30, true}),
        "just-outside-perfect-window threat queues");
    const DefenseReport ordinaryGuard = outsidePerfectWindow.TryDefend(DefenseInput::Guard);
    Expect(ordinaryGuard.result == DefenseResult::Guarded && !ordinaryGuard.counterGranted,
        "one-microsecond-outside guard remains ordinary instead of becoming perfect");

    ShadowbladeActions counter;
    Expect(counter.BeginIncomingAttack({0.10f, 20, 30, true}),
        "counter-boundary setup threat queues");
    Expect(counter.TryDefend(DefenseInput::Dodge).result == DefenseResult::PerfectDodge
            && counter.HasDefenseCounter(),
        "counter-boundary setup grants a counter");
    counter.AdvanceTime(ShadowbladeActions::DefenseCounterWindowSeconds - 0.000001f);
    Expect(counter.HasDefenseCounter(),
        "defense counter remains active one microsecond before its requested expiry");
    counter.AdvanceTime(0.000002f);
    Expect(!counter.HasDefenseCounter(),
        "defense counter expires after crossing its requested boundary");
}

void TestDefenseClockRebasesAfterSaturation() {
    using namespace Astral::Scene;

    ShadowbladeActions guarded;
    guarded.AdvanceTime(1.0e13f);
    Expect(guarded.BeginIncomingAttack({1.0f, 20, 30, true}),
        "valid threat can be queued after the defense clock reaches saturation");
    Expect(Near(guarded.IncomingAttackRemaining(), 1.0f),
        "rebased threat keeps its full requested windup");
    const DefenseReport immediateGuard = guarded.TryDefend(DefenseInput::Guard);
    Expect(immediateGuard.result == DefenseResult::Guarded
            && !immediateGuard.counterGranted && guarded.GuardIntegrity() == 70,
        "rebased one-second threat is not misclassified as an immediate perfect guard");

    ShadowbladeActions automaticHit;
    automaticHit.AdvanceTime(1.0e13f);
    Expect(automaticHit.BeginIncomingAttack({1.0f, 20, 30, true}),
        "automatic-hit threat queues after saturation");
    automaticHit.AdvanceTime(1.0f);
    Expect(!automaticHit.HasIncomingAttack()
            && automaticHit.LastDefense().result == DefenseResult::Hit
            && automaticHit.PlayerHealth() == 80,
        "rebased threat still expires once after its requested duration");
}

void TestCombatDefenseTrainingBridgeAndCues() {
    using namespace Astral::Scene;

    CombatSandbox combat;
    ShadowbladeActions actions;
    CombatDefenseTraining drill;
    Expect(drill.QueueNextAttack(combat, actions),
        "defense drill links one enemy plan to one Shadowblade threat");
    const EnemyAttackPlan firstPlan = combat.PendingEnemyAttack();
    Expect(firstPlan.pattern == EnemyAttackPattern::QuickCut
            && actions.HasIncomingAttack()
            && Near(actions.IncomingAttackRemaining(), firstPlan.windupSeconds),
        "linked threat preserves the enemy plan windup");
    Expect(!drill.QueueNextAttack(combat, actions),
        "defense drill rejects duplicate linked threats");

    DefenseTrainingCue cue = drill.Cue(combat, actions);
    Expect(cue.phase == DefenseTrainingCuePhase::Approach
            && cue.pattern == EnemyAttackPattern::QuickCut && cue.blockable,
        "fresh linked threat exposes approach cue metadata");

    const DefenseReport early = drill.TryDefend(combat, actions, DefenseInput::Dodge);
    Expect(early.result == DefenseResult::TooEarly && combat.HasPendingEnemyAttack()
            && actions.HasIncomingAttack() && drill.HasLinkedAttack(),
        "too-early dodge preserves both sides of the linked threat");

    Expect(!drill.AdvanceTime(combat, actions, 0.25f),
        "advancing inside a windup does not fabricate a resolution");
    cue = drill.Cue(combat, actions);
    Expect(cue.phase == DefenseTrainingCuePhase::DodgeWindow
            && cue.secondsToImpact > ShadowbladeActions::StandardPerfectDefenseWindowSeconds,
        "cue enters the existing dodge window before the perfect window");
    Expect(!drill.AdvanceTime(combat, actions, 0.20f),
        "advancing to the perfect window keeps the threat live");
    cue = drill.Cue(combat, actions);
    Expect(cue.phase == DefenseTrainingCuePhase::PerfectWindow,
        "cue exposes the existing perfect-defense timing boundary");

    const DefenseReport perfect = drill.TryDefend(combat, actions, DefenseInput::Dodge);
    Expect(perfect.result == DefenseResult::PerfectDodge && !combat.HasPendingEnemyAttack()
            && !actions.HasIncomingAttack() && !drill.HasLinkedAttack(),
        "perfect dodge resolves both the Shadowblade threat and enemy plan exactly once");
    Expect(combat.DefensePunishOpeningReady(),
        "perfect defense round-trip preserves the combat punish opening");
    Expect(drill.Stats().attacksQueued == 1 && drill.Stats().defenseInputs == 2
            && drill.Stats().perfectDefenses == 1
            && drill.Stats().currentPerfectStreak == 1
            && drill.Stats().bestPerfectStreak == 1,
        "defense drill telemetry records early input and one perfect resolution honestly");

    CombatSandbox automaticCombat;
    ShadowbladeActions automaticActions;
    CombatDefenseTraining automaticDrill;
    Expect(automaticDrill.QueueNextAttack(automaticCombat, automaticActions),
        "automatic-impact drill queues a linked attack");
    const EnemyAttackPlan automaticPlan = automaticCombat.PendingEnemyAttack();
    Expect(automaticDrill.AdvanceTime(automaticCombat, automaticActions,
            automaticPlan.windupSeconds),
        "coordinator reconciles an automatically expired Shadowblade threat");
    Expect(!automaticCombat.HasPendingEnemyAttack() && !automaticActions.HasIncomingAttack()
            && automaticActions.PlayerHealth() == 82
            && automaticDrill.Stats().hitsTaken == 1
            && automaticDrill.Stats().damageTaken == 18,
        "automatic impact damages and records exactly once");
    automaticDrill.AdvanceTime(automaticCombat, automaticActions, 1.0f);
    Expect(automaticActions.PlayerHealth() == 82
            && automaticDrill.Stats().hitsTaken == 1,
        "post-impact time cannot duplicate player damage or telemetry");

    CombatSandbox bossCombat;
    ShadowbladeActions bossActions;
    CombatDefenseTraining bossDrill;
    Expect(bossCombat.SetTrainingEnemyProfile(TrainingEnemyProfile::Boss)
            && bossCombat.SetBossPracticePhase(EnemyPhase::Pressure),
        "boss cue setup selects pressure phase");
    Expect(bossDrill.QueueNextAttack(bossCombat, bossActions),
        "pressure drill queues first boss attack");
    const float firstRecovery = bossCombat.PendingEnemyAttack().recoverySeconds;
    Expect(bossDrill.TryDefend(bossCombat, bossActions, DefenseInput::Guard).result
            == DefenseResult::Guarded,
        "first pressure attack can be resolved to reach the next pattern");
    bossDrill.AdvanceTime(bossCombat, bossActions, firstRecovery);
    Expect(bossDrill.QueueNextAttack(bossCombat, bossActions)
            && bossCombat.PendingEnemyAttack().pattern == EnemyAttackPattern::RiftBurst,
        "second pressure pattern reaches the unblockable RiftBurst");
    const DefenseTrainingCue unblockableCue = bossDrill.Cue(bossCombat, bossActions);
    Expect(unblockableCue.phase == DefenseTrainingCuePhase::Approach
            && !unblockableCue.blockable
            && unblockableCue.pattern == EnemyAttackPattern::RiftBurst,
        "cue metadata exposes an unblockable plan without inventing a copied indicator");
}

void TestCombatDefenseTrainingInterruptionAndGrades() {
    using namespace Astral::Scene;

    CombatSandbox interruptedCombat;
    ShadowbladeActions interruptedActions;
    CombatDefenseTraining interruptedDrill;
    Expect(interruptedDrill.QueueNextAttack(interruptedCombat, interruptedActions),
        "interruption setup queues a linked QuickCut");
    const AttackReport light = interruptedCombat.TryAttack(AttackType::Light, {});
    Expect(light.result == AttackResult::Hit && !light.staggerTriggered,
        "interruption setup light attack builds posture");
    Expect(!interruptedDrill.AdvanceTime(interruptedCombat, interruptedActions, 0.4f),
        "linked threat remains live before the follow-up stagger");
    const AttackReport heavy = interruptedCombat.TryAttack(AttackType::Heavy, {});
    Expect(heavy.result == AttackResult::Hit && heavy.staggerTriggered
            && !interruptedCombat.HasPendingEnemyAttack()
            && interruptedActions.HasIncomingAttack(),
        "combat stagger interrupts its planner before coordinator reconciliation");
    Expect(interruptedDrill.AdvanceTime(interruptedCombat, interruptedActions, 0.01f)
            && !interruptedActions.HasIncomingAttack()
            && !interruptedDrill.HasLinkedAttack(),
        "coordinator cancels the stale linked Shadowblade threat after interruption");
    Expect(interruptedActions.PlayerHealth() == 100
            && interruptedDrill.Stats().interruptions == 1,
        "stagger interruption cannot land stale player damage");
    interruptedDrill.AdvanceTime(interruptedCombat, interruptedActions, 1.0f);
    Expect(interruptedActions.PlayerHealth() == 100,
        "canceled linked threat remains canceled after its former impact time");

    CombatSandbox perfectCombat;
    ShadowbladeActions perfectActions;
    CombatDefenseTraining perfectDrill;
    for (int attempt = 0; attempt < 3; ++attempt) {
        Expect(perfectDrill.QueueNextAttack(perfectCombat, perfectActions),
            "grade drill queues each perfect-defense attempt");
        const EnemyAttackPlan plan = perfectCombat.PendingEnemyAttack();
        perfectDrill.AdvanceTime(perfectCombat, perfectActions, 0.45f);
        const DefenseReport perfect = perfectDrill.TryDefend(
            perfectCombat, perfectActions, DefenseInput::Dodge);
        Expect(perfect.result == DefenseResult::PerfectDodge,
            "grade drill records a clean perfect dodge");
        if (attempt < 2) {
            perfectDrill.AdvanceTime(perfectCombat, perfectActions, plan.recoverySeconds);
        }
    }
    Expect(perfectDrill.Grade() == DefenseTrainingGrade::Gold
            && perfectDrill.Stats().perfectDefenses == 3
            && perfectDrill.Stats().bestPerfectStreak == 3
            && perfectDrill.Stats().damageTaken == 0,
        "three clean perfect resolutions earn deterministic Gold drill grade");

    const int healthBeforeReset = perfectActions.PlayerHealth();
    const float recoveryBeforeReset = perfectCombat.EnemyAttackReadyInSeconds();
    perfectDrill.ResetStats();
    Expect(perfectDrill.Grade() == DefenseTrainingGrade::None
            && perfectDrill.Stats().attacksQueued == 0
            && perfectActions.PlayerHealth() == healthBeforeReset
            && Near(perfectCombat.EnemyAttackReadyInSeconds(), recoveryBeforeReset),
        "telemetry reset does not mutate player or combat state");

    CombatSandbox hitCombat;
    ShadowbladeActions hitActions;
    CombatDefenseTraining hitDrill;
    for (int attempt = 0; attempt < 3; ++attempt) {
        Expect(hitDrill.QueueNextAttack(hitCombat, hitActions),
            "damage-grade drill queues each automatic hit");
        const EnemyAttackPlan plan = hitCombat.PendingEnemyAttack();
        hitDrill.AdvanceTime(hitCombat, hitActions, plan.windupSeconds);
        if (attempt < 2) {
            hitDrill.AdvanceTime(hitCombat, hitActions, plan.recoverySeconds);
        }
    }
    Expect(hitDrill.Grade() == DefenseTrainingGrade::Bronze
            && hitDrill.Stats().hitsTaken == 3
            && hitDrill.Stats().damageTaken == 54,
        "repeated damage produces a lower bounded drill grade");

    CombatSandbox invalidCombat;
    ShadowbladeActions invalidActions;
    CombatDefenseTraining invalidDrill;
    Expect(invalidDrill.QueueNextAttack(invalidCombat, invalidActions),
        "invalid-delta drill queues a linked attack");
    const float remaining = invalidActions.IncomingAttackRemaining();
    const DefenseTrainingStats before = invalidDrill.Stats();
    Expect(!invalidDrill.AdvanceTime(invalidCombat, invalidActions,
            std::numeric_limits<float>::quiet_NaN()),
        "nonfinite coordinator delta does not resolve a linked attack");
    Expect(invalidDrill.Stats().attacksQueued == before.attacksQueued
            && invalidDrill.Stats().hitsTaken == before.hitsTaken
            && Near(invalidActions.IncomingAttackRemaining(), remaining),
        "nonfinite coordinator delta preserves telemetry and threat timing");
}
}

int main() {
    TestDashCostCooldownAndRegenerationCap();
    TestDirectionalDashNormalizationAndFallback();
    TestInvalidDeltaDoesNotMutateState();
    TestFatalStrikeRangeDamageAndCooldown();
    TestMixedAttackComboIncludesFatalStrike();
    TestStaggerFollowUpCostAndConsumption();
    TestGuardConflicts();
    TestInsufficientResourceRejection();
    TestThreatDodgeAndDefenseCounter();
    TestGuardIntegrityBreakAndUnblockableHit();
    TestPerfectGuardTimingPresetAndAutomaticHit();
    TestDefenseCounterExpiryIsSplitStable();
    TestIncomingAttackTimingIsSplitStable();
    TestSixtyHzBoundaryToleranceIsSplitStable();
    TestLongSixtyHzBoundaryToleranceIsSplitStable();
    TestMinimumThreatAndWindowBoundaryPrecision();
    TestDefenseClockRebasesAfterSaturation();
    TestCombatDefenseTrainingBridgeAndCues();
    TestCombatDefenseTrainingInterruptionAndGrades();
    if (failures != 0) return 1;
    std::cout << "Shadowblade action tests passed\n";
    return 0;
}