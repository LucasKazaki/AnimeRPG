#pragma once

#include "Engine/Scene/ManaReactorExpedition.h"

#include <array>
#include <cstddef>
#include <limits>

namespace Astral::Scene {

enum class ManaReactorMissionRecoveryAction : std::uint8_t {
    None,
    RetryStage,
    ReplayRun,
};

struct ManaReactorMissionRecord {
    bool valid{};
    int score{};
    ManaReactorGrade grade{ManaReactorGrade::Unranked};
    ManaReactorDifficulty difficulty{ManaReactorDifficulty::Standard};
    ManaReactorProtocol protocol{ManaReactorProtocol::Baseline};
};

struct ManaReactorMissionBriefing {
    bool available{};
    ManaReactorSnapshot snapshot{};
    std::array<ManaReactorControlPreview, 3> controlPreviews{};
    bool recommendationAvailable{};
    ManaReactorControl recommendedControl{ManaReactorControl::Balance};
    bool emergencyVentSuggested{};
    ManaReactorMissionRecoveryAction recovery{ManaReactorMissionRecoveryAction::None};
};

// Game-owned coordinator that attaches the already-reviewed Mana Reactor
// expedition domain to a live game owner. It adds no renderer, platform/input,
// save backend, networking, engine timing, or reward currency of its own.
class ManaReactorMission {
public:
    static constexpr std::size_t DifficultyCount = 3;
    static constexpr std::size_t ProtocolCount = 4;
    static constexpr std::size_t RecordCount = DifficultyCount * ProtocolCount;

    bool Begin(const ExplorationFieldGuide& guide,
        ManaReactorMode mode = ManaReactorMode::Expedition,
        ManaReactorDifficulty difficulty = ManaReactorDifficulty::Standard,
        ManaReactorProtocol protocol = ManaReactorProtocol::Baseline) {
        if (!expedition_.TryBegin(guide, mode, difficulty, protocol)) return false;
        hasRunConfiguration_ = true;
        lastMode_ = mode;
        lastDifficulty_ = difficulty;
        lastProtocol_ = protocol;
        return true;
    }

    ManaReactorControlResult ApplyControl(ManaReactorControl control) {
        const ManaReactorControlResult result = expedition_.ApplyControl(control);
        if (result == ManaReactorControlResult::RunCompleted) RecordCompletion();
        return result;
    }

    bool UseEmergencyVent() { return expedition_.UseEmergencyVent(); }
    bool RetryCurrentStage() { return expedition_.RetryCurrentStage(); }

    // QOL-027: replay a completed run with the exact prior mode/difficulty/protocol
    // without requiring the caller to rebuild the configuration. Mid-run replay is
    // intentionally rejected so this cannot become an accidental progress reset.
    bool ReplayCompletedRun() {
        if (!hasRunConfiguration_ || !expedition_.Complete()) return false;
        return expedition_.StartOver();
    }

    ManaReactorRewardReport ClaimFirstClearReward(CharacterProgression& progression) {
        return expedition_.ClaimFirstClearReward(progression);
    }

    ManaReactorMissionBriefing Briefing() const {
        ManaReactorMissionBriefing briefing{};
        briefing.available = hasRunConfiguration_;
        briefing.snapshot = expedition_.Snapshot();
        if (!hasRunConfiguration_) return briefing;

        constexpr std::array<ManaReactorControl, 3> controls{
            ManaReactorControl::Stabilize,
            ManaReactorControl::Balance,
            ManaReactorControl::Overdrive,
        };

        long long bestScore = std::numeric_limits<long long>::min();
        for (std::size_t index = 0; index < controls.size(); ++index) {
            const ManaReactorControlPreview preview = expedition_.PreviewControl(controls[index]);
            briefing.controlPreviews[index] = preview;
            if (!preview.valid || preview.result == ManaReactorControlResult::Failed) continue;

            long long score = static_cast<long long>(preview.projectedObjectiveProgress) * 1000LL
                + static_cast<long long>(preview.projectedStability) * 4LL
                - static_cast<long long>(preview.projectedHeat) * 3LL;
            if (preview.result == ManaReactorControlResult::StageCleared) score += 3000LL;
            if (preview.result == ManaReactorControlResult::RunCompleted) score += 6000LL;
            if (!briefing.recommendationAvailable || score > bestScore) {
                briefing.recommendationAvailable = true;
                briefing.recommendedControl = controls[index];
                bestScore = score;
            }
        }

        briefing.emergencyVentSuggested = expedition_.Active()
            && !expedition_.Failed()
            && expedition_.EmergencyVentAvailable()
            && expedition_.Heat() >= 75
            && expedition_.Stability() > ManaReactorExpedition::EmergencyVentStabilityCost;

        if (expedition_.Failed()) {
            briefing.recovery = ManaReactorMissionRecoveryAction::RetryStage;
        } else if (expedition_.Complete()) {
            briefing.recovery = ManaReactorMissionRecoveryAction::ReplayRun;
        }
        return briefing;
    }

