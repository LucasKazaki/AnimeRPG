#include "Engine/Scene/LandmarkEncounter.h"

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

Astral::Scene::LandmarkInteractionReport Discovery(Astral::Scene::LandmarkKind landmark) {
    return {Astral::Scene::LandmarkInteractionResult::Discovered, landmark, 0.0f};
}

void TestActivationRequiresDesignatedDiscoveryAndLiveTarget() {
    using namespace Astral::Scene;
    CombatSandbox combat;
    LandmarkEncounter encounter;

    Expect(encounter.TryActivate({}, combat).result
            == LandmarkEncounterResult::DiscoveryRequired,
        "activation requires a discovery report");
    Expect(encounter.TryActivate(Discovery(LandmarkKind::ReflectingPool), combat).result
            == LandmarkEncounterResult::DiscoveryRequired,
        "only the designated Lincoln discovery activates the encounter");
    const auto activated = encounter.TryActivate(Discovery(LandmarkKind::LincolnMemorial), combat);
    Expect(activated.result == LandmarkEncounterResult::Activated
            && encounter.State() == LandmarkEncounterState::Active,
        "Lincoln discovery activates one live training encounter");

    CombatSandbox defeatedCombat;
    defeatedCombat.ApplyDamage(defeatedCombat.Dummy().maximumHealth);
    LandmarkEncounter unavailableEncounter;
    Expect(unavailableEncounter.TryActivate(
            Discovery(LandmarkKind::LincolnMemorial), defeatedCombat).result
            == LandmarkEncounterResult::TargetUnavailable
            && unavailableEncounter.State() == LandmarkEncounterState::Locked,
        "a pre-defeated target cannot create an instant encounter completion");
}

void TestCompletionRewardsOnceThroughExistingCap() {
    using namespace Astral::Scene;
    CombatSandbox combat;
    ShadowbladeActions actions;
    LandmarkEncounter encounter;
    actions.TryFatalStrike({}, combat);
    Expect(Near(actions.Resource(), 50.0f) && combat.Dummy().health == 20,
        "setup uses existing fatal-strike cost and damage");
    encounter.TryActivate(Discovery(LandmarkKind::LincolnMemorial), combat);
    Expect(!encounter.Update(combat, actions)
            && encounter.State() == LandmarkEncounterState::Active,
        "a live target keeps the encounter active without reward");

    combat.ApplyDamage(20);
    Expect(encounter.Update(combat, actions)
            && encounter.State() == LandmarkEncounterState::Completed,
        "defeating the target completes the active encounter");
    Expect(encounter.LastReport().result == LandmarkEncounterResult::Completed
            && Near(encounter.LastReport().rewardApplied, LandmarkEncounter::CompletionReward)
            && Near(actions.Resource(), 80.0f),
        "completion restores the bounded reward through Shadowblade state");
    Expect(!encounter.Update(combat, actions) && Near(actions.Resource(), 80.0f),
        "repeat updates cannot farm the completion reward");
    Expect(encounter.TryActivate(Discovery(LandmarkKind::LincolnMemorial), combat).result
            == LandmarkEncounterResult::AlreadyStarted
            && encounter.State() == LandmarkEncounterState::Completed,
        "a completed encounter cannot reactivate");

    CombatSandbox cappedCombat;
    ShadowbladeActions cappedActions;
    LandmarkEncounter cappedEncounter;
    cappedActions.TryDash({});
    cappedEncounter.TryActivate(Discovery(LandmarkKind::LincolnMemorial), cappedCombat);
    cappedCombat.ApplyDamage(cappedCombat.Dummy().maximumHealth);
    Expect(cappedEncounter.Update(cappedCombat, cappedActions)
            && Near(cappedEncounter.LastReport().rewardApplied, 25.0f)
            && Near(cappedActions.Resource(), ShadowbladeActions::MaximumResource),
        "completion reward obeys the existing maximum-resource cap");
}

