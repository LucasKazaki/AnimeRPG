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
};

struct LandmarkEncounterReport {
    LandmarkEncounterResult result{LandmarkEncounterResult::None};
    float rewardApplied{};
};

class LandmarkEncounter {
public:
    static constexpr LandmarkKind EncounterLandmark = LandmarkKind::LincolnMemorial;
    static constexpr float CompletionReward = 30.0f;

    LandmarkEncounterReport TryActivate(const LandmarkInteractionReport& interaction,
        const CombatSandbox& combatSandbox);
    bool Update(const CombatSandbox& combatSandbox, ShadowbladeActions& shadowbladeActions);

    LandmarkEncounterState State() const { return state_; }
    const LandmarkEncounterReport& LastReport() const { return lastReport_; }

private:
    LandmarkEncounterState state_{LandmarkEncounterState::Locked};
    LandmarkEncounterReport lastReport_{};
};

} // namespace Astral::Scene
