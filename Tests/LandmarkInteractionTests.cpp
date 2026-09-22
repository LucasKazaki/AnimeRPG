#include "Engine/Scene/CharacterProgression.h"
#include "Engine/Scene/EncounterChallenge.h"
#include "Engine/Scene/LandmarkInteraction.h"

#include <array>
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

void TestSingleCharacterProgressionAndBuildPresets() {
    using namespace Astral::Scene;
    CharacterProgression progression;

    const ProgressionRewardReport grant = progression.GrantRewards(650, 120, 100);
    Expect(grant.experienceApplied == 650 && grant.levelsGained == 3
            && progression.Level() == 4 && progression.ExperienceIntoLevel() == 50,
        "experience advances the persistent protagonist through deterministic level thresholds");
    Expect(grant.masteryPointsApplied == 120 && progression.MasteryRank() == 3
            && progression.AvailableMasteryPoints() == 120,
        "mastery rewards advance rank while remaining available for specialization spending");
    Expect(grant.enhancementMaterialsApplied == 100
            && progression.EnhancementMaterials() == 100,
        "enhancement materials accumulate independently from mastery currency");

    Expect(progression.UpgradeTalent(CoreTalent::ShadowStep)
            == ProgressionActionResult::Success
            && progression.UpgradeTalent(CoreTalent::ShadowStep)
                == ProgressionActionResult::Success
            && progression.TalentTier(CoreTalent::ShadowStep) == 2
            && progression.AvailableMasteryPoints() == 60,
        "mastery points upgrade an original Shadowblade talent through rank-gated tiers");
    Expect(progression.UpgradeTalent(CoreTalent::ShadowStep)
            == ProgressionActionResult::Locked
            && progression.TalentTier(CoreTalent::ShadowStep) == 2,
        "higher talent tiers cannot bypass their mastery-rank prerequisite");
    Expect(progression.UpgradeTalent(CoreTalent::EclipseEdge)
            == ProgressionActionResult::Success
            && progression.UpgradeTalent(CoreTalent::BreakerFocus)
                == ProgressionActionResult::Success,
        "earned mastery can be split across distinct single-character specialization paths");

    Expect(progression.UpgradeWeapon() == ProgressionActionResult::Success
            && progression.UpgradeWeapon() == ProgressionActionResult::Success
            && progression.WeaponTier() == 2
            && progression.EnhancementMaterials() == 70,
        "weapon enhancement consumes bounded materials and honors protagonist level gates");
    Expect(progression.UpgradeWeapon() == ProgressionActionResult::Locked
            && progression.WeaponTier() == 2,
        "weapon enhancement cannot skip its next character-level gate");

    Expect(progression.EquipTalent(0, CoreTalent::ShadowStep)
            == ProgressionActionResult::Success
            && progression.EquipTalent(1, CoreTalent::EclipseEdge)
                == ProgressionActionResult::Success
            && progression.SaveBuildPreset(0) == ProgressionActionResult::Success
            && progression.BuildPresetSaved(0),
        "one of three preset slots saves the protagonist's equipped unlocked talents");
    Expect(progression.EquipTalent(1, CoreTalent::BreakerFocus)
            == ProgressionActionResult::Success
            && progression.EquippedTalent(1) == CoreTalent::BreakerFocus,
        "equipped build state can change independently of permanent progression");
    Expect(progression.LoadBuildPreset(0) == ProgressionActionResult::Success
            && progression.EquippedTalent(0) == CoreTalent::ShadowStep
            && progression.EquippedTalent(1) == CoreTalent::EclipseEdge,
        "loading a saved preset atomically restores the intended equipped talent pair");
    Expect(progression.EquipTalent(1, CoreTalent::ShadowStep)
            == ProgressionActionResult::Invalid,
        "a preset loadout cannot equip the same talent twice");
    Expect(progression.LoadBuildPreset(1) == ProgressionActionResult::EmptyPreset
            && progression.SaveBuildPreset(CharacterProgression::BuildPresetSlots)
                == ProgressionActionResult::Invalid,
        "empty and out-of-range preset operations fail closed without mutating the build");

    int capLevels = 0;
    CharacterProgression capped;
    const int capExperience = capped.GrantExperience(std::numeric_limits<int>::max(), capLevels);
    Expect(capped.Level() == CharacterProgression::MaximumLevel
            && capped.ExperienceIntoLevel() == 0 && capLevels == 19
            && capExperience == 19000,
        "extreme experience input saturates at the explicit level cap without overflow");
    int ignoredLevels = 0;
    Expect(capped.GrantExperience(100, ignoredLevels) == 0 && ignoredLevels == 0,
        "experience after the level cap is ignored deterministically");
}

