#pragma once

#include "Engine/Scene/CharacterProgression.h"
#include "Engine/Scene/ExplorationFieldGuide.h"

#include <algorithm>
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

enum class ManaReactorDifficulty : std::uint8_t {
    Guided,
    Standard,
    Critical,
};

enum class ManaReactorProtocol : std::uint8_t {
    Baseline,
    ThermalSink,
    StabilityMesh,
    SurgeHarness,
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
    ManaReactorDifficulty difficulty{ManaReactorDifficulty::Standard};
    ManaReactorProtocol protocol{ManaReactorProtocol::Baseline};
    int stagesCleared{};
    int totalStages{3};
    int progressPercent{};
    int objectiveProgress{};
    int objectiveRequired{2};
    int heat{};
    int stability{};
    int optionalTargetsComplete{};
    int retries{};
    int protocolRank{1};
    int emergencyVentsUsed{};
    int precisionChain{};
    int bestPrecisionChain{};
    int precisionPulses{};
    bool emergencyVentAvailable{true};
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
    int protocolRank{1};
    int emergencyVentsUsed{};
    int bestPrecisionChain{};
    int precisionPulses{};
    ManaReactorMode mode{ManaReactorMode::Expedition};
    ManaReactorDifficulty difficulty{ManaReactorDifficulty::Standard};
    ManaReactorProtocol protocol{ManaReactorProtocol::Baseline};
};

struct ManaReactorRewardReport {
    bool granted{};
    ProgressionRewardReport progression{};
};

struct ManaReactorControlPreview {
    bool valid{};
    ManaReactorControlResult result{ManaReactorControlResult::Rejected};
    int projectedObjectiveProgress{};
    int projectedHeat{};
    int projectedStability{};
    int projectedPrecisionChain{};
    bool protocolApplied{};
    bool precisionPulse{};
};

class ManaReactorExpedition {
public:
    static constexpr int StageCount = 3;
    static constexpr int TotalObjectiveUnits = 8;
    static constexpr int MaximumHeat = 100;
    static constexpr int MaximumStability = 100;
    static constexpr int MaximumProtocolRank = 3;
    static constexpr int MaximumPrecisionChain = 4;
    static constexpr int EmergencyVentCooling = 25;
    static constexpr int EmergencyVentStabilityCost = 5;
    static constexpr int FirstClearExperience = 350;
    static constexpr int FirstClearMastery = 35;
    static constexpr int FirstClearEnhancementMaterials = 50;

    bool TryBegin(const ExplorationFieldGuide& guide,
        ManaReactorMode mode = ManaReactorMode::Expedition,
        ManaReactorDifficulty difficulty = ManaReactorDifficulty::Standard,
        ManaReactorProtocol protocol = ManaReactorProtocol::Baseline) {
        if (active_ || !ValidMode(mode) || !ValidDifficulty(difficulty)
            || !ValidProtocol(protocol)
            || !guide.OperationProgress(FieldOperation::RiftInvestigation).Complete()) {
            return false;
        }
        mode_ = mode;
        difficulty_ = difficulty;
        protocol_ = protocol;
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
    ManaReactorDifficulty Difficulty() const { return difficulty_; }
    ManaReactorProtocol Protocol() const { return protocol_; }
    int Heat() const { return heat_; }
    int Stability() const { return stability_; }
    int ObjectiveProgress() const { return objectiveProgress_; }
    int ObjectiveRequired() const { return ObjectiveRequiredFor(stage_); }
    int StagesCleared() const { return stagesCleared_; }
    int OptionalTargetsComplete() const { return optionalTargetsComplete_; }
    int Retries() const { return retries_; }
    int BestExpeditionScore() const { return bestExpeditionScore_; }
    int ProtocolRank() const { return protocolRank_; }
    bool EmergencyVentAvailable() const { return emergencyVentAvailable_; }
    int EmergencyVentsUsed() const { return emergencyVentsUsed_; }
    int PrecisionChain() const { return precisionChain_; }
    int BestPrecisionChain() const { return bestPrecisionChain_; }
    int PrecisionPulses() const { return precisionPulses_; }

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
        snapshot.difficulty = difficulty_;
        snapshot.protocol = protocol_;
        snapshot.stagesCleared = stagesCleared_;
        snapshot.progressPercent = ProgressPercent();
        snapshot.objectiveProgress = objectiveProgress_;
        snapshot.objectiveRequired = ObjectiveRequired();
        snapshot.heat = heat_;
        snapshot.stability = stability_;
        snapshot.optionalTargetsComplete = optionalTargetsComplete_;
        snapshot.retries = retries_;
        snapshot.protocolRank = protocolRank_;
        snapshot.emergencyVentsUsed = emergencyVentsUsed_;
        snapshot.precisionChain = precisionChain_;
        snapshot.bestPrecisionChain = bestPrecisionChain_;
        snapshot.precisionPulses = precisionPulses_;
        snapshot.emergencyVentAvailable = emergencyVentAvailable_;
        snapshot.active = active_;
        snapshot.failed = failed_;
        snapshot.complete = complete_;
        return snapshot;
    }

