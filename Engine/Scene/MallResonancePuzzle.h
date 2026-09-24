#pragma once

#include "Engine/Scene/CharacterProgression.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace Astral::Scene {

enum class MallResonanceDifficulty : std::uint8_t {
    Survey,
    Standard,
    Expert,
};

enum class MallResonanceAssistMode : std::uint8_t {
    Off,
    Guidance,
};

enum class MallResonanceAnchor : std::uint8_t {
    LincolnMemorial,
    ReflectingPool,
    WashingtonMonument,
};

enum class MallResonanceState : std::uint8_t {
    Locked,
    Active,
    Complete,
};

enum class MallResonanceActionResult : std::uint8_t {
    Invalid,
    NotActive,
    Rotated,
    Mismatch,
    Completed,
};

enum class MallResonanceRank : std::uint8_t {
    None,
    Bronze,
    Silver,
    Gold,
};

struct MallResonanceGuidance {
    bool available{};
    bool readyToStabilize{};
    MallResonanceAnchor anchor{MallResonanceAnchor::LincolnMemorial};
    int rotationsNeeded{};
};

struct MallResonanceActionReport {
    MallResonanceActionResult result{MallResonanceActionResult::Invalid};
    MallResonanceAnchor anchor{MallResonanceAnchor::LincolnMemorial};
    int phase{};
    int moves{};
    int alignedAnchors{};
    MallResonanceRank rank{MallResonanceRank::None};
};

struct MallResonanceRecord {
    bool valid{};
    MallResonanceDifficulty difficulty{MallResonanceDifficulty::Survey};
    MallResonanceRank rank{MallResonanceRank::None};
    int moves{};
    double elapsedSeconds{};
    std::int64_t score{};
};

struct MallResonanceRewardReport {
    bool granted{};
    ProgressionRewardReport progression{};
};

struct MallResonanceBriefing {
    MallResonanceState state{MallResonanceState::Locked};
    MallResonanceDifficulty difficulty{MallResonanceDifficulty::Survey};
    MallResonanceAssistMode assistMode{MallResonanceAssistMode::Off};
    std::array<int, 3> phases{};
    int alignedAnchors{};
    int moves{};
    double elapsedSeconds{};
    int failedStabilizations{};
    MallResonanceGuidance guidance{};
    bool firstClearRewardAvailable{};
    int clearCount{};
};

// Game-owned environmental puzzle for the National Mall. It deliberately owns
// no renderer, platform input, generic engine facility, save backend, or class
// requirement. The live LandmarkInteraction owner supplies quest/progression
// authority while this class keeps deterministic puzzle state and records.
class MallResonancePuzzle {
public:
    static constexpr double MaximumRunSeconds = 3600.0;
    static constexpr int MaximumMoves = 1000000;
    static constexpr int MaximumFailedStabilizations = 1000000;
    static constexpr int FirstClearExperience = 120;
    static constexpr int FirstClearMastery = 25;
    static constexpr int FirstClearEnhancementMaterials = 8;

    bool Begin(bool prerequisiteComplete, MallResonanceDifficulty difficulty,
        MallResonanceAssistMode assistMode = MallResonanceAssistMode::Off) {
        if (!prerequisiteComplete || state_ == MallResonanceState::Active
            || !IsValidDifficulty(difficulty) || !IsValidAssistMode(assistMode)) {
            return false;
        }
        difficulty_ = difficulty;
        assistMode_ = assistMode;
        ResetTransientRun();
        state_ = MallResonanceState::Active;
        return true;
    }

    MallResonanceActionReport Rotate(MallResonanceAnchor anchor) {
        MallResonanceActionReport report{};
        report.anchor = anchor;
        if (!IsValidAnchor(anchor)) return report;
        if (state_ != MallResonanceState::Active) {
            report.result = MallResonanceActionResult::NotActive;
            return report;
        }

        const std::size_t index = AnchorIndex(anchor);
        phases_[index] = (phases_[index] + 1) % 3;
        if (moves_ < MaximumMoves) ++moves_;
        report.result = MallResonanceActionResult::Rotated;
        report.phase = phases_[index];
        report.moves = moves_;
        report.alignedAnchors = AlignedAnchorCount();
        return report;
    }

    bool AdvanceTime(float deltaSeconds) {
        if (state_ != MallResonanceState::Active
            || !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0f) {
            return false;
        }
        elapsedSeconds_ = std::min(MaximumRunSeconds,
            elapsedSeconds_ + static_cast<double>(deltaSeconds));
        return true;
    }