void TestConfiguredChallengeRewardsPersistAcrossRetryAndRespectCap() {
    using namespace Astral::Scene;

    CombatSandbox combat;
    ShadowbladeActions actions;
    LandmarkEncounter encounter;
    encounter.ConfigureChallenge(EncounterChallengeDifficulty::Standard,
        EncounterTacticalFocus::Balanced);

    actions.TryFatalStrike({}, combat);
    Expect(Near(actions.Resource(), 50.0f) && combat.Dummy().health == 20,
        "challenge reward setup creates room under the resource cap");
    Expect(encounter.TryActivate(Discovery(LandmarkKind::LincolnMemorial), combat).result
            == LandmarkEncounterResult::Activated,
        "configured challenge uses the existing encounter activation path");
    combat.ApplyDamage(20);
    Expect(encounter.Update(combat, actions),
        "configured challenge resolves when the encounter completes");

    const LandmarkEncounterReport first = encounter.LastReport();
    Expect(first.challenge.firstClear
            && Near(first.challenge.firstClearRewardRequested,
                EncounterChallengeTracker::StandardFirstClearReward)
            && Near(first.challenge.firstClearRewardApplied,
                EncounterChallengeTracker::StandardFirstClearReward),
        "first configured Standard clear requests and applies its challenge reward");
    Expect(Near(first.rewardApplied,
            LandmarkEncounter::CompletionReward
                + EncounterChallengeTracker::StandardFirstClearReward)
            && Near(actions.Resource(), 90.0f),
        "challenge reward is applied through the same bounded Shadowblade resource path");
    Expect(encounter.ChallengeTracker().FirstClearGranted(
            EncounterChallengeDifficulty::Standard),
        "encounter-owned challenge tracker records the Standard first clear");

    const auto retry = encounter.Retry(combat, actions);
    Expect(retry.result == LandmarkEncounterResult::Retried
            && encounter.State() == LandmarkEncounterState::Active,
        "configured completed encounter can retry without rebuilding challenge state");
    combat.ApplyDamage(combat.Dummy().maximumHealth);
    Expect(encounter.Update(combat, actions),
        "configured retry can complete normally");
    const LandmarkEncounterReport repeated = encounter.LastReport();
    Expect(!repeated.challenge.firstClear
            && Near(repeated.challenge.firstClearRewardRequested, 0.0f)
            && Near(repeated.challenge.firstClearRewardApplied, 0.0f)
            && Near(repeated.rewardApplied, 0.0f),
        "retry preserves first-clear ledger and cannot duplicate encounter or challenge rewards");
    Expect(encounter.ChallengeTracker().FirstClearGranted(
            EncounterChallengeDifficulty::Standard),
        "retry leaves the Standard first-clear ledger entry intact");

    CombatSandbox cappedCombat;
    ShadowbladeActions cappedActions;
    LandmarkEncounter cappedEncounter;
    cappedEncounter.ConfigureChallenge(EncounterChallengeDifficulty::Standard,
        EncounterTacticalFocus::Balanced);
    Expect(cappedEncounter.TryActivate(
            Discovery(LandmarkKind::LincolnMemorial), cappedCombat).result
            == LandmarkEncounterResult::Activated,
        "full-resource challenge activates through the normal encounter path");
    cappedCombat.ApplyDamage(cappedCombat.Dummy().maximumHealth);
    Expect(cappedEncounter.Update(cappedCombat, cappedActions),
        "full-resource challenge still records completion");
    const LandmarkEncounterReport capped = cappedEncounter.LastReport();
    Expect(capped.challenge.firstClear
            && Near(capped.challenge.firstClearRewardRequested,
                EncounterChallengeTracker::StandardFirstClearReward)
            && Near(capped.challenge.firstClearRewardApplied, 0.0f),
        "first-clear ledger advances even when the existing resource cap applies zero reward");
    Expect(Near(capped.rewardApplied, 0.0f)
            && Near(cappedActions.Resource(), ShadowbladeActions::MaximumResource)
            && cappedEncounter.ChallengeTracker().FirstClearGranted(
                EncounterChallengeDifficulty::Standard),
        "challenge and completion rewards cannot overflow or bypass the maximum resource cap");
}

