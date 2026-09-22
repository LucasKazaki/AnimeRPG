#include "Engine/Scene/LandmarkInteraction.h"

#include <cmath>
#include <iostream>

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

void TestDeterministicProximitySelection() {
    using namespace Astral::Scene;
    WorldBlockout world;
    LandmarkInteraction interaction;
    Expect(!interaction.HasSelection() && interaction.VisitedCount() == 0,
        "ledger begins empty with no selection");
    Expect(interaction.UpdateSelection({-2.0f, 11.5f, 0.0f}, world),
        "entering a landmark radius changes selection");
    Expect(interaction.HasSelection()
            && interaction.SelectedKind() == LandmarkKind::LincolnMemorial,
        "Lincoln footprint proximity is selected deterministically");
    Expect(!interaction.UpdateSelection({-8.0f, 18.0f, 0.0f}, world),
        "remaining in the same landmark does not report a selection change");
    Expect(interaction.UpdateSelection({-2.0f, 11.4f, 0.0f}, world)
            && !interaction.HasSelection(),
        "moving beyond the inclusive proximity boundary clears selection");
}

void TestBoundedDiscoveryLedgerAndRepeatSafety() {
    using namespace Astral::Scene;
    WorldBlockout world;
    LandmarkInteraction interaction;
    ShadowbladeActions actions;
    const auto outOfRange = interaction.TryInteract({}, world, actions);
    Expect(outOfRange.result == LandmarkInteractionResult::OutOfRange
            && interaction.VisitedCount() == 0,
        "out-of-range interaction does not mutate the ledger");

    const Astral::Math::Vec3 positions[3]{{-8.0f, 18.0f, 0.0f}, {4.0f, 39.0f, 0.0f},
        {5.0f, 68.0f, 0.0f}};
    for (const Astral::Math::Vec3& position : positions) {
        const auto report = interaction.TryInteract(position, world, actions);
        Expect(report.result == LandmarkInteractionResult::Discovered,
            "each landmark can be discovered exactly once");
    }
    Expect(interaction.VisitedCount() == LandmarkInteraction::LedgerCapacity,
        "the fixed ledger is bounded to the three world landmarks");
    Expect(interaction.IsVisited(LandmarkKind::LincolnMemorial)
            && interaction.IsVisited(LandmarkKind::ReflectingPool)
            && interaction.IsVisited(LandmarkKind::WashingtonMonument),
        "all bounded ledger entries remain visible");
    const auto repeated = interaction.TryInteract(positions[2], world, actions);
    Expect(repeated.result == LandmarkInteractionResult::AlreadyVisited
            && interaction.VisitedCount() == LandmarkInteraction::LedgerCapacity,
        "repeat interaction is deterministic and cannot grow the ledger");
}

void TestOrderedObjectiveGuidanceKeepsFreeDiscovery() {
    using namespace Astral::Scene;
    WorldBlockout world;
    LandmarkInteraction interaction;
    ShadowbladeActions actions;

    Expect(interaction.ObjectiveProgress() == 0
            && interaction.CurrentObjective() == LandmarkObjectiveStage::DiscoverLincoln,
        "objective begins at Lincoln regardless of free exploration");

    const auto monument = interaction.TryInteract({5.0f, 68.0f, 0.0f}, world, actions);
    Expect(monument.result == LandmarkInteractionResult::Discovered
            && interaction.IsVisited(LandmarkKind::WashingtonMonument),
        "out-of-order landmark discovery is retained");
    Expect(interaction.ObjectiveProgress() == 0
            && interaction.CurrentObjective() == LandmarkObjectiveStage::DiscoverLincoln,
        "out-of-order discovery does not skip the missing prerequisite objective");

    const auto lincoln = interaction.TryInteract({-8.0f, 18.0f, 0.0f}, world, actions);
    Expect(lincoln.result == LandmarkInteractionResult::Discovered
            && interaction.ObjectiveProgress() == 1
            && interaction.CurrentObjective() == LandmarkObjectiveStage::DiscoverReflectingPool,
        "discovering Lincoln advances guidance to the Reflecting Pool");

    const auto pool = interaction.TryInteract({4.0f, 39.0f, 0.0f}, world, actions);
    Expect(pool.result == LandmarkInteractionResult::Discovered
            && interaction.ObjectiveProgress() == LandmarkInteraction::LedgerCapacity
            && interaction.ObjectiveComplete(),
        "completing the missing ordered prefix counts an already-discovered later landmark");

    const auto repeated = interaction.TryInteract({5.0f, 68.0f, 0.0f}, world, actions);
    Expect(repeated.result == LandmarkInteractionResult::AlreadyVisited
            && interaction.ObjectiveComplete(),
        "repeat interactions cannot regress or over-advance completed objectives");
}

void TestRewardRoutesThroughResourceRules() {
    using namespace Astral::Scene;
    WorldBlockout world;
    LandmarkInteraction interaction;
    ShadowbladeActions actions;
    actions.TryDash({});
    Expect(Near(actions.Resource(), 75.0f), "reward setup uses the existing dash cost");
    const auto rewarded = interaction.TryInteract({-8.0f, 18.0f, 0.0f}, world, actions);
    Expect(rewarded.result == LandmarkInteractionResult::Discovered
            && Near(rewarded.rewardApplied, 20.0f) && Near(actions.Resource(), 95.0f),
        "Lincoln discovery restores through the capped Shadowblade resource API");
    const auto repeated = interaction.TryInteract({-8.0f, 18.0f, 0.0f}, world, actions);
    Expect(repeated.result == LandmarkInteractionResult::AlreadyVisited
            && Near(repeated.rewardApplied, 0.0f) && Near(actions.Resource(), 95.0f),
        "the discovery reward cannot be farmed");

    ShadowbladeActions cappedActions;
    LandmarkInteraction cappedInteraction;
    const auto capped = cappedInteraction.TryInteract(
        {-8.0f, 18.0f, 0.0f}, world, cappedActions);
    Expect(Near(capped.rewardApplied, 0.0f)
            && Near(cappedActions.Resource(), ShadowbladeActions::MaximumResource),
        "reward obeys the existing maximum-resource cap");
    Expect(Near(cappedActions.RestoreResource(-5.0f), 0.0f),
        "invalid restoration cannot mutate player resource");
}
}

int main() {
    TestDeterministicProximitySelection();
    TestBoundedDiscoveryLedgerAndRepeatSafety();
    TestOrderedObjectiveGuidanceKeepsFreeDiscovery();
    TestRewardRoutesThroughResourceRules();
    if (failures != 0) return 1;
    std::cout << "Landmark interaction tests passed\n";
    return 0;
}