    MallResonanceActionReport TryStabilize() {
        MallResonanceActionReport report{};
        report.moves = moves_;
        report.alignedAnchors = AlignedAnchorCount();
        if (state_ != MallResonanceState::Active) {
            report.result = MallResonanceActionResult::NotActive;
            return report;
        }
        if (report.alignedAnchors != static_cast<int>(phases_.size())) {
            if (failedStabilizations_ < MaximumFailedStabilizations) {
                ++failedStabilizations_;
            }
            report.result = MallResonanceActionResult::Mismatch;
            return report;
        }

        state_ = MallResonanceState::Complete;
        if (clearCount_ < std::numeric_limits<int>::max()) ++clearCount_;
        const MallResonanceRecord record = CurrentRecord();
        StoreBestRecord(record);
        report.result = MallResonanceActionResult::Completed;
        report.rank = record.rank;
        return report;
    }

    // Explicit player-facing recovery for a wedged or unwanted in-progress
    // board. It resets only transient board/run state and cannot erase records,
    // completion history, or an already claimed first-clear reward.
    bool ResetActiveRun() {
        if (state_ != MallResonanceState::Active) return false;
        ResetTransientRun();
        return true;
    }

    MallResonanceBriefing Briefing() const {
        MallResonanceBriefing briefing{};
        briefing.state = state_;
        briefing.difficulty = difficulty_;
        briefing.assistMode = assistMode_;
        briefing.phases = phases_;
        briefing.alignedAnchors = AlignedAnchorCount();
        briefing.moves = moves_;
        briefing.elapsedSeconds = elapsedSeconds_;
        briefing.failedStabilizations = failedStabilizations_;
        briefing.firstClearRewardAvailable = clearCount_ > 0 && !firstClearRewardClaimed_;
        briefing.clearCount = clearCount_;
        if (state_ == MallResonanceState::Active
            && assistMode_ == MallResonanceAssistMode::Guidance) {
            briefing.guidance = BuildGuidance();
        }
        return briefing;
    }

    MallResonanceRecord BestRecord(MallResonanceDifficulty difficulty) const {
        if (!IsValidDifficulty(difficulty)) return {};
        return bestRecords_[DifficultyIndex(difficulty)];
    }

    MallResonanceRewardReport ClaimFirstClearReward(CharacterProgression& progression) {
        MallResonanceRewardReport report{};
        if (clearCount_ <= 0 || firstClearRewardClaimed_) return report;
        firstClearRewardClaimed_ = true;
        report.granted = true;
        report.progression = progression.GrantRewards(
            FirstClearExperience, FirstClearMastery, FirstClearEnhancementMaterials);
        return report;
    }

    MallResonanceState State() const { return state_; }
    MallResonanceDifficulty Difficulty() const { return difficulty_; }
    MallResonanceAssistMode AssistMode() const { return assistMode_; }
    int ClearCount() const { return clearCount_; }
    bool FirstClearRewardClaimed() const { return firstClearRewardClaimed_; }

private:
    struct DifficultyDefinition {
        std::array<int, 3> targets{};
        int parMoves{};
        double goldSeconds{};
        double silverSeconds{};
        int baseScore{};
    };

    static constexpr bool IsValidDifficulty(MallResonanceDifficulty difficulty) {
        return difficulty == MallResonanceDifficulty::Survey
            || difficulty == MallResonanceDifficulty::Standard
            || difficulty == MallResonanceDifficulty::Expert;
    }

    static constexpr bool IsValidAssistMode(MallResonanceAssistMode mode) {
        return mode == MallResonanceAssistMode::Off
            || mode == MallResonanceAssistMode::Guidance;
    }

    static constexpr bool IsValidAnchor(MallResonanceAnchor anchor) {
        return anchor == MallResonanceAnchor::LincolnMemorial
            || anchor == MallResonanceAnchor::ReflectingPool
            || anchor == MallResonanceAnchor::WashingtonMonument;
    }

    static constexpr std::size_t AnchorIndex(MallResonanceAnchor anchor) {
        switch (anchor) {
        case MallResonanceAnchor::ReflectingPool: return 1;
        case MallResonanceAnchor::WashingtonMonument: return 2;
        case MallResonanceAnchor::LincolnMemorial:
        default: return 0;
        }
    }

    static constexpr std::size_t DifficultyIndex(MallResonanceDifficulty difficulty) {
        switch (difficulty) {
        case MallResonanceDifficulty::Standard: return 1;
        case MallResonanceDifficulty::Expert: return 2;
        case MallResonanceDifficulty::Survey:
        default: return 0;
        }
    }