void TestManaReactionStateAndReset() {
    using namespace Astral::Scene;
    CombatSandbox combat;

    const auto solar = combat.ApplyManaAffinity(ManaAffinity::Solar);
    Expect(solar.reaction == ManaReaction::None && solar.bonusDamage == 0
            && combat.TargetAffinity() == ManaAffinity::Solar,
        "first mana affinity primes the target without bonus damage");

    const auto repeatedSolar = combat.ApplyManaAffinity(ManaAffinity::Solar);
    Expect(repeatedSolar.reaction == ManaReaction::None && repeatedSolar.bonusDamage == 0
            && combat.TargetAffinity() == ManaAffinity::Solar,
        "repeating the same affinity does not fabricate a reaction");

    const auto eclipse = combat.ApplyManaAffinity(ManaAffinity::Umbral);
    Expect(eclipse.previous == ManaAffinity::Solar
            && eclipse.applied == ManaAffinity::Umbral
            && eclipse.reaction == ManaReaction::Eclipse
            && eclipse.bonusDamage == CombatSandbox::EclipseReactionDamage
            && !eclipse.weaknessExploited,
        "opposing affinities trigger one bounded Eclipse reaction on the neutral target");
    Expect(combat.TargetAffinity() == ManaAffinity::None
            && combat.Dummy().health == 100 - CombatSandbox::EclipseReactionDamage,
        "Eclipse consumes the primed affinity and applies its bounded damage");
    Expect(combat.Stats().totalDamage == CombatSandbox::EclipseReactionDamage
            && combat.Stats().hitCount == 1,
        "reaction damage participates in shared training statistics");

    combat.ResetTrainingSession();
    Expect(combat.TargetAffinity() == ManaAffinity::None && combat.Dummy().health == 100,
        "training reset clears transient affinity state");
    const auto none = combat.ApplyManaAffinity(ManaAffinity::None);
    Expect(none.reaction == ManaReaction::None && none.bonusDamage == 0
            && combat.Dummy().health == 100,
        "None affinity is a deterministic no-op");
}

void TestComboFinisherAndAccessibleAssistPreset() {
    using namespace Astral::Scene;

    CombatSandbox standard;
    for (int hit = 0; hit < CombatSandbox::StandardComboFinisherHits; ++hit) {
        const auto report = standard.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
        Expect(report.result == AttackResult::Hit,
            "standard finisher setup light attack hits");
        if (hit + 1 < CombatSandbox::StandardComboFinisherHits) {
            standard.AdvanceTime(0.4f);
        }
    }
    Expect(standard.ComboFinisherReady() && standard.ComboCount() == 3,
        "three standard hits arm one combo finisher");
    const auto outOfRange = standard.TryComboFinisher({-10.0f, 0.0f, 0.0f});
    Expect(outOfRange.result == ComboFinisherResult::OutOfRange
            && standard.ComboFinisherReady(),
        "out-of-range finisher preserves the earned opening");
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const auto invalidPosition = standard.TryComboFinisher({nan, 0.0f, 0.0f});
    Expect(invalidPosition.result == ComboFinisherResult::OutOfRange
            && standard.ComboFinisherReady() && standard.ComboCount() == 3,
        "nonfinite finisher position cannot activate or consume the earned opening");
    const auto finisher = standard.TryComboFinisher({0.0f, 0.0f, 0.0f});
    Expect(finisher.result == ComboFinisherResult::Activated
            && finisher.damageApplied == 25 && standard.Dummy().IsDefeated(),
        "valid finisher applies bounded damage capped to remaining health");
    Expect(!standard.ComboFinisherReady() && standard.ComboCount() == 0,
        "successful finisher consumes the combo opening once");

    CombatSandbox accessible;
    accessible.SetCombatAssistPreset(CombatAssistPreset::Accessible);
    Expect(accessible.ComboFinisherRequiredHits() == CombatSandbox::AccessibleComboFinisherHits,
        "accessible combat assist lowers only the finisher hit threshold");
    accessible.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    accessible.AdvanceTime(0.4f);
    accessible.TryAttack(AttackType::Heavy, {0.0f, 0.0f, 0.0f});
    Expect(accessible.ComboFinisherReady(),
        "accessible preset arms the same finisher after two successful hits");
    accessible.ResetTrainingSession();
    Expect(accessible.AssistPreset() == CombatAssistPreset::Accessible
            && accessible.ComboFinisherRequiredHits() == CombatSandbox::AccessibleComboFinisherHits,
        "session reset preserves the player-selected assist preset");
}

