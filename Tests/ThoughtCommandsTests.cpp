#include "Engine/Scene/ThoughtCommands.h"
#include "Engine/Scene/DefensePracticeSession.h"
#include "Engine/Scene/ExplorationFieldGuide.h"
#include "Engine/Scene/LandmarkInteraction.h"
#include "Engine/Scene/ManaReactorExpedition.h"
#include "Engine/Scene/ShadowCryptExpedition.h"
#include "Engine/Scene/ShadowbladeLoadout.h"
#include "Engine/Scene/ShadowbladeTrainingPath.h"

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

void TestNormalizationAndGrammar() {
    using Astral::Scene::ThoughtCommandReason;
    using Astral::Scene::ThoughtCommands;
    using Astral::Scene::ThoughtCommandType;
    const auto dash = ThoughtCommands::Parse("  DaSh\t");
    Expect(dash.type == ThoughtCommandType::Dash && dash.normalized == "dash",
        "dash is trimmed and case normalized");
    Expect(ThoughtCommands::Parse("FATAL").type == ThoughtCommandType::Fatal,
        "fatal is case insensitive");
    Expect(ThoughtCommands::Parse(" guard   ON ").type == ThoughtCommandType::GuardOn,
        "guard on collapses whitespace");
    Expect(ThoughtCommands::Parse("GUARD off").type == ThoughtCommandType::GuardOff,
        "guard off is supported");
    Expect(ThoughtCommands::Parse("Focus").type == ThoughtCommandType::Focus,
        "focus is supported");
    Expect(ThoughtCommands::Parse(" \t\n").reason == ThoughtCommandReason::Empty,
        "empty input has a deterministic reason");
    Expect(ThoughtCommands::Parse("dance").reason == ThoughtCommandReason::Unsupported,
        "unsupported input has a deterministic reason");
    Expect(ThoughtCommands::Parse("dash fatal").reason == ThoughtCommandReason::Ambiguous,
        "multiple recognized commands are ambiguous");
}

void TestSupportedCommandsAndNoOps() {
    using namespace Astral::Scene;
    ThoughtCommands commands;
    ShadowbladeActions actions;
    CombatSandbox combat;

    const auto dash = commands.Submit("dash", {}, actions, combat);
    Expect(dash.status == ThoughtCommandStatus::Accepted
            && dash.shadowAction.result == ShadowActionResult::Activated,
        "dash delegates to Shadowblade activation");
    Expect(Near(actions.Resource(), 75.0f), "delegated dash consumes M7 resource");

    actions.AdvanceTime(1.0f);
    const auto fatal = commands.Submit("fatal", {}, actions, combat);
    Expect(fatal.status == ThoughtCommandStatus::Accepted
            && fatal.shadowAction.damageApplied == 80 && combat.Dummy().health == 20,
        "fatal delegates damage to existing Shadowblade and combat domains");

    const auto guardOn = commands.Submit("guard on", {}, actions, combat);
    Expect(guardOn.status == ThoughtCommandStatus::Accepted && actions.IsGuarding()
            && commands.IsCommandGuardActive(),
        "guard on commands held guard state");
    const auto repeatedOn = commands.Submit("guard on", {}, actions, combat);
    Expect(repeatedOn.status == ThoughtCommandStatus::Rejected
            && repeatedOn.reason == ThoughtCommandReason::NoOp && actions.IsGuarding(),
        "repeated guard on is a non-mutating no-op");
    const auto guardOff = commands.Submit("guard off", {}, actions, combat);
    Expect(guardOff.status == ThoughtCommandStatus::Accepted && !actions.IsGuarding(),
        "guard off releases commanded guard state");
    const auto repeatedOff = commands.Submit("guard off", {}, actions, combat);
    Expect(repeatedOff.status == ThoughtCommandStatus::Rejected
            && repeatedOff.reason == ThoughtCommandReason::NoOp,
        "repeated guard off is a non-mutating no-op");
}

void TestParserRejectionsDoNotMutateGameplay() {
    using namespace Astral::Scene;
    ThoughtCommands commands;
    ShadowbladeActions actions;
    CombatSandbox combat;
    const float resource = actions.Resource();
    const int health = combat.Dummy().health;
    for (const char* input : {"", "dance", "dash fatal"}) {
        const auto report = commands.Submit(input, {}, actions, combat);
        Expect(report.status == ThoughtCommandStatus::Rejected,
            "invalid parser result is rejected");
        Expect(Near(actions.Resource(), resource) && combat.Dummy().health == health
                && !commands.IsFocusActive() && !commands.IsCommandGuardActive(),
            "parser rejection causes no gameplay mutation");
    }
}

