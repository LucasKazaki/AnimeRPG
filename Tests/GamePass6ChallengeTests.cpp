#include "Engine/Scene/EncounterChallenge.h"

#include <cstdint>
#include <cstdlib>
#include <limits>

namespace {
void Check(bool condition) {
    if (!condition) std::abort();
}
}

int main() {
    using namespace Astral::Scene;

    EncounterChallengeTracker disabled;
    const auto disabledResult = disabled.Resolve({1000, 1, 1, 1, true, EncounterTimeGrade::Gold});
    Check(disabledResult.score == 0 && disabledResult.rank == EncounterChallengeRank::None
        && !disabledResult.firstClear);

    EncounterChallengeTracker standard;
    standard.Configure(EncounterChallengeDifficulty::Standard,
        EncounterTacticalFocus::Reaction);
    const auto standardResult = standard.Resolve({1000, 2, 1, 0, true, EncounterTimeGrade::Gold});
    Check(standardResult.flawlessGoal && standardResult.techniqueVarietyGoal
        && standardResult.speedGoal && standardResult.tacticalGoal);
    Check(standardResult.sideGoalsCompleted == 4
        && standardResult.rank == EncounterChallengeRank::Gold);
    Check(standardResult.score == 1680);
    Check(standardResult.firstClear
        && standardResult.firstClearRewardRequested == EncounterChallengeTracker::StandardFirstClearReward);
    const auto repeatStandard = standard.Resolve({1000, 2, 1, 0, true, EncounterTimeGrade::Gold});
    Check(!repeatStandard.firstClear && repeatStandard.firstClearRewardRequested == 0.0f);

    standard.Configure(EncounterChallengeDifficulty::Expert,
        EncounterTacticalFocus::Finisher);
    const auto expert = standard.Resolve({800, 0, 1, 2, false, EncounterTimeGrade::Silver});
    Check(expert.sideGoalsCompleted == 3 && expert.rank == EncounterChallengeRank::Silver);
    Check(expert.firstClear && standard.FirstClearGranted(EncounterChallengeDifficulty::Expert));

    standard.Configure(EncounterChallengeDifficulty::Apex,
        EncounterTacticalFocus::Balanced);
    const auto apexSilver = standard.Resolve({900, 1, 1, 1, true, EncounterTimeGrade::Silver});
    Check(apexSilver.sideGoalsCompleted == 4 && apexSilver.rank == EncounterChallengeRank::Silver);
    Check(apexSilver.firstClear);
    const auto apexGold = standard.Resolve({900, 1, 1, 1, true, EncounterTimeGrade::Gold});
    Check(apexGold.rank == EncounterChallengeRank::Gold && !apexGold.firstClear);

    EncounterChallengeTracker balancedMode;
    balancedMode.Configure(EncounterChallengeDifficulty::Standard,
        EncounterTacticalFocus::Balanced, EncounterScoringMode::Balanced);
    const auto balancedScore = balancedMode.Resolve({2000, 1, 1, 1, false, EncounterTimeGrade::Bronze});

    EncounterChallengeTracker techniqueFirst;
    techniqueFirst.Configure(EncounterChallengeDifficulty::Standard,
        EncounterTacticalFocus::Balanced, EncounterScoringMode::TechniqueFirst);
    const auto techniqueScore = techniqueFirst.Resolve({2000, 1, 1, 1, false, EncounterTimeGrade::Bronze});
    Check(techniqueScore.sideGoalsCompleted == balancedScore.sideGoalsCompleted);
    Check(techniqueScore.score > 0 && techniqueScore.score != balancedScore.score);
    Check(techniqueScore.score == 1900);

    EncounterChallengeTracker noTechnique;
    noTechnique.Configure(EncounterChallengeDifficulty::Standard,
        EncounterTacticalFocus::Reaction, EncounterScoringMode::TechniqueFirst);
    const auto noTechniqueResult = noTechnique.Resolve({2000, 0, 0, 0, false, EncounterTimeGrade::Bronze});
    Check(!noTechniqueResult.tacticalGoal && noTechniqueResult.sideGoalsCompleted == 0
        && noTechniqueResult.rank == EncounterChallengeRank::None
        && noTechniqueResult.score == 1000);

    EncounterChallengeTracker overflow;
    overflow.Configure(EncounterChallengeDifficulty::Apex,
        EncounterTacticalFocus::Balanced, EncounterScoringMode::Balanced);
    const auto saturated = overflow.Resolve({std::numeric_limits<std::int64_t>::max(),
        std::numeric_limits<int>::max(), std::numeric_limits<int>::max(),
        std::numeric_limits<int>::max(), true, EncounterTimeGrade::Gold});
    Check(saturated.score == std::numeric_limits<std::int64_t>::max());

    EncounterChallengeTracker negativeCounts;
    negativeCounts.Configure(EncounterChallengeDifficulty::Standard,
        EncounterTacticalFocus::Balanced);
    const auto clamped = negativeCounts.Resolve({-50, -3, -2, -1, false, EncounterTimeGrade::Bronze});
    Check(clamped.score == 0 && clamped.sideGoalsCompleted == 0
        && clamped.rank == EncounterChallengeRank::None);
    return 0;
}