void TestObjectiveCompletionRewardIsOneShot() {
    using namespace Astral::Scene;

    WorldBlockout world;
    LandmarkInteraction interaction;
    ShadowbladeActions actions;
    actions.TryDash({});
    actions.AdvanceTime(1.0f);
    actions.TryDash({});
    Expect(Near(actions.Resource(), 65.0f),
        "objective reward setup creates enough missing resource to observe both rewards");

    Expect(interaction.TryInteract({4.0f, 39.0f, 0.0f}, world, actions).result
            == LandmarkInteractionResult::Discovered,
        "Reflecting Pool can still be discovered before the guided objective");
    Expect(interaction.TryInteract({5.0f, 68.0f, 0.0f}, world, actions).result
            == LandmarkInteractionResult::Discovered,
        "Washington Monument can still be discovered before Lincoln");

    const auto completion =
        interaction.TryInteract({-8.0f, 18.0f, 0.0f}, world, actions);
    Expect(completion.result == LandmarkInteractionResult::Discovered
            && interaction.ObjectiveComplete()
            && interaction.ObjectiveCompletionRewardGranted(),
        "discovering the final missing landmark completes the objective chain");
    Expect(Near(completion.rewardApplied,
            LandmarkInteraction::LincolnReward + LandmarkInteraction::ObjectiveCompletionReward)
            && Near(actions.Resource(), ShadowbladeActions::MaximumResource),
        "final discovery applies Lincoln plus one-time objective completion reward through caps");
    Expect(!interaction.OrderedSequenceIntact()
            && !interaction.OrderedResonanceRewardGranted(),
        "free out-of-order discovery completes normally without the ordered resonance bonus");

    const auto repeated =
        interaction.TryInteract({-8.0f, 18.0f, 0.0f}, world, actions);
    Expect(repeated.result == LandmarkInteractionResult::AlreadyVisited
            && Near(repeated.rewardApplied, 0.0f),
        "repeat landmark interaction cannot farm objective completion reward");
}

