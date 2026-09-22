#include "Engine/Scene/ShadowbladeActions.h"

#include <cmath>
#include <iostream>
#include <limits>

namespace {
int failures = 0;
void Expect(bool condition, const char* message) { if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; } }
bool Near(float actual, float expected) { return std::fabs(actual - expected) < 0.001f; }

void TestDashCostCooldownAndRegenerationCap() {
    Astral::Scene::ShadowbladeActions actions;
    const auto activated = actions.TryDash({2.0f, 4.0f, 0.0f});
    Expect(activated.result == Astral::Scene::ShadowActionResult::Activated, "dash activates when ready");
    Expect(Near(activated.dashDestination.x, 2.0f) && Near(activated.dashDestination.y, 10.0f), "neutral dash direction preserves deterministic forward fallback");
    Expect(Near(activated.resourceSpent, Astral::Scene::ShadowbladeActions::DashCost), "dash report records its resource spend");
    Expect(Near(actions.Resource(), 75.0f), "dash consumes its defined resource cost");
    Expect(Near(actions.DashCooldownRemaining(), 1.0f), "dash starts its cooldown");
    const auto rejected = actions.TryDash({});
    Expect(rejected.result == Astral::Scene::ShadowActionResult::Cooldown, "dash rejects activation during cooldown");
    actions.AdvanceTime(0.5f);
    Expect(Near(actions.Resource(), 82.5f) && Near(actions.DashCooldownRemaining(), 0.5f), "positive delta advances dash state");
    actions.AdvanceTime(100.0f);
    Expect(Near(actions.Resource(), 100.0f) && Near(actions.DashCooldownRemaining(), 0.0f), "resource and cooldown clamp");
}

void TestDirectionalDashNormalizationAndFallback() {
    using namespace Astral::Scene;
    ShadowbladeActions cardinal;
    const auto right = cardinal.TryDash({2.0f, 4.0f, 0.0f}, {1.0f, 0.0f, 0.0f});
    Expect(right.result == ShadowActionResult::Activated && Near(right.dashDestination.x, 8.0f) && Near(right.dashDestination.y, 4.0f), "cardinal dash follows direction");
    ShadowbladeActions diagonal;
    const auto diagonalReport = diagonal.TryDash({}, {1.0f, 1.0f, 0.0f});
    const float diagonalDistance = std::sqrt(diagonalReport.dashDestination.x * diagonalReport.dashDestination.x + diagonalReport.dashDestination.y * diagonalReport.dashDestination.y);
    Expect(diagonalReport.result == ShadowActionResult::Activated && Near(diagonalDistance, ShadowbladeActions::DashDistance), "diagonal dash keeps distance");
    ShadowbladeActions invalid;
    const float infinity = std::numeric_limits<float>::infinity();
    const auto fallback = invalid.TryDash({1.0f, 2.0f, 0.0f}, {infinity, 1.0f, 0.0f});
    Expect(fallback.result == ShadowActionResult::Activated && Near(fallback.dashDestination.x, 1.0f) && Near(fallback.dashDestination.y, 8.0f), "nonfinite direction falls back safely");
}

void TestInvalidDeltaDoesNotMutateState() {
    Astral::Scene::ShadowbladeActions actions;
    actions.TryDash({});
    const float resource = actions.Resource(); const float cooldown = actions.DashCooldownRemaining();
    actions.AdvanceTime(0.0f); actions.AdvanceTime(-1.0f); actions.AdvanceTime(std::numeric_limits<float>::quiet_NaN()); actions.AdvanceTime(std::numeric_limits<float>::infinity());
    Expect(Near(actions.Resource(), resource) && Near(actions.DashCooldownRemaining(), cooldown), "invalid deltas do not mutate action state");
}

