#pragma once

#include "Engine/Scene/CharacterProgression.h"
#include "Engine/Scene/ShadowbladeActions.h"
#include "Engine/Scene/WorldBlockout.h"

#include <array>
#include <cstddef>

namespace Astral::Scene {

enum class LandmarkInteractionResult {
    None,
    OutOfRange,
    Discovered,
    AlreadyVisited,
};

enum class LandmarkObjectiveStage {
    DiscoverLincoln,
    DiscoverReflectingPool,
    DiscoverWashingtonMonument,
    Complete,
};

struct LandmarkInteractionReport {
    LandmarkInteractionResult result{LandmarkInteractionResult::None};
    LandmarkKind landmark{LandmarkKind::LincolnMemorial};
    float rewardApplied{};
    ProgressionRewardReport progressionReward{};
};

class LandmarkInteraction {
public:
    static constexpr float ProximityRadius = 3.0f;
    static constexpr float LincolnReward = 20.0f;
    static constexpr float ObjectiveCompletionReward = 15.0f;
    static constexpr float OrderedResonanceReward = 10.0f;
    static constexpr int ObjectiveExperienceReward = 180;
    static constexpr int ObjectiveMasteryReward = 40;
    static constexpr int ObjectiveEnhancementMaterialReward = 15;
    static constexpr std::size_t LedgerCapacity = 3;

    bool UpdateSelection(const Math::Vec3& playerPosition, const WorldBlockout& world);
    LandmarkInteractionReport TryInteract(const Math::Vec3& playerPosition,
        const WorldBlockout& world, ShadowbladeActions& shadowbladeActions);
    void SetCharacterProgression(CharacterProgression* progression) { progression_ = progression; }

    bool HasSelection() const { return selectedIndex_ < LedgerCapacity; }
    std::size_t SelectedIndex() const { return selectedIndex_; }
    LandmarkKind SelectedKind() const;
    bool IsVisited(LandmarkKind kind) const;
    std::size_t VisitedCount() const;
    std::size_t ObjectiveProgress() const;
    LandmarkObjectiveStage CurrentObjective() const;
    bool ObjectiveComplete() const {
        return CurrentObjective() == LandmarkObjectiveStage::Complete;
    }
    bool ObjectiveCompletionRewardGranted() const {
        return objectiveCompletionRewardGranted_;
    }
    std::size_t OrderedDiscoveryProgress() const { return orderedDiscoveryProgress_; }
    bool OrderedSequenceIntact() const { return orderedSequenceIntact_; }
    bool OrderedResonanceRewardGranted() const { return orderedResonanceRewardGranted_; }
    const LandmarkInteractionReport& LastReport() const { return lastReport_; }

private:
    static std::size_t IndexOf(LandmarkKind kind);

    std::array<bool, LedgerCapacity> visited_{};
    std::size_t selectedIndex_{LedgerCapacity};
    bool objectiveCompletionRewardGranted_{};
    std::size_t orderedDiscoveryProgress_{};
    bool orderedSequenceIntact_{true};
    bool orderedResonanceRewardGranted_{};
    CharacterProgression* progression_{}; // Non-owning; caller controls the progression lifetime.
    LandmarkInteractionReport lastReport_{};
};

} // namespace Astral::Scene