void TestEncounterGradesAndCleanRetry() {
    using namespace Astral::Scene;

    CombatSandbox combat;
    ShadowbladeActions actions;
    LandmarkEncounter encounter;
    Expect(encounter.Retry(combat, actions).result == LandmarkEncounterResult::RetryUnavailable,
        "retry is rejected before an encounter has completed");

    actions.TryDash({});
    actions.AdvanceTime(1.0f);
    actions.TryDash({});
    actions.SetDefenseTimingPreset(DefenseTimingPreset::Forgiving);
    Expect(encounter.TryActivate(Discovery(LandmarkKind::LincolnMemorial), combat).result
            == LandmarkEncounterResult::Activated,
        "graded encounter activates from the existing Lincoln discovery");
    combat.AdvanceTime(2.0f);
    combat.ApplyDamage(combat.Dummy().maximumHealth);
    Expect(encounter.Update(combat, actions),
        "graded encounter completes when the target is defeated");
    Expect(encounter.LastReport().grade == EncounterGrade::Gold
            && Near(encounter.LastReport().completionSeconds, 2.0f),
        "fast clear receives deterministic Gold timing grade");
    Expect(encounter.CompletionRewardGranted()
            && Near(encounter.LastReport().rewardApplied, LandmarkEncounter::CompletionReward),
        "first clear grants the existing completion reward exactly once");

    const auto retry = encounter.Retry(combat, actions);
    Expect(retry.result == LandmarkEncounterResult::Retried
            && encounter.State() == LandmarkEncounterState::Active,
        "completed encounter can restart immediately without rediscovery");
    Expect(combat.Dummy().health == combat.Dummy().maximumHealth
            && combat.Stats().totalDamage == 0 && combat.ComboCount() == 0,
        "retry restores target, combo, and training metrics to a clean attempt");
    Expect(Near(actions.Resource(), ShadowbladeActions::MaximumResource)
            && Near(actions.DashCooldownRemaining(), 0.0f)
            && Near(actions.FatalStrikeCooldownRemaining(), 0.0f)
            && actions.PlayerHealth() == ShadowbladeActions::MaximumPlayerHealth
            && actions.GuardIntegrity() == ShadowbladeActions::MaximumGuardIntegrity,
        "retry restores Shadowblade resource, cooldown, health, and guard state");
    Expect(Near(actions.PerfectDefenseWindowSeconds(),
            ShadowbladeActions::ForgivingPerfectDefenseWindowSeconds),
        "retry preserves the player's selected defense timing preset");

    combat.AdvanceTime(7.0f);
    combat.ApplyDamage(combat.Dummy().maximumHealth);
    Expect(encounter.Update(combat, actions)
            && encounter.LastReport().grade == EncounterGrade::Bronze
            && Near(encounter.LastReport().rewardApplied, 0.0f),
        "retry can be completed and graded without duplicating its one-time reward");

    CombatSandbox silverCombat;
    ShadowbladeActions silverActions;
    LandmarkEncounter silverEncounter;
    silverEncounter.TryActivate(Discovery(LandmarkKind::LincolnMemorial), silverCombat);
    silverCombat.AdvanceTime(4.0f);
    silverCombat.ApplyDamage(silverCombat.Dummy().maximumHealth);
    Expect(silverEncounter.Update(silverCombat, silverActions)
            && silverEncounter.LastReport().grade == EncounterGrade::Silver,
        "mid-speed clear receives deterministic Silver timing grade");
}

void TestEncounterGradePrecisionAtCutoffs() {
    using namespace Astral::Scene;

    CombatSandbox exactGoldCombat;
    ShadowbladeActions exactGoldActions;
    LandmarkEncounter exactGoldEncounter;
    exactGoldEncounter.TryActivate(Discovery(LandmarkKind::LincolnMemorial), exactGoldCombat);
    exactGoldCombat.AdvanceTime(3.0f);
    exactGoldCombat.ApplyDamage(exactGoldCombat.Dummy().maximumHealth);
    Expect(exactGoldEncounter.Update(exactGoldCombat, exactGoldActions)
            && exactGoldEncounter.LastReport().grade == EncounterGrade::Gold,
        "an exact three-second clear remains Gold");

    CombatSandbox steppedGoldCombat;
    ShadowbladeActions steppedGoldActions;
    LandmarkEncounter steppedGoldEncounter;
    steppedGoldEncounter.TryActivate(Discovery(LandmarkKind::LincolnMemorial), steppedGoldCombat);
    for (int step = 0; step < 30; ++step) steppedGoldCombat.AdvanceTime(0.1f);
    steppedGoldCombat.ApplyDamage(steppedGoldCombat.Dummy().maximumHealth);
    Expect(steppedGoldEncounter.Update(steppedGoldCombat, steppedGoldActions)
            && steppedGoldEncounter.LastReport().grade == EncounterGrade::Silver,
        "accumulated frame time just above three seconds cannot be rounded back into Gold");

    CombatSandbox steppedSilverCombat;
    ShadowbladeActions steppedSilverActions;
    LandmarkEncounter steppedSilverEncounter;
    steppedSilverEncounter.TryActivate(Discovery(LandmarkKind::LincolnMemorial), steppedSilverCombat);
    for (int step = 0; step < 60; ++step) steppedSilverCombat.AdvanceTime(0.1f);
    steppedSilverCombat.ApplyDamage(steppedSilverCombat.Dummy().maximumHealth);
    Expect(steppedSilverEncounter.Update(steppedSilverCombat, steppedSilverActions)
            && steppedSilverEncounter.LastReport().grade == EncounterGrade::Bronze,
        "accumulated frame time just above six seconds cannot be rounded back into Silver");
}

