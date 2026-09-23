#pragma once

#include "Engine/Scene/CharacterProgression.h"
#include "Engine/Scene/ExplorationFieldGuide.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

namespace Astral::Scene {

enum class ManaReactorStage : std::uint8_t {
    IntakeBay,
    CoolingLattice,
    CoreChamber,
    Complete,
};

enum class ManaReactorHazard : std::uint8_t {
    CondenserLeak,
    ArcSurge,
    CoreInstability,
    None,
};

enum class ManaReactorControl : std::uint8_t {
    Stabilize,
    Balance,
    Overdrive,
};

enum class ManaReactorMode : std::uint8_t {
    Expedition,
    Calibration,
};

enum class ManaReactorControlResult : std::uint8_t {
    Rejected,
    Applied,
    StageCleared,
    RunCompleted,
    Failed,
};

enum class ManaReactorGrade : std::uint8_t {
    Unranked,
    Bronze,
    Silver,
    Gold,
};

struct ManaReactorSnapshot {
    ManaReactorStage stage{ManaReactorStage::IntakeBay};
    ManaReactorHazard hazard{ManaReactorHazard::CondenserLeak};
    ManaReactorMode mode{ManaReactorMode::Expedition};
    int stagesCleared{};
    int totalStages{3};
    int progressPercent{};
    int objectiveProgress{};
    int objectiveRequired{2};
    int heat{};
    int stability{};
    int optionalTargetsComplete{};
    int retries{};
    bool active{};
    bool failed{};
    bool complete{};
};

struct ManaReactorCompletion {
    ManaReactorGrade grade{ManaReactorGrade::Unranked};
    int score{};
    int stagesCleared{};
    int optionalTargetsComplete{};
    int retries{};
    int peakHeat{};
    int minimumStability{100};
    ManaReactorMode mode{ManaReactorMode::Expedition};
};

struct ManaReactorRewardReport {
    bool granted{};
    ProgressionRewardReport progression{};
};

class ManaReactorExpedition {
public:
    static constexpr int StageCount = 3;
    static constexpr int TotalObjectiveUnits = 8;
    static constexpr int MaximumHeat = 100;
    static constexpr int MaximumStability = 100;
    static constexpr int FirstClearExperience = 350;
    static constexpr int FirstClearMastery = 35;
    static constexpr int FirstClearEnhancementMaterials = 50;

    bool TryBegin(const ExplorationFieldGuide& guide,
        ManaReactorMode mode = ManaReactorMode::Expedition) {
        if (active_ || !ValidMode(mode)
            || !guide.OperationProgress(FieldOperation::RiftInvestigation).Complete()) {
            return false;
        }
        mode_ = mode;
        runStarted_ = true;
        ResetRunState();
        return true;
    }

    bool Active() const { return active_; }
    bool Failed() const { return failed_; }
    bool Complete() const { return complete_; }
    ManaReactorStage CurrentStage() const { return stage_; }
    ManaReactorHazard CurrentHazard() const { return HazardFor(stage_); }
    ManaReactorMode Mode() const { return mode_; }
    int Heat() const { return heat_; }
    int Stability() const { return stability_; }
    int ObjectiveProgress() const { return objectiveProgress_; }
    int ObjectiveRequired() const { return ObjectiveRequiredFor(stage_); }
    int StagesCleared() const { return stagesCleared_; }
    int OptionalTargetsComplete() const { return optionalTargetsComplete_; }
    int Retries() const { return retries_; }
    int BestExpeditionScore() const { return bestExpeditionScore_; }

    int ProgressPercent() const {
        if (complete_) return 100;
        const int units = CompletedObjectiveUnits() + objectiveProgress_;
        return std::min(99, std::max(0, units * 100 / TotalObjectiveUnits));
    }

    ManaReactorSnapshot Snapshot() const {
        ManaReactorSnapshot snapshot{};
        snapshot.stage = stage_;
        snapshot.hazard = HazardFor(stage_);
        snapshot.mode = mode_;
        snapshot.stagesCleared = stagesCleared_;
        snapshot.progressPercent = ProgressPercent();
        snapshot.objectiveProgress = objectiveProgress_;
        snapshot.objectiveRequired = ObjectiveRequired();
        snapshot.heat = heat_;
        snapshot.stability = stability_;
        snapshot.optionalTargetsComplete = optionalTargetsComplete_;
        snapshot.retries = retries_;
        snapshot.active = active_;
        snapshot.failed = failed_;
        snapshot.complete = complete_;
        return snapshot;
    }