void TestProgressionTrainingReadinessJourneyAndAwakening() {
    using namespace Astral::Scene;
    CharacterProgression progression;

    Expect(progression.SkillRank(ShadowSkill::Dash) == 1
            && progression.SkillRank(ShadowSkill::FatalStrike) == 1
            && progression.SkillRank(ShadowSkill::Defense) == 1,
        "Shadowblade skill practice starts at bounded rank one");
    Expect(progression.RecordSkillPractice(ShadowSkill::Dash, 10) == 10
            && progression.SkillRank(ShadowSkill::Dash) == 2,
        "skill practice advances Dash through a deterministic rank threshold");
    Expect(progression.RecordSkillPractice(ShadowSkill::FatalStrike, 20) == 20
            && progression.SkillRank(ShadowSkill::FatalStrike) == 3,
        "Fatal Strike proficiency tracks independently from Dash");
    Expect(progression.RecordSkillPractice(ShadowSkill::Defense,
            std::numeric_limits<int>::max()) == 40
            && progression.SkillRank(ShadowSkill::Defense)
                == CharacterProgression::MaximumSkillRank,
        "extreme practice input saturates at the skill-rank cap without overflow");
    Expect(progression.RecordSkillPractice(static_cast<ShadowSkill>(999), 10) == 0,
        "invalid skill identifiers fail closed");

    const std::array<int, CharacterProgression::TalentCount> talentTargets{2, 1, 0};
    const std::array<int, CharacterProgression::SkillCount> skillTargets{2, 3, 5};
    Expect(progression.SetTrainingPlan(5, 1, talentTargets, skillTargets)
            == ProgressionActionResult::Success,
        "a valid training plan stores level, weapon, talent, and skill targets");
    const TrainingPlanStatus initialPlan = progression.CurrentTrainingPlanStatus();
    Expect(initialPlan.active && initialPlan.targetsTotal == 8
            && !initialPlan.Complete() && initialPlan.remainingLevels == 4
            && initialPlan.remainingWeaponTiers == 1
            && initialPlan.remainingTalentTiers[0] == 2
            && initialPlan.remainingSkillRanks[0] == 0
            && initialPlan.remainingSkillRanks[1] == 0
            && initialPlan.remainingSkillRanks[2] == 0,
        "training plan reports only the unfinished progression targets");
    Expect(progression.SetTrainingPlan(21, 1, talentTargets, skillTargets)
            == ProgressionActionResult::Invalid,
        "training plans reject targets beyond explicit progression caps");

    progression.GrantRewards(1000, 300, 500);
    Expect(progression.Level() == 5
            && progression.UpgradeTalent(CoreTalent::ShadowStep)
                == ProgressionActionResult::Success
            && progression.UpgradeTalent(CoreTalent::ShadowStep)
                == ProgressionActionResult::Success
            && progression.UpgradeTalent(CoreTalent::EclipseEdge)
                == ProgressionActionResult::Success
            && progression.UpgradeWeapon() == ProgressionActionResult::Success,
        "earned progression can satisfy a multi-axis training plan");
    const TrainingPlanStatus completedPlan = progression.CurrentTrainingPlanStatus();
    Expect(completedPlan.Complete() && completedPlan.targetsComplete == 8,
        "training plan becomes complete only when every stored target is met");
    Expect(progression.CombatReadinessScore() == 27,
        "combat readiness combines bounded level, weapon, talent, and practiced-skill progress");

    const JourneyClaimReport firstMilestone = progression.ClaimJourneyMilestone(0);
    Expect(firstMilestone.result == ProgressionActionResult::Success
            && firstMilestone.reward.masteryPointsApplied == 20
            && firstMilestone.reward.enhancementMaterialsApplied == 10
            && progression.JourneyMilestoneClaimed(0),
        "reaching level five exposes one explicit Journey-style progression reward");
    Expect(progression.ClaimJourneyMilestone(0).result
            == ProgressionActionResult::AlreadyClaimed
            && progression.ClaimJourneyMilestone(1).result
                == ProgressionActionResult::Locked,
        "journey milestone rewards cannot be duplicated or claimed before their level gate");

    CharacterProgression weaponProgression;
    int levelsGained = 0;
    weaponProgression.GrantExperience(std::numeric_limits<int>::max(), levelsGained);
    weaponProgression.GrantEnhancementMaterials(1000);
    for (int tier = 0; tier < CharacterProgression::MaximumWeaponTier; ++tier) {
        Expect(weaponProgression.UpgradeWeapon() == ProgressionActionResult::Success,
            "max-level protagonist can enhance the weapon through each bounded tier");
    }
    Expect(weaponProgression.AwakenWeapon() == ProgressionActionResult::Success
            && weaponProgression.WeaponAwakeningRank() == 1,
        "max-tier weapon can earn its first late-progression awakening");
    Expect(weaponProgression.AwakenWeapon() == ProgressionActionResult::Success
            && weaponProgression.WeaponAwakeningRank()
                == CharacterProgression::MaximumWeaponAwakeningRank,
        "second awakening consumes its larger bounded material cost at level twenty");
    Expect(weaponProgression.AwakenWeapon() == ProgressionActionResult::Maxed,
        "weapon awakening cannot exceed its explicit cap");
}

