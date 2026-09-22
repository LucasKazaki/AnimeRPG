#include "Engine/Scene/LandmarkEncounter.h"

namespace Astral::Scene {

LandmarkEncounterReport LandmarkEncounter::TryActivate(
    const LandmarkInteractionReport& interaction, const CombatSandbox& combatSandbox) {
    if (state_ != LandmarkEncounterState::Locked) {
        return {LandmarkEncounterResult::AlreadyStarted, 0.0f};
    }
    if (interaction.result != LandmarkInteractionResult::Discovered
        || interaction.landmark != EncounterLandmark) {
        lastReport_ = {LandmarkEncounterResult::DiscoveryRequired, 0.0f};
        return lastReport_;
    }
    if (combatSandbox.Dummy().IsDefeated()) {
        lastReport_ = {LandmarkEncounterResult::TargetUnavailable, 0.0f};
        return lastReport_;
    }

    state_ = LandmarkEncounterState::Active;
    lastReport_ = {LandmarkEncounterResult::Activated, 0.0f};
    return lastReport_;
}

LandmarkEncounterReport LandmarkEncounter::TryRetry(CombatSandbox& combatSandbox,
    ShadowbladeActions& shadowbladeActions) {
    if (state_ == LandmarkEncounterState::Locked) {
        lastReport_ = {LandmarkEncounterResult::RetryUnavailable, 0.0f};
        return lastReport_;
    }

    combatSandbox.ResetTrainingSession();
    shadowbladeActions.ResetForEncounter();
    state_ = LandmarkEncounterState::Active;
    lastReport_ = {LandmarkEncounterResult::Retried, 0.0f};
    return lastReport_;
}

bool LandmarkEncounter::Update(const CombatSandbox& combatSandbox,
    ShadowbladeActions& shadowbladeActions) {
    if (state_ != LandmarkEncounterState::Active || !combatSandbox.Dummy().IsDefeated()) {
        return false;
    }

    state_ = LandmarkEncounterState::Completed;
    const float rewardApplied = rewardGranted_
        ? 0.0f
        : shadowbladeActions.RestoreResource(CompletionReward);
    rewardGranted_ = true;
    lastReport_ = {LandmarkEncounterResult::Completed, rewardApplied};
    return true;
}

} // namespace Astral::Scene
