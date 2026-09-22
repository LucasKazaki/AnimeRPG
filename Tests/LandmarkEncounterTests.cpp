#include "Engine/Scene/LandmarkEncounter.h"

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
            && eclipse.bonusDamage == CombatSandbox::EclipseReactionDamage,
        "opposing affinities trigger one bounded Eclipse reaction");
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

} // namespace

int main() {
    TestActivationRequiresDesignatedDiscoveryAndLiveTarget();
    TestCompletionRewardsOnceThroughExistingCap();
    TestManaReactionStateAndReset();
    TestComboFinisherAndAccessibleAssistPreset();
    TestObjectiveCompletionRewardIsOneShot();
    TestEncounterGradesAndCleanRetry();
    if (failures != 0) return 1;
    std::cout << "Landmark encounter tests passed\n";
    return 0;
}