    ManaReactorMissionRecord BestRecord(ManaReactorDifficulty difficulty,
        ManaReactorProtocol protocol) const {
        const std::size_t index = RecordIndex(difficulty, protocol);
        return index < records_.size() ? records_[index] : ManaReactorMissionRecord{};
    }

    const ManaReactorExpedition& Expedition() const { return expedition_; }
    bool HasRunConfiguration() const { return hasRunConfiguration_; }
    ManaReactorMode LastMode() const { return lastMode_; }
    ManaReactorDifficulty LastDifficulty() const { return lastDifficulty_; }
    ManaReactorProtocol LastProtocol() const { return lastProtocol_; }

private:
    static constexpr std::size_t DifficultyIndex(ManaReactorDifficulty difficulty) {
        switch (difficulty) {
        case ManaReactorDifficulty::Guided: return 0;
        case ManaReactorDifficulty::Standard: return 1;
        case ManaReactorDifficulty::Critical: return 2;
        }
        return DifficultyCount;
    }

    static constexpr std::size_t ProtocolIndex(ManaReactorProtocol protocol) {
        switch (protocol) {
        case ManaReactorProtocol::Baseline: return 0;
        case ManaReactorProtocol::ThermalSink: return 1;
        case ManaReactorProtocol::StabilityMesh: return 2;
        case ManaReactorProtocol::SurgeHarness: return 3;
        }
        return ProtocolCount;
    }

    static constexpr std::size_t RecordIndex(ManaReactorDifficulty difficulty,
        ManaReactorProtocol protocol) {
        const std::size_t difficultyIndex = DifficultyIndex(difficulty);
        const std::size_t protocolIndex = ProtocolIndex(protocol);
        if (difficultyIndex >= DifficultyCount || protocolIndex >= ProtocolCount) {
            return RecordCount;
        }
        return difficultyIndex * ProtocolCount + protocolIndex;
    }

    void RecordCompletion() {
        const ManaReactorCompletion completion = expedition_.CompletionSummary();
        // Calibration is a consequence-free practice mode. It must never replace
        // an Expedition personal best even when it uses the same difficulty/protocol.
        if (completion.mode != ManaReactorMode::Expedition
            || completion.grade == ManaReactorGrade::Unranked) {
            return;
        }
        const std::size_t index = RecordIndex(completion.difficulty, completion.protocol);
        if (index >= records_.size()) return;
        ManaReactorMissionRecord& record = records_[index];
        if (!record.valid || completion.score > record.score) {
            record.valid = true;
            record.score = completion.score;
            record.grade = completion.grade;
            record.difficulty = completion.difficulty;
            record.protocol = completion.protocol;
        }
    }

    ManaReactorExpedition expedition_{};
    std::array<ManaReactorMissionRecord, RecordCount> records_{};
    bool hasRunConfiguration_{};
    ManaReactorMode lastMode_{ManaReactorMode::Expedition};
    ManaReactorDifficulty lastDifficulty_{ManaReactorDifficulty::Standard};
    ManaReactorProtocol lastProtocol_{ManaReactorProtocol::Baseline};
};

} // namespace Astral::Scene
