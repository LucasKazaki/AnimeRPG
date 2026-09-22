#include "Engine/Scene/LandmarkEncounter.h"

#include <algorithm>

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
    activationElapsedSeconds_ = combatSandbox.ElapsedSeconds();
    lastReport_ = {LandmarkEncounterResult::Activated, 0.0f};
    return lastReport_;
}

bool LandmarkEncounter::Update(const CombatSandbox& combatSandbox,
    ShadowbladeActions& shadowbladeActions) {
    if (state_ != LandmarkEncounterState::Active || !combatSandbox.Dummy().IsDefeated()) {
        return false;
    }

    state_ = LandmarkEncounterState::Completed;
    const float completionSeconds = std::max(
        0.0f, combatSandbox.ElapsedSeconds() - activationElapsedSeconds_);
    float rewardApplied = 0.0f;
    if (!completionRewardGranted_) {
        completionRewardGranted_ = true;
        rewardApplied = shadowbladeActions.RestoreResource(CompletionReward);
    }
    lastReport_ = {LandmarkEncounterResult::Completed, rewardApplied,
        GradeForSeconds(completionSeconds), completionSeconds};
    return true;
}

LandmarkEncounterReport LandmarkEncounter::Retry(CombatSandbox& combatSandbox,
    ShadowbladeActions& shadowbladeActions) {
    if (state_ != LandmarkEncounterState::Completed) {
        lastReport_ = {LandmarkEncounterResult::RetryUnavailable, 0.0f};
        return lastReport_;
    }

    const DefenseTimingPreset timingPreset =
        shadowbladeActions.PerfectDefenseWindowSeconds()
            == ShadowbladeActions::ForgivingPerfectDefenseWindowSeconds
        ? DefenseTimingPreset::Forgiving
        : DefenseTimingPreset::Standard;
    combatSandbox.ResetTrainingSession();
    shadowbladeActions = ShadowbladeActions{};
    shadowbladeActions.SetDefenseTimingPreset(timingPreset);
    state_ = LandmarkEncounterState::Active;
    activationElapsedSeconds_ = combatSandbox.ElapsedSeconds();
    lastReport_ = {LandmarkEncounterResult::Retried, 0.0f};
    return lastReport_;
}

EncounterGrade LandmarkEncounter::GradeForSeconds(float seconds) {
    if (seconds <= GoldTimeSeconds) return EncounterGrade::Gold;
    if (seconds <= SilverTimeSeconds) return EncounterGrade::Silver;
    return EncounterGrade::Bronze;
}

} // namespace Astral::Scene