void TestDelegatedRejectionGates() {
    using namespace Astral::Scene;
    ThoughtCommands commands;
    ShadowbladeActions actions;
    CombatSandbox combat;

    commands.Submit("guard on", {}, actions, combat);
    const auto guarded = commands.Submit("dash", {}, actions, combat);
    Expect(guarded.reason == ThoughtCommandReason::GuardedConflict
            && Near(actions.Resource(), 100.0f),
        "command dash preserves the existing guard conflict gate");
    commands.Submit("guard off", {}, actions, combat);

    commands.Submit("dash", {}, actions, combat);
    const auto cooldown = commands.Submit("dash", {}, actions, combat);
    Expect(cooldown.reason == ThoughtCommandReason::Cooldown
            && Near(actions.Resource(), 75.0f),
        "command dash preserves cooldown and rejection cost rules");

    const auto range = commands.Submit("fatal", {-10.0f, 0.0f, 0.0f}, actions, combat);
    Expect(range.reason == ThoughtCommandReason::OutOfRange && combat.Dummy().health == 100,
        "command fatal preserves the existing range gate");

    ThoughtCommands resourceCommands;
    ShadowbladeActions resourceActions;
    CombatSandbox resourceCombat;
    for (int activation = 0; activation < 6; ++activation) {
        if (activation > 0) resourceActions.AdvanceTime(1.0f);
        resourceCommands.Submit("dash", {}, resourceActions, resourceCombat);
    }
    const auto resource = resourceCommands.Submit(
        "fatal", {}, resourceActions, resourceCombat);
    Expect(resource.reason == ThoughtCommandReason::InsufficientResource,
        "command fatal preserves the existing resource gate");
}

void TestFocusBoundsAndLocalDelta() {
    using namespace Astral::Scene;
    ThoughtCommands commands;
    ShadowbladeActions actions;
    CombatSandbox combat;
    Expect(Near(commands.TimeMultiplier(), ThoughtCommands::NormalTimeMultiplier),
        "focus starts at normal local time");
    auto focus = commands.Submit("focus", {}, actions, combat);
    Expect(focus.status == ThoughtCommandStatus::Accepted && commands.IsFocusActive()
            && Near(commands.TimeMultiplier(), ThoughtCommands::FocusTimeMultiplier),
        "focus toggles to its bounded multiplier");
    Expect(commands.TimeMultiplier() > 0.0f && commands.TimeMultiplier() <= 1.0f
            && Near(commands.ScaleDelta(2.0f), 0.7f),
        "focused local delta is positive and bounded");
    Expect(Near(commands.ScaleDelta(-1.0f), 0.0f)
            && Near(commands.ScaleDelta(std::numeric_limits<float>::infinity()), 0.0f),
        "invalid local deltas cannot mutate simulation");
    commands.Submit("focus", {}, actions, combat);
    Expect(!commands.IsFocusActive()
            && Near(commands.TimeMultiplier(), ThoughtCommands::NormalTimeMultiplier),
        "focus toggles deterministically back to normal");
}

void TestPhysicalAndCommandGuardComposition() {
    using namespace Astral::Scene;
    ThoughtCommands commands;
    ShadowbladeActions actions;
    CombatSandbox combat;
    commands.ApplyGuardState(true, actions);
    Expect(actions.IsGuarding(), "physical M7 guard remains supported");
    commands.Submit("guard on", {}, actions, combat);
    commands.ApplyGuardState(false, actions);
    Expect(actions.IsGuarding(), "command guard remains held after physical release");
    commands.Submit("guard off", {}, actions, combat);
    commands.ApplyGuardState(false, actions);
    Expect(!actions.IsGuarding(), "effective guard releases when both sources are off");
}
}

#include "DefensePracticePass13Tests.inc"
#include "ExplorationFieldGuidePass14Tests.inc"
#include "ShadowCryptExpeditionPass15Tests.inc"
#include "ShadowbladeLoadoutPass16Tests.inc"
#include "ShadowbladeLoadoutEffectsPass17Tests.inc"
#include "ShadowbladeTuningPass18Tests.inc"
#include "ShadowbladeCombatFlowPass19Tests.inc"
#include "LandmarkDialogueFlowPass20Tests.inc"
#include "ManaReactorExpeditionPass21Tests.inc"
#include "ManaReactorStrategyPass22Tests.inc"
#include "ShadowbladeTrainingPathPass23Tests.inc"
#include "ShadowbladeTrainingAssignmentPass23Tests.inc"

int main() {
    TestNormalizationAndGrammar();
    TestSupportedCommandsAndNoOps();
    TestParserRejectionsDoNotMutateGameplay();
    TestDelegatedRejectionGates();
    TestFocusBoundsAndLocalDelta();
    TestPhysicalAndCommandGuardComposition();
    TestDefensePracticePass13();
    TestExplorationFieldGuidePass14();
    TestShadowCryptExpeditionPass15();
    TestShadowbladeLoadoutPass16();
    TestShadowbladeLoadoutEffectsPass17();
    TestShadowbladeTuningPass18();
    TestShadowbladeCombatFlowPass19();
    TestLandmarkDialogueFlowPass20();
    TestManaReactorExpeditionPass21();
    TestManaReactorStrategyPass22();
    TestShadowbladeTrainingPathPass23();
    TestShadowbladeTrainingAssignmentPass23();
    if (failures != 0) return 1;
    std::cout << "Thought command tests passed\n";
    return 0;
}
