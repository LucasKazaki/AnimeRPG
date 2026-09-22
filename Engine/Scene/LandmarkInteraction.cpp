#include "Engine/Scene/LandmarkInteraction.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Astral::Scene {
namespace {
float AxisDistance(float point, float center, float dimension) {
    return std::max(0.0f, std::abs(point - center) - dimension * 0.5f);
}
}

bool LandmarkInteraction::UpdateSelection(const Math::Vec3& playerPosition,
    const WorldBlockout& world) {
    const Math::Vec3 ground = world.GroundPosition(playerPosition);
    std::size_t nearest = LedgerCapacity;
    float nearestDistanceSquared = std::numeric_limits<float>::max();
    for (std::size_t index = 0; index < world.Landmarks().size(); ++index) {
        const LandmarkProxy& landmark = world.Landmarks()[index];
        const float deltaX = AxisDistance(ground.x, landmark.position.x, landmark.dimensions.x);
        const float deltaZ = AxisDistance(ground.z, landmark.position.z, landmark.dimensions.z);
        const float distanceSquared = deltaX * deltaX + deltaZ * deltaZ;
        if (distanceSquared <= ProximityRadius * ProximityRadius
            && distanceSquared < nearestDistanceSquared) {
            nearest = index;
            nearestDistanceSquared = distanceSquared;
        }
    }
    const bool changed = nearest != selectedIndex_;
    selectedIndex_ = nearest;
    return changed;
}

LandmarkInteractionReport LandmarkInteraction::TryInteract(const Math::Vec3& playerPosition,
    const WorldBlockout& world, ShadowbladeActions& shadowbladeActions) {
    UpdateSelection(playerPosition, world);
    if (!HasSelection()) {
        lastReport_ = {LandmarkInteractionResult::OutOfRange,
            LandmarkKind::LincolnMemorial, 0.0f};
        return lastReport_;
    }

    const LandmarkKind kind = world.Landmarks()[selectedIndex_].kind;
    const bool alreadyVisited = visited_[selectedIndex_];
    const bool canAdvanceObjective = objectiveStarted_ && !objectiveVisited_[selectedIndex_];
    if (alreadyVisited && !canAdvanceObjective) {
        lastReport_ = {LandmarkInteractionResult::AlreadyVisited, kind, 0.0f};
        return lastReport_;
    }

    float reward = 0.0f;
    ProgressionRewardReport progressionReward{};
    LandmarkInteractionResult result = LandmarkInteractionResult::Discovered;

    if (!alreadyVisited) {
        visited_[selectedIndex_] = true;
        if (kind == LandmarkKind::LincolnMemorial) {
            reward += shadowbladeActions.RestoreResource(LincolnReward);
        }
    } else {
        result = LandmarkInteractionResult::ObjectiveAdvanced;
    }

    if (canAdvanceObjective) {
        RecordObjectiveVisit(selectedIndex_);
        ApplyObjectiveRewards(shadowbladeActions, reward, progressionReward);
    }

    lastReport_ = {result, kind, reward, progressionReward};
    return lastReport_;
}

bool LandmarkInteraction::SetObjectiveActivationMode(LandmarkObjectiveActivationMode mode) {
    if (mode != LandmarkObjectiveActivationMode::AutoStart
        && mode != LandmarkObjectiveActivationMode::ManualStart) {
        return false;
    }
    if (mode == objectiveActivationMode_) return true;

    const bool objectiveHistory = std::any_of(objectiveVisited_.begin(), objectiveVisited_.end(),
        [](bool visited) { return visited; });
    if (objectiveHistory || objectiveCompletionRewardGranted_
        || orderedResonanceRewardGranted_) {
        return false;
    }
    objectiveActivationMode_ = mode;
    objectiveStarted_ = mode == LandmarkObjectiveActivationMode::AutoStart;
    objectiveVisited_ = {};
    orderedDiscoveryProgress_ = 0;
    orderedSequenceIntact_ = true;
    return true;
}

bool LandmarkInteraction::StartObjective() {
    if (objectiveStarted_) return false;
    objectiveStarted_ = true;
    return true;
}

void LandmarkInteraction::RecordObjectiveVisit(std::size_t index) {
    if (index >= LedgerCapacity || objectiveVisited_[index]) return;

    if (orderedSequenceIntact_) {
        if (index == orderedDiscoveryProgress_) {
            ++orderedDiscoveryProgress_;
        } else {
            orderedSequenceIntact_ = false;
        }
    }
    objectiveVisited_[index] = true;
}

void LandmarkInteraction::ApplyObjectiveRewards(ShadowbladeActions& shadowbladeActions,
    float& reward, ProgressionRewardReport& progressionReward) {
    if (ObjectiveComplete() && !objectiveCompletionRewardGranted_) {
        objectiveCompletionRewardGranted_ = true;
        reward += shadowbladeActions.RestoreResource(ObjectiveCompletionReward);
        if (progression_ != nullptr) {
            progressionReward = progression_->GrantRewards(
                ObjectiveExperienceReward,
                ObjectiveMasteryReward,
                ObjectiveEnhancementMaterialReward);
        }
    }
    if (ObjectiveComplete() && orderedSequenceIntact_
        && orderedDiscoveryProgress_ == LedgerCapacity
        && !orderedResonanceRewardGranted_) {
        orderedResonanceRewardGranted_ = true;
        reward += shadowbladeActions.RestoreResource(OrderedResonanceReward);
    }
}

LandmarkKind LandmarkInteraction::SelectedKind() const {
    switch (selectedIndex_) {
    case 0: return LandmarkKind::LincolnMemorial;
    case 1: return LandmarkKind::ReflectingPool;
    case 2: return LandmarkKind::WashingtonMonument;
    default: return LandmarkKind::LincolnMemorial;
    }
}

bool LandmarkInteraction::IsVisited(LandmarkKind kind) const {
    return visited_[IndexOf(kind)];
}

std::size_t LandmarkInteraction::VisitedCount() const {
    return static_cast<std::size_t>(visited_[0]) + static_cast<std::size_t>(visited_[1])
        + static_cast<std::size_t>(visited_[2]);
}

std::size_t LandmarkInteraction::ObjectiveProgress() const {
    if (!objectiveStarted_) return 0;
    if (!objectiveVisited_[0]) return 0;
    if (!objectiveVisited_[1]) return 1;
    if (!objectiveVisited_[2]) return 2;
    return LedgerCapacity;
}

LandmarkObjectiveStage LandmarkInteraction::CurrentObjective() const {
    if (!objectiveStarted_) return LandmarkObjectiveStage::AwaitingStart;
    switch (ObjectiveProgress()) {
    case 0: return LandmarkObjectiveStage::DiscoverLincoln;
    case 1: return LandmarkObjectiveStage::DiscoverReflectingPool;
    case 2: return LandmarkObjectiveStage::DiscoverWashingtonMonument;
    default: return LandmarkObjectiveStage::Complete;
    }
}

std::size_t LandmarkInteraction::IndexOf(LandmarkKind kind) {
    switch (kind) {
    case LandmarkKind::LincolnMemorial: return 0;
    case LandmarkKind::ReflectingPool: return 1;
    case LandmarkKind::WashingtonMonument: return 2;
    }
    return 0;
}

} // namespace Astral::Scene
