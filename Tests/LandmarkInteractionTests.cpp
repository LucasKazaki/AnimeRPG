#include "Engine/Scene/EncounterChallenge.h"
#include "Engine/Scene/LandmarkInteraction.h"

#include <cmath>
#include <cstdint>
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

void TestEncounterChallengeScoringRanksAndFirstClears() {
    using namespace Astral::Scene;

    EncounterChallengeTracker disabled;
    const auto disabledResult = disabled.Resolve(
        {1000, 1, 1, 1, true, EncounterTimeGrade::Gold});
    Expect(disabledResult.score == 0
            && disabledResult.rank == EncounterChallengeRank::None
            && !disabledResult.firstClear,
        "challenge tracker remains inert until explicitly configured");

    EncounterChallengeTracker standard;
    standard.Configure(EncounterChallengeDifficulty::Standard,
        EncounterTacticalFocus::Reaction);
    const auto standardResult = standard.Resolve(
        {1000, 2, 1, 0, true, EncounterTimeGrade::Gold});
    Expect(standardResult.flawlessGoal && standardResult.techniqueVarietyGoal
            && standardResult.speedGoal && standardResult.tacticalGoal
            && standardResult.sideGoalsCompleted == 4,
        "configured Standard challenge records all four independent side goals");
    Expect(standardResult.rank == EncounterChallengeRank::Gold
            && standardResult.score == 1680,
        "Standard challenge combines base, tactical, and side-goal scoring deterministically");
    Expect(standardResult.firstClear
            && Near(standardResult.firstClearRewardRequested,
                EncounterChallengeTracker::StandardFirstClearReward),
        "first Standard clear requests its one-time reward");
    const auto repeatStandard = standard.Resolve(
        {1000, 2, 1, 0, true, EncounterTimeGrade::Gold});
    Expect(!repeatStandard.firstClear
            && Near(repeatStandard.firstClearRewardRequested, 0.0f),
        "repeating Standard cannot request the first-clear reward twice");

    standard.Configure(EncounterChallengeDifficulty::Expert,
        EncounterTacticalFocus::Finisher);
    const auto expert = standard.Resolve(
        {800, 0, 1, 2, false, EncounterTimeGrade::Silver});
    Expect(expert.sideGoalsCompleted == 3
            && expert.rank == EncounterChallengeRank::Silver
            && expert.firstClear
            && standard.FirstClearGranted(EncounterChallengeDifficulty::Expert),
        "Expert uses its stricter thresholds and an independent first-clear ledger entry");

    standard.Configure(EncounterChallengeDifficulty::Apex,
        EncounterTacticalFocus::Balanced);
    const auto apexNoRank = standard.Resolve(
        {0, 1, 1, 0, false, EncounterTimeGrade::Bronze});
    Expect(apexNoRank.sideGoalsCompleted == 2
            && apexNoRank.rank == EncounterChallengeRank::None,
        "Apex requires more than two side goals before awarding a rank");
    const auto apexBronze = standard.Resolve(
        {0, 1, 0, 0, true, EncounterTimeGrade::Silver});
    Expect(apexBronze.sideGoalsCompleted == 3
            && apexBronze.rank == EncounterChallengeRank::Bronze,
        "Apex awards Bronze at three side goals, stricter than Expert");
    const auto apexSilver = standard.Resolve(
        {900, 1, 1, 1, true, EncounterTimeGrade::Silver});
    Expect(apexSilver.sideGoalsCompleted == 4
            && apexSilver.rank == EncounterChallengeRank::Silver,
        "Apex with all side goals still requires Gold time for Gold rank");
    const auto apexGold = standard.Resolve(
        {900, 1, 1, 1, true, EncounterTimeGrade::Gold});
    Expect(apexGold.rank == EncounterChallengeRank::Gold,
        "Apex Gold requires both all side goals and Gold clear time");

    EncounterChallengeTracker balancedMode;
    balancedMode.Configure(EncounterChallengeDifficulty::Standard,
        EncounterTacticalFocus::Balanced, EncounterScoringMode::Balanced);
    const auto balancedScore = balancedMode.Resolve(
        {2000, 1, 1, 1, false, EncounterTimeGrade::Bronze});

    EncounterChallengeTracker techniqueFirst;
    techniqueFirst.Configure(EncounterChallengeDifficulty::Standard,
        EncounterTacticalFocus::Balanced, EncounterScoringMode::TechniqueFirst);
    const auto techniqueScore = techniqueFirst.Resolve(
        {2000, 1, 1, 1, false, EncounterTimeGrade::Bronze});
    Expect(techniqueScore.sideGoalsCompleted == balancedScore.sideGoalsCompleted
            && techniqueScore.score == 1900
            && techniqueScore.score != balancedScore.score,
        "TechniqueFirst changes score weighting without changing objective completion");

    EncounterChallengeTracker noTechnique;
    noTechnique.Configure(EncounterChallengeDifficulty::Standard,
        EncounterTacticalFocus::Reaction, EncounterScoringMode::TechniqueFirst);
    const auto noTechniqueResult = noTechnique.Resolve(
        {2000, 0, 0, 0, false, EncounterTimeGrade::Bronze});
    Expect(!noTechniqueResult.tacticalGoal
            && noTechniqueResult.sideGoalsCompleted == 0
            && noTechniqueResult.rank == EncounterChallengeRank::None
            && noTechniqueResult.score == 1000,
        "TechniqueFirst cannot manufacture technique goals from raw damage alone");

    EncounterChallengeTracker overflow;
    overflow.Configure(EncounterChallengeDifficulty::Apex,
        EncounterTacticalFocus::Balanced, EncounterScoringMode::Balanced);
    const auto saturated = overflow.Resolve(
        {std::numeric_limits<std::int64_t>::max(),
            std::numeric_limits<int>::max(), std::numeric_limits<int>::max(),
            std::numeric_limits<int>::max(), true, EncounterTimeGrade::Gold});
    Expect(saturated.score == std::numeric_limits<std::int64_t>::max(),
        "challenge score saturates instead of overflowing at extreme valid counts");

    EncounterChallengeTracker negativeCounts;
    negativeCounts.Configure(EncounterChallengeDifficulty::Standard,
        EncounterTacticalFocus::Balanced);
    const auto clamped = negativeCounts.Resolve(
        {-50, -3, -2, -1, false, EncounterTimeGrade::Bronze});
    Expect(clamped.score == 0 && clamped.sideGoalsCompleted == 0
            && clamped.rank == EncounterChallengeRank::None,
        "negative score and event inputs fail closed to zero contribution");
}
}

int main() {
    TestDeterministicProximitySelection();
    TestBoundedDiscoveryLedgerAndRepeatSafety();
    TestOrderedObjectiveGuidanceKeepsFreeDiscovery();
    TestRewardRoutesThroughResourceRules();
    TestEncounterChallengeScoringRanksAndFirstClears();
    if (failures != 0) return 1;
    std::cout << "Landmark interaction tests passed\n";
    return 0;
}