void TestFatalStrikeRangeDamageAndCooldown() {
    using namespace Astral::Scene;
    ShadowbladeActions actions; CombatSandbox combat;
    const auto outOfRange = actions.TryFatalStrike({-10.0f, 0.0f, 0.0f}, combat);
    Expect(outOfRange.result == ShadowActionResult::OutOfRange && combat.Dummy().health == 100 && Near(actions.Resource(), 100.0f), "out-of-range fatal is free");
    const auto hit = actions.TryFatalStrike({0.0f, 0.0f, 0.0f}, combat);
    Expect(hit.result == ShadowActionResult::Activated && hit.damageApplied == 80 && combat.Dummy().health == 20, "fatal strike hits");
    Expect(combat.ComboCount() == 1 && combat.Stats().bestCombo == 1, "fatal strike joins shared combo");
    Expect(Near(actions.Resource(), 50.0f) && Near(actions.FatalStrikeCooldownRemaining(), 2.0f), "fatal strike cost/cooldown");
    Expect(actions.TryFatalStrike({0.0f, 0.0f, 0.0f}, combat).result == ShadowActionResult::Cooldown, "fatal cooldown rejects");
    actions.AdvanceTime(2.0f); combat.AdvanceTime(2.0f);
    const auto defeat = actions.TryFatalStrike({0.0f, 0.0f, 0.0f}, combat);
    Expect(defeat.damageApplied == 20 && combat.Dummy().IsDefeated(), "fatal caps to remaining health");
}

void TestMixedAttackComboIncludesFatalStrike() {
    using namespace Astral::Scene;
    CombatSandbox combat; ShadowbladeActions actions;
    Expect(combat.TryAttack(AttackType::Light, {0.0f,0.0f,0.0f}).result == AttackResult::Hit && combat.ComboCount() == 1, "light starts combo");
    combat.AdvanceTime(0.4f);
    const auto fatal = actions.TryFatalStrike({0.0f,0.0f,0.0f}, combat);
    Expect(fatal.result == ShadowActionResult::Activated && fatal.damageApplied == 75, "fatal succeeds in combo window");
    Expect(combat.ComboCount() == 2 && combat.Stats().bestCombo == 2 && combat.Stats().hitCount == 2, "light plus fatal is a two-hit combo");
}

void TestStaggerFollowUpCostAndConsumption() {
    using namespace Astral::Scene;
    CombatSandbox combat; ShadowbladeActions actions;
    combat.TryAttack(AttackType::Light, {0,0,0}); combat.AdvanceTime(0.4f);
    const auto heavy = combat.TryAttack(AttackType::Heavy, {0,0,0});
    Expect(heavy.staggerTriggered && combat.IsStaggered(), "light-heavy creates stagger");
    const auto out = actions.TryFatalStrike({-10,0,0}, combat);
    Expect(out.result == ShadowActionResult::OutOfRange && combat.IsStaggered(), "failed follow-up preserves opening");
    const auto follow = actions.TryFatalStrike({0,0,0}, combat);
    Expect(follow.result == ShadowActionResult::Activated && follow.followUp && Near(follow.resourceSpent, ShadowbladeActions::StaggerFollowUpCost), "stagger follow-up activates");
    Expect(!combat.IsStaggered() && follow.damageApplied == 15 && combat.Dummy().IsDefeated(), "follow-up consumes opening and caps damage");
    Expect(combat.ComboCount() == 3 && combat.Stats().bestCombo == 3, "follow-up extends combo");
}

void TestGuardConflicts() {
    using namespace Astral::Scene;
    ShadowbladeActions actions; CombatSandbox combat; actions.SetGuarding(true);
    Expect(actions.TryDash({}).result == ShadowActionResult::GuardedConflict, "guard blocks dash");
    Expect(actions.TryFatalStrike({}, combat).result == ShadowActionResult::GuardedConflict, "guard blocks fatal");
    actions.SetGuarding(false); Expect(actions.TryDash({}).result == ShadowActionResult::Activated, "release permits dash");
}

void TestInsufficientResourceRejection() {
    using namespace Astral::Scene;
    ShadowbladeActions actions;
    for (int i=0;i<8;++i) { Expect(actions.TryDash({}).result == ShadowActionResult::Activated, "setup dash activates"); actions.AdvanceTime(1.0f); }
    Expect(Near(actions.Resource(),20.0f) && actions.TryDash({}).result == ShadowActionResult::InsufficientResource, "dash insufficient resource");
    CombatSandbox combat; Expect(actions.TryFatalStrike({},combat).result == ShadowActionResult::InsufficientResource && combat.Dummy().health == 100, "fatal insufficient resource");
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
