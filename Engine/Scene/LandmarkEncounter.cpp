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
    activationElapsedSeconds_ = combatSandbox.ElapsedSecondsPrecise();
    lastReport_ = {LandmarkEncounterResult::Activated, 0.0f};
    return lastReport_;
}

bool LandmarkEncounter::Update(const CombatSandbox& combatSandbox,
    ShadowbladeActions& shadowbladeActions) {
    if (state_ != LandmarkEncounterState::Active || !combatSandbox.Dummy().IsDefeated()) {
        return false;
    }

    state_ = LandmarkEncounterState::Completed;
    const double completionSecondsPrecise = std::max(
        0.0, combatSandbox.ElapsedSecondsPrecise() - activationElapsedSeconds_);
    const float completionSeconds = static_cast<float>(completionSecondsPrecise);
    const EncounterGrade grade = GradeForSeconds(completionSecondsPrecise);
    float rewardApplied = 0.0f;
    if (!completionRewardGranted_) {
        completionRewardGranted_ = true;
        rewardApplied = shadowbladeActions.RestoreResource(CompletionReward);
    }

    EncounterChallengeResult challenge{};
    if (challengeTracker_.Enabled()) {
        const TrainingStats& stats = combatSandbox.Stats();
        challenge = challengeTracker_.Resolve({
            combatSandbox.TrainingChallengeScore(),
            stats.reactionCount,
            stats.staggerCount,
            stats.finisherCount,
            shadowbladeActions.PlayerHealth() == ShadowbladeActions::MaximumPlayerHealth,
            ChallengeTimeGrade(grade),
        });
        if (challenge.firstClearRewardRequested > 0.0f) {
            challenge.firstClearRewardApplied =
                shadowbladeActions.RestoreResource(challenge.firstClearRewardRequested);
            rewardApplied += challenge.firstClearRewardApplied;
        }
    }

    lastReport_ = {LandmarkEncounterResult::Completed, rewardApplied,
        grade, completionSeconds, challenge};
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
    activationElapsedSeconds_ = combatSandbox.ElapsedSecondsPrecise();
    lastReport_ = {LandmarkEncounterResult::Retried, 0.0f};
    return lastReport_;
}

EncounterGrade LandmarkEncounter::GradeForSeconds(double seconds) {
    if (seconds <= GoldTimeSeconds) return EncounterGrade::Gold;
    if (seconds <= SilverTimeSeconds) return EncounterGrade::Silver;
    return EncounterGrade::Bronze;
}

EncounterTimeGrade LandmarkEncounter::ChallengeTimeGrade(EncounterGrade grade) {
    switch (grade) {
    case EncounterGrade::Gold: return EncounterTimeGrade::Gold;
    case EncounterGrade::Silver: return EncounterTimeGrade::Silver;
    case EncounterGrade::Bronze:
    case EncounterGrade::None:
    default: return EncounterTimeGrade::Bronze;
    }
}

} // namespace Astral::Scene