    ManaReactorControlResult ApplyControl(ManaReactorControl control) {
        if (!active_ || complete_ || failed_ || !ValidControl(control)) {
            return ManaReactorControlResult::Rejected;
        }

        int progress = 0;
        int heatDelta = 0;
        int stabilityDelta = 0;
        switch (control) {
        case ManaReactorControl::Stabilize:
            progress = 1;
            heatDelta = -18;
            stabilityDelta = 8;
            break;
        case ManaReactorControl::Balance:
            progress = 1;
            heatDelta = -8;
            break;
        case ManaReactorControl::Overdrive:
            progress = 2;
            heatDelta = 18;
            stabilityDelta = -10;
            break;
        default:
            return ManaReactorControlResult::Rejected;
        }

        const HazardPressure pressure = PressureFor(stage_);
        heat_ = ClampPercent(heat_ + heatDelta + pressure.heat);
        stability_ = ClampPercent(stability_ + stabilityDelta + pressure.stability);
        stagePeakHeat_ = std::max(stagePeakHeat_, heat_);
        stageMinimumStability_ = std::min(stageMinimumStability_, stability_);
        runPeakHeat_ = std::max(runPeakHeat_, heat_);
        runMinimumStability_ = std::min(runMinimumStability_, stability_);

        const int required = ObjectiveRequired();
        objectiveProgress_ = std::min(required, objectiveProgress_ + progress);

        if (heat_ >= MaximumHeat || stability_ <= 0) {
            failed_ = true;
            return ManaReactorControlResult::Failed;
        }
        if (objectiveProgress_ < required) {
            return ManaReactorControlResult::Applied;
        }
        return ClearCurrentStage();
    }

    bool RetryCurrentStage() {
        if (!runStarted_ || !active_ || complete_) return false;
        if (retries_ < MaximumTrackedRetries) ++retries_;
        failed_ = false;
        ResetStageState(stage_);
        return true;
    }

    bool StartOver() {
        if (!runStarted_) return false;
        ResetRunState();
        return true;
    }

    ManaReactorCompletion CompletionSummary() const {
        ManaReactorCompletion summary{};
        summary.mode = mode_;
        summary.stagesCleared = stagesCleared_;
        summary.optionalTargetsComplete = optionalTargetsComplete_;
        summary.retries = retries_;
        summary.peakHeat = runPeakHeat_;
        summary.minimumStability = runMinimumStability_;
        if (!complete_) return summary;

        if (optionalTargetsComplete_ == StageCount && retries_ == 0
            && runPeakHeat_ <= 90 && runMinimumStability_ >= 60) {
            summary.grade = ManaReactorGrade::Gold;
        } else if (optionalTargetsComplete_ >= 1 && retries_ <= 2) {
            summary.grade = ManaReactorGrade::Silver;
        } else {
            summary.grade = ManaReactorGrade::Bronze;
        }
        summary.score = Score();
        return summary;
    }

    ManaReactorRewardReport ClaimFirstClearReward(CharacterProgression& progression) {
        ManaReactorRewardReport report{};
        if (!complete_ || mode_ != ManaReactorMode::Expedition) return report;
        report.progression = progression.ClaimManaReactorFirstClearReward(
            FirstClearExperience, FirstClearMastery, FirstClearEnhancementMaterials,
            report.granted);
        return report;
    }

private:
    struct HazardPressure {
        int heat{};
        int stability{};
    };

    static constexpr int MaximumTrackedRetries = 1000000;

    static constexpr bool ValidMode(ManaReactorMode mode) {
        return mode == ManaReactorMode::Expedition || mode == ManaReactorMode::Calibration;
    }

    static constexpr bool ValidControl(ManaReactorControl control) {
        return control == ManaReactorControl::Stabilize
            || control == ManaReactorControl::Balance
            || control == ManaReactorControl::Overdrive;
    }

    static constexpr int ClampPercent(int value) {
        return value < 0 ? 0 : (value > 100 ? 100 : value);
    }

    static constexpr int StageIndex(ManaReactorStage stage) {
        switch (stage) {
        case ManaReactorStage::IntakeBay: return 0;
        case ManaReactorStage::CoolingLattice: return 1;
        case ManaReactorStage::CoreChamber: return 2;
        case ManaReactorStage::Complete: return StageCount;
        }
        return StageCount + 1;
    }

    static constexpr int ObjectiveRequiredFor(ManaReactorStage stage) {
        switch (stage) {
        case ManaReactorStage::IntakeBay: return 2;
        case ManaReactorStage::CoolingLattice: return 3;
        case ManaReactorStage::CoreChamber: return 3;
        case ManaReactorStage::Complete: return 0;
        }
        return 0;
    }

    static constexpr ManaReactorHazard HazardFor(ManaReactorStage stage) {
        switch (stage) {
        case ManaReactorStage::IntakeBay: return ManaReactorHazard::CondenserLeak;
        case ManaReactorStage::CoolingLattice: return ManaReactorHazard::ArcSurge;
        case ManaReactorStage::CoreChamber: return ManaReactorHazard::CoreInstability;
        case ManaReactorStage::Complete: return ManaReactorHazard::None;
        }
        return ManaReactorHazard::None;
    }

    static constexpr HazardPressure PressureFor(ManaReactorStage stage) {
        switch (stage) {
        case ManaReactorStage::IntakeBay: return {4, 0};
        case ManaReactorStage::CoolingLattice: return {8, -3};
        case ManaReactorStage::CoreChamber: return {12, -6};
        case ManaReactorStage::Complete: return {};
        }
        return {};
    }