void TestTrainingEnemyProfilesAndVariantSelector() {
    using namespace Astral::Scene;

    CombatSandbox combat;
    const TrainingEnemyDefinition standard = combat.CurrentEnemyDefinition();
    Expect(combat.EnemyProfile() == TrainingEnemyProfile::Standard
            && standard.maximumHealth == 100 && standard.maximumPosture == 80
            && standard.weakness == ManaAffinity::None,
        "default training profile preserves the original neutral target");

    combat.SetCombatAssistPreset(CombatAssistPreset::Accessible);
    combat.ApplyDamage(25);
    Expect(!combat.SetTrainingEnemyProfile(TrainingEnemyProfile::Standard)
            && combat.Dummy().health == 75 && combat.Stats().totalDamage == 25,
        "selecting the already-active profile is idempotent and does not erase the attempt");

    Expect(combat.SetTrainingEnemyProfile(TrainingEnemyProfile::Bulwark),
        "training can switch to the Bulwark enemy profile");
    const TrainingEnemyDefinition bulwark = combat.CurrentEnemyDefinition();
    Expect(bulwark.maximumHealth == 180 && bulwark.maximumPosture == 120
            && bulwark.weakness == ManaAffinity::Umbral,
        "Bulwark exposes distinct health, posture, and affinity weakness data");
    Expect(combat.Dummy().health == 180 && combat.Dummy().maximumPosture == 120
            && combat.Stats().totalDamage == 0 && combat.TargetAffinity() == ManaAffinity::None,
        "changing training enemy starts a clean transient combat attempt");
    Expect(combat.AssistPreset() == CombatAssistPreset::Accessible,
        "enemy selection preserves the player's combat-assist preference");

    combat.ApplyManaAffinity(ManaAffinity::Solar);
    Expect(combat.SetTrainingEnemyProfile(TrainingEnemyProfile::Vanguard),
        "training can switch again to the Vanguard profile");
    const TrainingEnemyDefinition vanguard = combat.CurrentEnemyDefinition();
    Expect(vanguard.maximumHealth == 90 && vanguard.maximumPosture == 60
            && vanguard.weakness == ManaAffinity::Solar
            && combat.Dummy().health == 90 && combat.TargetAffinity() == ManaAffinity::None,
        "Vanguard applies its own bounded stats and clears prior target-affinity state");
}

