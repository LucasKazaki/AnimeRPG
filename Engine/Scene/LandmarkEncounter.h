#pragma once

#include "Engine/Scene/CombatSandbox.h"
#include "Engine/Scene/LandmarkInteraction.h"
#include "Engine/Scene/ShadowbladeActions.h"

namespace Astral::Scene {

enum class LandmarkEncounterState {
    Locked,
    Active,
    Completed,
};

enum class LandmarkEncounterResult {
    None,
    DiscoveryRequired,
    TargetUnavailable,
    Activated,
    AlreadyStarted,
    Completed,
    RetryUnavailable,
    Retried,
};

enum class EncounterGrade {
    None,
    Bronze,
    Silver,
    Gold,
};

struct LandmarkEncounterReport {
    LandmarkEncounterResult result{LandmarkEncounterResult::None};
    float rewardApplied{};
    EncounterGrade grade{EncounterGrade::None};
    float completionSeconds{};
};

class LandmarkEncounter {
public:
    static constexpr LandmarkKind EncounterLandmark = LandmarkKind::LincolnMemorial;
    static constexpr float CompletionReward = 30.0f;
    static constexpr double GoldTimeSeconds = 3.0;
    static constexpr double SilverTimeSeconds = 6.0;

    LandmarkEncounterReport TryActivate(const LandmarkInteractionReport& interaction,
        const CombatSandbox& combatSandbox);
    bool Update(const CombatSandbox& combatSandbox, ShadowbladeActions& shadowbladeActions);
    LandmarkEncounterReport Retry(CombatSandbox& combatSandbox,
        ShadowbladeActions& shadowbladeActions);

    LandmarkEncounterState State() const { return state_; }
    bool CompletionRewardGranted() const { return completionRewardGranted_; }
    const LandmarkEncounterReport& LastReport() const { return lastReport_; }

private:
    static EncounterGrade GradeForSeconds(double seconds);

    LandmarkEncounterState state_{LandmarkEncounterState::Locked};
    LandmarkEncounterReport lastReport_{};
    double activationElapsedSeconds_{};
    bool completionRewardGranted_{};
};

} // namespace Astral::Scene