void TestLandmarkObjectiveGrantsProgressionOnce() {
    using namespace Astral::Scene;
    WorldBlockout world;
    LandmarkInteraction interaction;
    ShadowbladeActions actions;
    CharacterProgression progression;
    interaction.SetCharacterProgression(&progression);

    const Astral::Math::Vec3 lincoln{-8.0f, 18.0f, 0.0f};
    const Astral::Math::Vec3 pool{4.0f, 39.0f, 0.0f};
    const Astral::Math::Vec3 monument{5.0f, 68.0f, 0.0f};
    Expect(interaction.TryInteract(lincoln, world, actions).result
            == LandmarkInteractionResult::Discovered
            && interaction.TryInteract(pool, world, actions).result
                == LandmarkInteractionResult::Discovered,
        "progression integration preserves ordinary landmark discovery before completion");
    const LandmarkInteractionReport completion =
        interaction.TryInteract(monument, world, actions);
    Expect(completion.result == LandmarkInteractionResult::Discovered
            && completion.progressionReward.experienceApplied
                == LandmarkInteraction::ObjectiveExperienceReward
            && completion.progressionReward.masteryPointsApplied
                == LandmarkInteraction::ObjectiveMasteryReward
            && completion.progressionReward.enhancementMaterialsApplied
                == LandmarkInteraction::ObjectiveEnhancementMaterialReward,
        "first National Mall objective completion grants all bounded progression currencies");
    Expect(progression.Level() == 2 && progression.ExperienceIntoLevel() == 80
            && progression.AvailableMasteryPoints() == 40
            && progression.EnhancementMaterials() == 15,
        "landmark reward feeds the persistent protagonist's level, mastery, and weapon material state");

    const LandmarkInteractionReport repeated =
        interaction.TryInteract(monument, world, actions);
    Expect(repeated.result == LandmarkInteractionResult::AlreadyVisited
            && repeated.progressionReward.experienceApplied == 0
            && repeated.progressionReward.masteryPointsApplied == 0
            && repeated.progressionReward.enhancementMaterialsApplied == 0
            && progression.Level() == 2 && progression.ExperienceIntoLevel() == 80
            && progression.AvailableMasteryPoints() == 40
            && progression.EnhancementMaterials() == 15,
        "repeat landmark interaction cannot farm persistent progression rewards");

    LandmarkInteraction compatibilityInteraction;
    ShadowbladeActions compatibilityActions;
    compatibilityInteraction.TryInteract(lincoln, world, compatibilityActions);
    compatibilityInteraction.TryInteract(pool, world, compatibilityActions);
    const auto compatibilityCompletion =
        compatibilityInteraction.TryInteract(monument, world, compatibilityActions);
    Expect(compatibilityCompletion.result == LandmarkInteractionResult::Discovered
            && compatibilityCompletion.progressionReward.experienceApplied == 0,
        "callers that do not attach progression preserve the existing landmark behavior");
}