void TestWeaknessReactionAndEclipseFinisherOpening() {
    using namespace Astral::Scene;

    CombatSandbox combat;
    combat.SetTrainingEnemyProfile(TrainingEnemyProfile::Bulwark);
    combat.ApplyManaAffinity(ManaAffinity::Solar);
    const ManaReactionReport eclipse = combat.ApplyManaAffinity(ManaAffinity::Umbral);
    Expect(eclipse.reaction == ManaReaction::Eclipse && eclipse.weaknessExploited
            && eclipse.bonusDamage
                == CombatSandbox::EclipseReactionDamage + CombatSandbox::WeaknessReactionBonusDamage,
        "applying the Bulwark weakness as the second affinity strengthens Eclipse once");
    Expect(combat.Dummy().health == 150 && combat.EclipseOpeningReady()
            && combat.Stats().reactionCount == 1 && combat.TechniqueChain() == 1
            && combat.Stats().techniqueScore == CombatSandbox::ReactionTechniquePoints,
        "successful Eclipse creates one follow-up opening and one technique event");

    for (int hit = 0; hit < CombatSandbox::StandardComboFinisherHits; ++hit) {
        Expect(combat.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f}).result
                == AttackResult::Hit,
            "Eclipse follow-up setup keeps ordinary light attacks available");
        if (hit + 1 < CombatSandbox::StandardComboFinisherHits) combat.AdvanceTime(0.4f);
    }
    Expect(combat.ComboFinisherReady() && combat.EclipseOpeningReady(),
        "earned combo finisher and Eclipse opening can coexist");

    const ComboFinisherReport rejected = combat.TryComboFinisher({-10.0f, 0.0f, 0.0f});
    Expect(rejected.result == ComboFinisherResult::OutOfRange
            && combat.ComboFinisherReady() && combat.EclipseOpeningReady(),
        "invalid Eclipse follow-up attempt preserves both earned openings");

    const ComboFinisherReport followUp = combat.TryComboFinisher({0.0f, 0.0f, 0.0f});
    Expect(followUp.result == ComboFinisherResult::Activated && followUp.eclipseFollowUp
            && followUp.damageApplied
                == CombatSandbox::ComboFinisherDamage + CombatSandbox::EclipseFinisherBonusDamage,
        "valid Eclipse follow-up adds its bounded finisher bonus");
    Expect(combat.Dummy().health == 10 && !combat.EclipseOpeningReady()
            && combat.Stats().finisherCount == 1 && combat.TechniqueChain() == 2
            && combat.Stats().techniqueScore
                == CombatSandbox::ReactionTechniquePoints
                    + CombatSandbox::FinisherTechniquePoints * 2,
        "successful Eclipse finisher consumes the opening and extends technique scoring once");

    CombatSandbox nonWeak;
    nonWeak.SetTrainingEnemyProfile(TrainingEnemyProfile::Vanguard);
    nonWeak.ApplyManaAffinity(ManaAffinity::Solar);
    const ManaReactionReport ordinary = nonWeak.ApplyManaAffinity(ManaAffinity::Umbral);
    Expect(ordinary.reaction == ManaReaction::Eclipse && !ordinary.weaknessExploited
            && ordinary.bonusDamage == CombatSandbox::EclipseReactionDamage,
        "opposing affinity still reacts without a weakness bonus when the applied affinity mismatches");
}

void TestTechniqueChainChallengeScoreAndTimeout() {
    using namespace Astral::Scene;

    CombatSandbox combat;
    combat.SetTrainingEnemyProfile(TrainingEnemyProfile::Bulwark);
    combat.ApplyManaAffinity(ManaAffinity::Solar);
    combat.ApplyManaAffinity(ManaAffinity::Umbral);
    Expect(combat.TechniqueChain() == 1
            && combat.Stats().techniqueScore == CombatSandbox::ReactionTechniquePoints,
        "first combat technique starts a one-step technique chain");

    combat.TryAttack(AttackType::Heavy, {0.0f, 0.0f, 0.0f});
    combat.AdvanceTime(1.0f);
    const AttackReport stagger = combat.TryAttack(AttackType::Heavy, {0.0f, 0.0f, 0.0f});
    Expect(stagger.staggerTriggered && combat.TechniqueChain() == 2
            && combat.Stats().staggerCount == 1
            && combat.Stats().bestTechniqueChain == 2,
        "stagger inside the chain window extends the technique chain exactly once");
    Expect(combat.Stats().techniqueScore
            == CombatSandbox::ReactionTechniquePoints
                + CombatSandbox::StaggerTechniquePoints * 2,
        "technique points use the bounded current-chain multiplier");
    Expect(combat.Stats().totalDamage == 150 && combat.TrainingChallengeScore() == 250,
        "fast challenge score combines damage, technique score, and the fast-clear coefficient");

    combat.AdvanceTime(CombatSandbox::TechniqueChainWindowSeconds + 0.01f);
    Expect(combat.TechniqueChain() == 0 && combat.Stats().bestTechniqueChain == 2,
        "inactive technique chain expires without erasing the best-chain record");
    combat.ApplyDamage(30);
    const int frozenScore = combat.TrainingChallengeScore();
    Expect(combat.Dummy().IsDefeated() && frozenScore == 287,
        "defeat freezes a fast-clear challenge score using bounded integer scoring");
    combat.AdvanceTime(100.0f);
    Expect(combat.TrainingChallengeScore() == frozenScore,
        "post-defeat idle time cannot reduce an already-earned challenge score");
}