    ManaReactorControlPreview PreviewControl(ManaReactorControl control) const {
        ManaReactorControlPreview preview = EvaluateControl(control);
        if (!preview.valid) return preview;

        ManaReactorExpedition projected = *this;
        preview.result = projected.ApplyControl(control);
        const ManaReactorSnapshot projectedState = projected.Snapshot();
        preview.projectedObjectiveProgress = projectedState.objectiveProgress;
        preview.projectedHeat = projectedState.heat;
        preview.projectedStability = projectedState.stability;
        preview.projectedPrecisionChain = projectedState.precisionChain;
        return preview;
    }

    ManaReactorControlResult ApplyControl(ManaReactorControl control) {
        const ManaReactorControlPreview preview = EvaluateControl(control);
        if (!preview.valid) return ManaReactorControlResult::Rejected;

        heat_ = preview.projectedHeat;
        stability_ = preview.projectedStability;
        objectiveProgress_ = preview.projectedObjectiveProgress;
        stagePeakHeat_ = std::max(stagePeakHeat_, heat_);
        stageMinimumStability_ = std::min(stageMinimumStability_, stability_);
        runPeakHeat_ = std::max(runPeakHeat_, heat_);
        runMinimumStability_ = std::min(runMinimumStability_, stability_);

        if (preview.result == ManaReactorControlResult::Failed) {
            failed_ = true;
            ResetPrecisionChain();
            return preview.result;
        }

        precisionChain_ = preview.projectedPrecisionChain;
        bestPrecisionChain_ = std::max(bestPrecisionChain_, precisionChain_);
        if (preview.precisionPulse && precisionPulses_ < MaximumTrackedPrecisionPulses) {
            ++precisionPulses_;
        }
        hasLastControl_ = true;
        lastControl_ = control;

        if (preview.result == ManaReactorControlResult::StageCleared
            || preview.result == ManaReactorControlResult::RunCompleted) {
            return ClearCurrentStage();
        }
        return preview.result;
    }