void TestManualObjectiveActivationPreservesFreeExploration() {
    using namespace Astral::Scene;
    WorldBlockout world;
    LandmarkInteraction interaction;
    ShadowbladeActions actions;
    CharacterProgression progression;
    interaction.SetCharacterProgression(&progression);

    Expect(interaction.SetObjectiveActivationMode(LandmarkObjectiveActivationMode::ManualStart)
            && !interaction.ObjectiveActive()
            && interaction.CurrentObjective() == LandmarkObjectiveStage::AwaitingStart,
        "manual objective mode waits for explicit player activation");
    const auto earlyLincoln = interaction.TryInteract(
        {-8.0f, 18.0f, 0.0f}, world, actions);
    Expect(earlyLincoln.result == LandmarkInteractionResult::Discovered
            && interaction.IsVisited(LandmarkKind::LincolnMemorial)
            && interaction.ObjectiveProgress() == 0
            && !interaction.ObjectiveCompletionRewardGranted(),
        "free landmark discovery before activation does not silently start quest progress");

    Expect(interaction.StartObjective()
            && interaction.CurrentObjective() == LandmarkObjectiveStage::DiscoverLincoln,
        "explicit activation starts the objective at its first authored step");
    const auto revisitLincoln = interaction.TryInteract(
        {-8.0f, 18.0f, 0.0f}, world, actions);
    Expect(revisitLincoln.result == LandmarkInteractionResult::ObjectiveAdvanced
            && interaction.ObjectiveProgress() == 1
            && Near(revisitLincoln.rewardApplied, 0.0f),
        "revisiting a previously explored landmark advances the opted-in quest without duplicating discovery reward");
    Expect(interaction.TryInteract({4.0f, 39.0f, 0.0f}, world, actions).result
            == LandmarkInteractionResult::Discovered,
        "new discoveries continue to advance an active manual objective normally");
    const auto completion = interaction.TryInteract(
        {5.0f, 68.0f, 0.0f}, world, actions);
    Expect(completion.result == LandmarkInteractionResult::Discovered
            && interaction.ObjectiveComplete()
            && completion.progressionReward.experienceApplied
                == LandmarkInteraction::ObjectiveExperienceReward,
        "manual objective completion grants its existing one-time progression reward");
    const auto repeat = interaction.TryInteract({5.0f, 68.0f, 0.0f}, world, actions);
    Expect(repeat.result == LandmarkInteractionResult::AlreadyVisited
            && repeat.progressionReward.experienceApplied == 0
            && !interaction.StartObjective()
            && !interaction.SetObjectiveActivationMode(LandmarkObjectiveActivationMode::AutoStart),
        "completed manual objectives cannot be restarted, mode-reset, or reward-farmed");
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
    TestSingleCharacterProgressionAndBuildPresets();
    TestProgressionTrainingReadinessJourneyAndAwakening();
    TestLandmarkObjectiveGrantsProgressionOnce();
    TestManualObjectiveActivationPreservesFreeExploration();
    TestEncounterChallengeScoringRanksAndFirstClears();
    if (failures != 0) return 1;
    std::cout << "Landmark interaction tests passed\n";
    return 0;
}