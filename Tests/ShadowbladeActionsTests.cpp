#include "Engine/Scene/ShadowbladeActions.h"

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
    if (failures != 0) return 1;
    std::cout << "Shadowblade action tests passed\n";
    return 0;
}
