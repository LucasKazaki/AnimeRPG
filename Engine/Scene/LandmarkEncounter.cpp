#include "Engine/Scene/LandmarkEncounter.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace Astral::Scene {
namespace {

int NonnegativeDelta(int current, int baseline) {
    return current >= baseline ? current - baseline : 0;
}

std::int64_t NonnegativeDelta(std::int64_t current, std::int64_t baseline) {
    return current >= baseline ? current - baseline : 0;
}

std::int64_t SaturatingAddNonnegative(std::int64_t left, std::int64_t right) {
    if (left <= 0) return std::max<std::int64_t>(0, right);
    if (right <= 0) return left;
    const std::int64_t maximum = std::numeric_limits<std::int64_t>::max();
    return left > maximum - right ? maximum : left + right;
}

std::int64_t EncounterCombatScore(const TrainingStats& current,
    const TrainingStats& baseline, double completionSeconds) {
    const std::int64_t damage = NonnegativeDelta(current.totalDamage, baseline.totalDamage);
    const std::int64_t technique =
        NonnegativeDelta(current.techniqueScore, baseline.techniqueScore);
    const std::int64_t baseScore = SaturatingAddNonnegative(damage, technique);
    if (baseScore <= 0) return 0;

    if (completionSeconds <= CombatSandbox::FastChallengeSeconds) {
        return SaturatingAddNonnegative(baseScore, baseScore / 4);
    }
    if (completionSeconds <= CombatSandbox::StandardChallengeSeconds) {
        return baseScore;
    }
    const std::int64_t quarter = baseScore / 4;
    const std::int64_t remainder = baseScore % 4;
    return quarter * 3 + remainder * 3 / 4;
}

} // namespace

LandmarkEncounterReport LandmarkEncounter::TryActivate(
    const LandmarkInteractionReport& interaction, CombatSandbox& combatSandbox) {
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

    if (challengeTracker_.Enabled()) {
        combatSandbox.ResetTechniqueChain();
    }
    state_ = LandmarkEncounterState::Active;
    activationElapsedSeconds_ = combatSandbox.ElapsedSecondsPrecise();
    activationTrainingStats_ = combatSandbox.Stats();
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
            EncounterCombatScore(stats, activationTrainingStats_, completionSecondsPrecise),
            NonnegativeDelta(stats.reactionCount, activationTrainingStats_.reactionCount),
            NonnegativeDelta(stats.staggerCount, activationTrainingStats_.staggerCount),
            NonnegativeDelta(stats.finisherCount, activationTrainingStats_.finisherCount),
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

    combatSandbox.ResetTrainingSession();
    shadowbladeActions.ResetTransientStatePreservingLoadout();
    state_ = LandmarkEncounterState::Active;
    activationElapsedSeconds_ = combatSandbox.ElapsedSecondsPrecise();
    activationTrainingStats_ = combatSandbox.Stats();
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