    static constexpr int BaselineHeat(ManaReactorStage stage) {
        switch (stage) {
        case ManaReactorStage::IntakeBay: return 30;
        case ManaReactorStage::CoolingLattice: return 45;
        case ManaReactorStage::CoreChamber: return 60;
        case ManaReactorStage::Complete: return 0;
        }
        return 0;
    }

    static constexpr int BaselineStability(ManaReactorStage stage) {
        switch (stage) {
        case ManaReactorStage::IntakeBay: return 90;
        case ManaReactorStage::CoolingLattice: return 85;
        case ManaReactorStage::CoreChamber: return 80;
        case ManaReactorStage::Complete: return 100;
        }
        return 100;
    }

    static constexpr int OptionalHeatLimit(ManaReactorStage stage) {
        switch (stage) {
        case ManaReactorStage::IntakeBay: return 50;
        case ManaReactorStage::CoolingLattice: return 65;
        case ManaReactorStage::CoreChamber: return 80;
        case ManaReactorStage::Complete: return 0;
        }
        return 0;
    }

    static constexpr int OptionalStabilityFloor(ManaReactorStage stage) {
        switch (stage) {
        case ManaReactorStage::IntakeBay: return 75;
        case ManaReactorStage::CoolingLattice: return 70;
        case ManaReactorStage::CoreChamber: return 65;
        case ManaReactorStage::Complete: return 100;
        }
        return 100;
    }

    static constexpr ManaReactorStage NextStage(ManaReactorStage stage) {
        switch (stage) {
        case ManaReactorStage::IntakeBay: return ManaReactorStage::CoolingLattice;
        case ManaReactorStage::CoolingLattice: return ManaReactorStage::CoreChamber;
        case ManaReactorStage::CoreChamber: return ManaReactorStage::Complete;
        case ManaReactorStage::Complete: return ManaReactorStage::Complete;
        }
        return ManaReactorStage::Complete;
    }

    int CompletedObjectiveUnits() const {
        switch (stage_) {
        case ManaReactorStage::IntakeBay: return 0;
        case ManaReactorStage::CoolingLattice: return 2;
        case ManaReactorStage::CoreChamber: return 5;
        case ManaReactorStage::Complete: return TotalObjectiveUnits;
        }
        return 0;
    }

    void ResetStageState(ManaReactorStage stage) {
        objectiveProgress_ = 0;
        heat_ = BaselineHeat(stage);
        stability_ = BaselineStability(stage);
        stagePeakHeat_ = heat_;
        stageMinimumStability_ = stability_;
        runPeakHeat_ = std::max(runPeakHeat_, heat_);
        runMinimumStability_ = std::min(runMinimumStability_, stability_);
    }

    void ResetRunState() {
        active_ = true;
        failed_ = false;
        complete_ = false;
        stage_ = ManaReactorStage::IntakeBay;
        stagesCleared_ = 0;
        optionalTargetsComplete_ = 0;
        retries_ = 0;
        runPeakHeat_ = 0;
        runMinimumStability_ = 100;
        ResetStageState(stage_);
    }

    bool CurrentOptionalTargetMet() const {
        return stagePeakHeat_ <= OptionalHeatLimit(stage_)
            && stageMinimumStability_ >= OptionalStabilityFloor(stage_);
    }

    ManaReactorControlResult ClearCurrentStage() {
        if (CurrentOptionalTargetMet()) ++optionalTargetsComplete_;
        ++stagesCleared_;
        stage_ = NextStage(stage_);
        objectiveProgress_ = 0;
        if (stage_ == ManaReactorStage::Complete) {
            active_ = false;
            complete_ = true;
            const int score = Score();
            if (mode_ == ManaReactorMode::Expedition) {
                bestExpeditionScore_ = std::max(bestExpeditionScore_, score);
            }
            return ManaReactorControlResult::RunCompleted;
        }
        ResetStageState(stage_);
        return ManaReactorControlResult::StageCleared;
    }

    int Score() const {
        if (!complete_) return 0;
        const std::int64_t value = 1000
            + static_cast<std::int64_t>(optionalTargetsComplete_) * 150
            - static_cast<std::int64_t>(retries_) * 75
            - static_cast<std::int64_t>(runPeakHeat_) * 2
            - static_cast<std::int64_t>(100 - runMinimumStability_);
        if (value <= 0) return 0;
        return static_cast<int>(std::min<std::int64_t>(
            value, std::numeric_limits<int>::max()));
    }

    bool runStarted_{};
    bool active_{};
    bool failed_{};
    bool complete_{};
    ManaReactorMode mode_{ManaReactorMode::Expedition};
    ManaReactorStage stage_{ManaReactorStage::IntakeBay};
    int objectiveProgress_{};
    int stagesCleared_{};
    int heat_{};
    int stability_{100};
    int stagePeakHeat_{};
    int stageMinimumStability_{100};
    int runPeakHeat_{};
    int runMinimumStability_{100};
    int optionalTargetsComplete_{};
    int retries_{};
    int bestExpeditionScore_{};
};

} // namespace Astral::Scene