    bool UseEmergencyVent() {
        if (!active_ || complete_ || failed_ || !emergencyVentAvailable_
            || heat_ <= 0 || stability_ <= EmergencyVentStabilityCost) {
            return false;
        }

        heat_ = std::max(0, heat_ - EmergencyVentCooling);
        stability_ = std::max(0, stability_ - EmergencyVentStabilityCost);
        stageMinimumStability_ = std::min(stageMinimumStability_, stability_);
        runMinimumStability_ = std::min(runMinimumStability_, stability_);
        emergencyVentAvailable_ = false;
        if (emergencyVentsUsed_ < MaximumTrackedEmergencyVents) ++emergencyVentsUsed_;
        ResetPrecisionChain();
        return true;
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
        summary.difficulty = difficulty_;
        summary.protocol = protocol_;
        summary.stagesCleared = stagesCleared_;
        summary.optionalTargetsComplete = optionalTargetsComplete_;
        summary.retries = retries_;
        summary.peakHeat = runPeakHeat_;
        summary.minimumStability = runMinimumStability_;
        summary.protocolRank = protocolRank_;
        summary.emergencyVentsUsed = emergencyVentsUsed_;
        summary.bestPrecisionChain = bestPrecisionChain_;
        summary.precisionPulses = precisionPulses_;
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
    static constexpr int MaximumTrackedEmergencyVents = 1000000;
    static constexpr int MaximumTrackedPrecisionPulses = 1000000;

    static constexpr bool ValidMode(ManaReactorMode mode) {
        return mode == ManaReactorMode::Expedition || mode == ManaReactorMode::Calibration;
    }

    static constexpr bool ValidDifficulty(ManaReactorDifficulty difficulty) {
        return difficulty == ManaReactorDifficulty::Guided
            || difficulty == ManaReactorDifficulty::Standard
            || difficulty == ManaReactorDifficulty::Critical;
    }

    static constexpr bool ValidProtocol(ManaReactorProtocol protocol) {
        return protocol == ManaReactorProtocol::Baseline
            || protocol == ManaReactorProtocol::ThermalSink
            || protocol == ManaReactorProtocol::StabilityMesh
            || protocol == ManaReactorProtocol::SurgeHarness;
    }

    static constexpr bool ValidControl(ManaReactorControl control) {
        return control == ManaReactorControl::Stabilize
            || control == ManaReactorControl::Balance
            || control == ManaReactorControl::Overdrive;
    }

    static constexpr int ClampPercent(int value) {
        return value < 0 ? 0 : (value > 100 ? 100 : value);
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

    static constexpr HazardPressure BasePressureFor(ManaReactorStage stage) {
        switch (stage) {
        case ManaReactorStage::IntakeBay: return {4, 0};
        case ManaReactorStage::CoolingLattice: return {8, -3};
        case ManaReactorStage::CoreChamber: return {12, -6};
        case ManaReactorStage::Complete: return {};
        }
        return {};
    }

    static constexpr HazardPressure PressureFor(
        ManaReactorStage stage, ManaReactorDifficulty difficulty) {
        HazardPressure pressure = BasePressureFor(stage);
        if (difficulty == ManaReactorDifficulty::Guided) {
            pressure.heat = pressure.heat > 4 ? pressure.heat - 4 : 0;
            pressure.stability = std::min(0, pressure.stability + 2);
        } else if (difficulty == ManaReactorDifficulty::Critical) {
            pressure.heat += 6;
            pressure.stability -= 4;
        }
        return pressure;
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

    int ProjectedPrecisionChain(ManaReactorControl control) const {
        if (protocol_ == ManaReactorProtocol::Baseline) return 0;
        if (!hasLastControl_) return 1;
        if (lastControl_ == control) return 1;
        return std::min(MaximumPrecisionChain, precisionChain_ + 1);
    }

    void ApplyProtocolAdjustment(ManaReactorControl control, int& progress,
        int& heatDelta, int& stabilityDelta) const {
        switch (protocol_) {
        case ManaReactorProtocol::Baseline:
            break;
        case ManaReactorProtocol::ThermalSink:
            heatDelta -= 3 * protocolRank_;
            break;
        case ManaReactorProtocol::StabilityMesh:
            stabilityDelta += 3 * protocolRank_;
            break;
        case ManaReactorProtocol::SurgeHarness:
            if (control == ManaReactorControl::Overdrive) {
                heatDelta += 2 * protocolRank_;
                if (protocolRank_ >= 2) ++progress;
            }
            break;
        }
    }

    ManaReactorControlPreview EvaluateControl(ManaReactorControl control) const {
        ManaReactorControlPreview preview{};
        if (!active_ || complete_ || failed_ || !ValidControl(control)) return preview;

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
            return preview;
        }

        ApplyProtocolAdjustment(control, progress, heatDelta, stabilityDelta);
        const int projectedChain = ProjectedPrecisionChain(control);
        const bool precisionPulse =
            protocol_ != ManaReactorProtocol::Baseline && projectedChain >= 3;
        if (precisionPulse) {
            heatDelta -= 4;
            stabilityDelta += 2;
        }

        const HazardPressure pressure = PressureFor(stage_, difficulty_);
        const int projectedHeat = ClampPercent(heat_ + heatDelta + pressure.heat);
        const int projectedStability =
            ClampPercent(stability_ + stabilityDelta + pressure.stability);
        const int required = ObjectiveRequired();
        const int projectedProgress = std::min(required, objectiveProgress_ + progress);

        preview.valid = true;
        preview.projectedObjectiveProgress = projectedProgress;
        preview.projectedHeat = projectedHeat;
        preview.projectedStability = projectedStability;
        preview.projectedPrecisionChain = projectedChain;
        preview.protocolApplied = protocol_ != ManaReactorProtocol::Baseline;
        preview.precisionPulse = precisionPulse;

        if (projectedHeat >= MaximumHeat || projectedStability <= 0) {
            preview.result = ManaReactorControlResult::Failed;
        } else if (projectedProgress >= required) {
            preview.result = stage_ == ManaReactorStage::CoreChamber
                ? ManaReactorControlResult::RunCompleted
                : ManaReactorControlResult::StageCleared;
        } else {
            preview.result = ManaReactorControlResult::Applied;
        }
        return preview;
    }

    void ResetPrecisionChain() {
        precisionChain_ = 0;
        hasLastControl_ = false;
        lastControl_ = ManaReactorControl::Stabilize;
    }

    void ResetStageState(ManaReactorStage stage) {
        objectiveProgress_ = 0;
        heat_ = BaselineHeat(stage);
        stability_ = BaselineStability(stage);
        stagePeakHeat_ = heat_;
        stageMinimumStability_ = stability_;
        runPeakHeat_ = std::max(runPeakHeat_, heat_);
        runMinimumStability_ = std::min(runMinimumStability_, stability_);
        emergencyVentAvailable_ = true;
        ResetPrecisionChain();
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
        protocolRank_ = 1;
        emergencyVentsUsed_ = 0;
        bestPrecisionChain_ = 0;
        precisionPulses_ = 0;
        ResetStageState(stage_);
    }

    bool CurrentOptionalTargetMet() const {
        return stagePeakHeat_ <= OptionalHeatLimit(stage_)
            && stageMinimumStability_ >= OptionalStabilityFloor(stage_);
    }

    ManaReactorControlResult ClearCurrentStage() {
        const bool optionalMet = CurrentOptionalTargetMet();
        if (optionalMet) {
            ++optionalTargetsComplete_;
            if (protocol_ != ManaReactorProtocol::Baseline
                && protocolRank_ < MaximumProtocolRank) {
                ++protocolRank_;
            }
        }
        ++stagesCleared_;
        stage_ = NextStage(stage_);
        objectiveProgress_ = 0;
        if (stage_ == ManaReactorStage::Complete) {
            active_ = false;
            complete_ = true;
            emergencyVentAvailable_ = false;
            ResetPrecisionChain();
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
        int difficultyAdjustment = 0;
        if (difficulty_ == ManaReactorDifficulty::Guided) {
            difficultyAdjustment = -150;
        } else if (difficulty_ == ManaReactorDifficulty::Critical) {
            difficultyAdjustment = 150;
        }
        const int protocolAdjustment = protocol_ == ManaReactorProtocol::Baseline
            ? 0
            : (protocolRank_ - 1) * 40 + precisionPulses_ * 30;
        const std::int64_t value = 1000
            + static_cast<std::int64_t>(optionalTargetsComplete_) * 150
            - static_cast<std::int64_t>(retries_) * 75
            - static_cast<std::int64_t>(runPeakHeat_) * 2
            - static_cast<std::int64_t>(100 - runMinimumStability_)
            + static_cast<std::int64_t>(difficultyAdjustment)
            + static_cast<std::int64_t>(protocolAdjustment)
            - static_cast<std::int64_t>(emergencyVentsUsed_) * 60;
        if (value <= 0) return 0;
        return static_cast<int>(std::min<std::int64_t>(
            value, std::numeric_limits<int>::max()));
    }

    bool runStarted_{};
    bool active_{};
    bool failed_{};
    bool complete_{};
    ManaReactorMode mode_{ManaReactorMode::Expedition};
    ManaReactorDifficulty difficulty_{ManaReactorDifficulty::Standard};
    ManaReactorProtocol protocol_{ManaReactorProtocol::Baseline};
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
    int protocolRank_{1};
    bool emergencyVentAvailable_{true};
    int emergencyVentsUsed_{};
    int precisionChain_{};
    int bestPrecisionChain_{};
    int precisionPulses_{};
    bool hasLastControl_{};
    ManaReactorControl lastControl_{ManaReactorControl::Stabilize};
};

} // namespace Astral::Scene