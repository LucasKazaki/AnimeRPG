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
    activationCombatOwner_ = &combatSandbox;
    activationActionsOwner_ = interaction.actionOwner;
    lastReport_ = {LandmarkEncounterResult::Activated, 0.0f};
    return lastReport_;
}

bool LandmarkEncounter::Update(const CombatSandbox& combatSandbox,
    ShadowbladeActions& shadowbladeActions) {
    // This read-only observation is safe in the current Win32 flow, which has
    // already advanced the combat clock for the frame. The explicit training
    // AdvanceTraining path remains the sole owner of practice-session clock steps.
    trainingHub_.ObserveCombat(combatSandbox);

    if (state_ != LandmarkEncounterState::Active
        || activationCombatOwner_ != &combatSandbox) {
        return false;
    }
    if (activationActionsOwner_ && activationActionsOwner_ != &shadowbladeActions) {
        return false;
    }
    if (!combatSandbox.Dummy().IsDefeated()) {
        // Legacy/synthetic callers may not carry the action-owner witness that the
        // real LandmarkInteraction path supplies. A live nonterminal update can
        // establish that owner before completion, but an instant terminal update
        // cannot establish the provenance required for the training unlock.
        if (!activationActionsOwner_) activationActionsOwner_ = &shadowbladeActions;
        return false;
    }

    const bool trainingUnlockAuthorized = activationActionsOwner_ == &shadowbladeActions;
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
        const std::int64_t localDamage =
            NonnegativeDelta(stats.totalDamage, activationTrainingStats_.totalDamage);
        const std::int64_t localTechnique =
            NonnegativeDelta(stats.techniqueScore, activationTrainingStats_.techniqueScore);
        challenge = challengeTracker_.Resolve({
            EncounterCombatScore(stats, activationTrainingStats_, completionSecondsPrecise),
            NonnegativeDelta(stats.reactionCount, activationTrainingStats_.reactionCount),
            NonnegativeDelta(stats.staggerCount, activationTrainingStats_.staggerCount),
            NonnegativeDelta(stats.finisherCount, activationTrainingStats_.finisherCount),
            shadowbladeActions.PlayerHealth() == ShadowbladeActions::MaximumPlayerHealth,
            ChallengeTimeGrade(grade),
            localDamage,
            localTechnique,
            combatSandbox.TechniqueChain(),
        });
        if (challenge.firstClearRewardRequested > 0.0f) {
            challenge.firstClearRewardApplied =
                shadowbladeActions.RestoreResource(challenge.firstClearRewardRequested);
            rewardApplied += challenge.firstClearRewardApplied;
        }
    }

    // GAME pass 25: only an encounter completed by the combat/action pair that
    // established the live interaction may unlock training. Synthetic legacy
    // completion without an action witness retains the encounter reward contract
    // but cannot satisfy the training story gate.
    if (trainingUnlockAuthorized) {
        trainingHub_.Unlock();
        trainingHub_.ObserveCombat(combatSandbox);
    }

    lastReport_ = {LandmarkEncounterResult::Completed, rewardApplied,
        grade, completionSeconds, challenge};
    return true;
}

LandmarkEncounterReport LandmarkEncounter::Retry(CombatSandbox& combatSandbox,
    ShadowbladeActions& shadowbladeActions) {
    if (state_ != LandmarkEncounterState::Completed || trainingHub_.Active()
        || (trainingHub_.Unlocked()
            && (activationCombatOwner_ != &combatSandbox
                || activationActionsOwner_ != &shadowbladeActions))
        || !trainingHub_.AcceptsOwnerPair(combatSandbox, shadowbladeActions)) {
        lastReport_ = {LandmarkEncounterResult::RetryUnavailable, 0.0f};
        return lastReport_;
    }

    // Training practice deliberately uses an Endless target. Returning the same
    // authoritative owner pair to the landmark encounter must restore the standard
    // finite target before resetting the encounter. A mismatched pair is rejected
    // above before either owner can be mutated. Legacy synthetic completions that
    // never unlocked training retain their historical retry contract.
    if (combatSandbox.TargetMode() != TrainingTargetMode::Standard) {
        if (!combatSandbox.SetTrainingTargetMode(TrainingTargetMode::Standard)) {
            lastReport_ = {LandmarkEncounterResult::RetryUnavailable, 0.0f};
            return lastReport_;
        }
    } else {
        combatSandbox.ResetTrainingSession();
    }
    shadowbladeActions.ResetTransientStatePreservingLoadout();
    state_ = LandmarkEncounterState::Active;
    activationElapsedSeconds_ = combatSandbox.ElapsedSecondsPrecise();
    activationTrainingStats_ = combatSandbox.Stats();
    activationCombatOwner_ = &combatSandbox;
    activationActionsOwner_ = &shadowbladeActions;
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