    static constexpr DifficultyDefinition DefinitionFor(MallResonanceDifficulty difficulty) {
        switch (difficulty) {
        case MallResonanceDifficulty::Standard:
            return {{{2, 1, 2}}, 5, 60.0, 120.0, 1300};
        case MallResonanceDifficulty::Expert:
            return {{{2, 2, 1}}, 5, 40.0, 90.0, 1600};
        case MallResonanceDifficulty::Survey:
        default:
            return {{{1, 1, 1}}, 3, 90.0, 180.0, 1000};
        }
    }

    int AlignedAnchorCount() const {
        const auto targets = DefinitionFor(difficulty_).targets;
        int aligned = 0;
        for (std::size_t index = 0; index < phases_.size(); ++index) {
            if (phases_[index] == targets[index]) ++aligned;
        }
        return aligned;
    }

    MallResonanceGuidance BuildGuidance() const {
        MallResonanceGuidance guidance{};
        const auto targets = DefinitionFor(difficulty_).targets;
        for (std::size_t index = 0; index < phases_.size(); ++index) {
            if (phases_[index] == targets[index]) continue;
            guidance.available = true;
            guidance.anchor = static_cast<MallResonanceAnchor>(index);
            guidance.rotationsNeeded = (targets[index] - phases_[index] + 3) % 3;
            return guidance;
        }
        guidance.available = true;
        guidance.readyToStabilize = true;
        guidance.rotationsNeeded = 0;
        return guidance;
    }

    MallResonanceRank RankForCurrentRun() const {
        const DifficultyDefinition definition = DefinitionFor(difficulty_);
        if (moves_ <= definition.parMoves && elapsedSeconds_ <= definition.goldSeconds
            && failedStabilizations_ == 0) {
            return MallResonanceRank::Gold;
        }
        if (moves_ <= definition.parMoves + 3
            && elapsedSeconds_ <= definition.silverSeconds
            && failedStabilizations_ <= 1) {
            return MallResonanceRank::Silver;
        }
        return MallResonanceRank::Bronze;
    }

    std::int64_t ScoreForCurrentRun(MallResonanceRank rank) const {
        const DifficultyDefinition definition = DefinitionFor(difficulty_);
        int rankBonus = 100;
        if (rank == MallResonanceRank::Gold) rankBonus = 300;
        else if (rank == MallResonanceRank::Silver) rankBonus = 200;
        const std::int64_t movePenalty = static_cast<std::int64_t>(moves_) * 10;
        const std::int64_t timePenalty = static_cast<std::int64_t>(elapsedSeconds_);
        const std::int64_t failurePenalty = static_cast<std::int64_t>(failedStabilizations_) * 50;
        const std::int64_t raw = static_cast<std::int64_t>(definition.baseScore + rankBonus)
            - movePenalty - timePenalty - failurePenalty;
        return std::max<std::int64_t>(0, raw);
    }

    MallResonanceRecord CurrentRecord() const {
        MallResonanceRecord record{};
        record.valid = true;
        record.difficulty = difficulty_;
        record.rank = RankForCurrentRun();
        record.moves = moves_;
        record.elapsedSeconds = elapsedSeconds_;
        record.score = ScoreForCurrentRun(record.rank);
        return record;
    }

    static int RankValue(MallResonanceRank rank) {
        switch (rank) {
        case MallResonanceRank::Gold: return 3;
        case MallResonanceRank::Silver: return 2;
        case MallResonanceRank::Bronze: return 1;
        case MallResonanceRank::None:
        default: return 0;
        }
    }

    void StoreBestRecord(const MallResonanceRecord& candidate) {
        MallResonanceRecord& best = bestRecords_[DifficultyIndex(candidate.difficulty)];
        const int candidateRank = RankValue(candidate.rank);
        const int bestRank = RankValue(best.rank);
        const bool better = !best.valid
            || candidateRank > bestRank
            || (candidateRank == bestRank && candidate.score > best.score)
            || (candidateRank == bestRank && candidate.score == best.score
                && candidate.moves < best.moves)
            || (candidateRank == bestRank && candidate.score == best.score
                && candidate.moves == best.moves
                && candidate.elapsedSeconds < best.elapsedSeconds);
        if (better) best = candidate;
    }

    void ResetTransientRun() {
        phases_ = {};
        moves_ = 0;
        elapsedSeconds_ = 0.0;
        failedStabilizations_ = 0;
    }

    MallResonanceState state_{MallResonanceState::Locked};
    MallResonanceDifficulty difficulty_{MallResonanceDifficulty::Survey};
    MallResonanceAssistMode assistMode_{MallResonanceAssistMode::Off};
    std::array<int, 3> phases_{};
    int moves_{};
    double elapsedSeconds_{};
    int failedStabilizations_{};
    std::array<MallResonanceRecord, 3> bestRecords_{};
    int clearCount_{};
    bool firstClearRewardClaimed_{};
};

} // namespace Astral::Scene