void TestOrderedLandmarkResonanceBonus() {
    using namespace Astral::Scene;

    WorldBlockout world;
    LandmarkInteraction interaction;
    ShadowbladeActions actions;
    actions.TryDash({});
    actions.AdvanceTime(1.0f);
    actions.TryDash({});
    actions.AdvanceTime(1.0f);
    actions.TryDash({});
    Expect(Near(actions.Resource(), 55.0f),
        "ordered resonance setup creates enough missing resource to observe all rewards");

    const auto lincoln = interaction.TryInteract({-8.0f, 18.0f, 0.0f}, world, actions);
    Expect(lincoln.result == LandmarkInteractionResult::Discovered
            && interaction.OrderedSequenceIntact()
            && interaction.OrderedDiscoveryProgress() == 1
            && Near(lincoln.rewardApplied, LandmarkInteraction::LincolnReward),
        "Lincoln first advances the intended resonance discovery order");
    const auto pool = interaction.TryInteract({4.0f, 39.0f, 0.0f}, world, actions);
    Expect(pool.result == LandmarkInteractionResult::Discovered
            && interaction.OrderedDiscoveryProgress() == 2,
        "Reflecting Pool second preserves the resonance sequence");
    const auto monument = interaction.TryInteract({5.0f, 68.0f, 0.0f}, world, actions);
    Expect(monument.result == LandmarkInteractionResult::Discovered
            && interaction.ObjectiveComplete()
            && interaction.OrderedDiscoveryProgress() == LandmarkInteraction::LedgerCapacity
            && interaction.OrderedResonanceRewardGranted(),
        "Washington Monument third completes the optional ordered resonance sequence");
    Expect(Near(monument.rewardApplied,
            LandmarkInteraction::ObjectiveCompletionReward
                + LandmarkInteraction::OrderedResonanceReward)
            && Near(actions.Resource(), ShadowbladeActions::MaximumResource),
        "ordered completion grants its extra bounded reward through the existing resource cap");

    const auto repeated = interaction.TryInteract({5.0f, 68.0f, 0.0f}, world, actions);
    Expect(repeated.result == LandmarkInteractionResult::AlreadyVisited
            && Near(repeated.rewardApplied, 0.0f),
        "ordered resonance reward cannot be farmed by repeated landmark interaction");
}

} // namespace

int main() {
    TestActivationRequiresDesignatedDiscoveryAndLiveTarget();
    TestCompletionRewardsOnceThroughExistingCap();
    TestConfiguredChallengeRewardsPersistAcrossRetryAndRespectCap();
    TestManaReactionStateAndReset();
    TestComboFinisherAndAccessibleAssistPreset();
    TestObjectiveCompletionRewardIsOneShot();
    TestEncounterGradesAndCleanRetry();
    TestEncounterGradePrecisionAtCutoffs();
    TestTrainingEnemyProfilesAndVariantSelector();
    TestWeaknessReactionAndEclipseFinisherOpening();
    TestTechniqueChainChallengeScoreAndTimeout();
    TestOrderedLandmarkResonanceBonus();
    if (failures != 0) return 1;
    std::cout << "Landmark encounter tests passed\n";
    return 0;
}
