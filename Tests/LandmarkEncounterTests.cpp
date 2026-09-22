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
            && Near(actions.Resource(), 80.0f) && encounter.RewardGranted(),
        "completion restores the bounded reward through Shadowblade state");
    Expect(!encounter.Update(combat, actions) && Near(actions.Resource(), 80.0f),
        "repeat updates cannot farm the completion reward");
    Expect(encounter.TryActivate(Discovery(LandmarkKind::LincolnMemorial), combat).result
            == LandmarkEncounterResult::AlreadyStarted
            && encounter.State() == LandmarkEncounterState::Completed,
        "a completed encounter cannot reactivate without explicit retry");

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

void TestCleanRetryResetsTransientStateWithoutDuplicateReward() {
    using namespace Astral::Scene;

    CombatSandbox lockedCombat;
    ShadowbladeActions lockedActions;
    LandmarkEncounter lockedEncounter;
    Expect(lockedEncounter.TryRetry(lockedCombat, lockedActions).result
            == LandmarkEncounterResult::RetryUnavailable
            && lockedEncounter.State() == LandmarkEncounterState::Locked,
        "retry is unavailable before the encounter is activated");

    CombatSandbox combat;
    ShadowbladeActions actions;
    LandmarkEncounter encounter;
    encounter.TryActivate(Discovery(LandmarkKind::LincolnMemorial), combat);
    actions.TryDash({});
    actions.SetGuarding(true);
    combat.ScheduleEnemyAttack(0.1f, 35);
    combat.AdvanceTime(0.1f);
    combat.ApplyDamage(combat.Dummy().maximumHealth);
    Expect(encounter.Update(combat, actions) && encounter.RewardGranted(),
        "first clear records its one-time reward grant");

    const auto retried = encounter.TryRetry(combat, actions);
    Expect(retried.result == LandmarkEncounterResult::Retried
            && encounter.State() == LandmarkEncounterState::Active,
        "completed encounter can enter a clean explicit retry");
    Expect(combat.Dummy().health == combat.Dummy().maximumHealth
            && combat.Player().health == combat.Player().maximumHealth
            && combat.Dummy().posture == 0 && !combat.EnemyAttackActive()
            && !combat.CounterReady() && !combat.ComboFinisherReady(),
        "retry resets transient combat state and both training participants");
    Expect(Near(actions.Resource(), ShadowbladeActions::MaximumResource)
            && Near(actions.DashCooldownRemaining(), 0.0f)
            && Near(actions.FatalStrikeCooldownRemaining(), 0.0f)
            && !actions.IsGuarding(),
        "retry restores a clean Shadowblade resource, cooldown, and guard state");
    Expect(encounter.RewardGranted(),
        "retry preserves the authoritative one-time reward history");

    actions.TryFatalStrike({}, combat);
    Expect(Near(actions.Resource(), 50.0f),
        "replay setup spends resource after the clean retry");
    combat.ApplyDamage(combat.Dummy().health);
    Expect(encounter.Update(combat, actions)
            && encounter.LastReport().result == LandmarkEncounterResult::Completed
            && Near(encounter.LastReport().rewardApplied, 0.0f)
            && Near(actions.Resource(), 50.0f),
        "replay completion cannot duplicate the original progression reward");

    CombatSandbox activeCombat;
    ShadowbladeActions activeActions;
    LandmarkEncounter activeEncounter;
    activeEncounter.TryActivate(Discovery(LandmarkKind::LincolnMemorial), activeCombat);
    activeActions.TryDash({});
    activeCombat.ApplyDamage(25);
    Expect(activeEncounter.TryRetry(activeCombat, activeActions).result
            == LandmarkEncounterResult::Retried
            && activeCombat.Dummy().health == activeCombat.Dummy().maximumHealth
            && Near(activeActions.Resource(), ShadowbladeActions::MaximumResource)
            && !activeEncounter.RewardGranted(),
        "retrying an uncleared attempt restarts cleanly without marking its reward claimed");
}
}

int main() {
    TestActivationRequiresDesignatedDiscoveryAndLiveTarget();
    TestCompletionRewardsOnceThroughExistingCap();
    TestCleanRetryResetsTransientStateWithoutDuplicateReward();
    if (failures != 0) return 1;
    std::cout << "Landmark encounter tests passed\n";
    return 0;
}
