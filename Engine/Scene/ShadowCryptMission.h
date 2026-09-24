#pragma once

#include "Engine/Scene/ShadowCryptExpedition.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace Astral::Scene {

enum class ShadowCryptGuidanceAction : std::uint8_t {
    None,
    BreakSealAnchor,
    RecoverArchiveRecord,
    DiscoverCoolingCache,
    ClearCoolingCache,
    StabilizeRiftNode,
    DefeatRiftWarden,
    ResumeCheckpoint,
    ReplayRun,
};

enum class ShadowCryptMissionFocusMode : std::uint8_t {
    Standard,
    Focused,
    Count,
};

enum class ShadowCryptEntryBlocker : std::uint8_t {
    None,
    ProtagonistUnavailable,
    ShadowCryptLeadIncomplete,
    ActiveRun,
    CheckpointPending,
    CompletedRunRequiresReplay,
};

enum class ShadowCryptEntryGuidance : std::uint8_t {
    None,
    BindProtagonist,
    FindCryptSigil,
    AskAboutShadowCrypt,
    EnterShadowCrypt,
    ContinueRun,
    ResumeCheckpoint,
    ReplayCompletedRun,
};

struct ShadowCryptMissionRecord {
    bool valid{};
    int score{};
    ShadowCryptGrade grade{ShadowCryptGrade::Unranked};
    int damageTaken{};
    int defeats{};
    bool optionalCacheCleared{};
};

struct ShadowCryptEntryReport {
    bool readyToBegin{};
    FieldOperationProgress leadProgress{};
    ShadowCryptEntryBlocker blocker{ShadowCryptEntryBlocker::None};
};

struct ShadowCryptMissionPreview {
    bool available{};
    int mainRoomCount{};
    std::array<int, ShadowCryptExpedition::MainRoomCount> objectiveRequirements{};
    bool optionalCoolingCache{};
    int optionalCacheUnlockAfterRooms{};
    bool riftWardenFinalEncounter{};
    ShadowCryptRoom finalRoom{ShadowCryptRoom::WardenSanctum};
    ShadowCryptObjective finalObjective{ShadowCryptObjective::DefeatRiftWarden};
    ShadowCryptMissionFocusMode focusMode{ShadowCryptMissionFocusMode::Standard};
};

struct ShadowCryptMissionResult {
    bool available{};
    ShadowCryptMissionRecord latest{};
    ShadowCryptMissionRecord personalBest{};
    int scoreBehindBest{};
    bool newPersonalBest{};
};

struct ShadowCryptMissionBriefing {
    bool available{};
    bool active{};
    bool complete{};
    bool suspended{};
    bool safeToSuspend{};
    bool checkpointAvailable{};
    ShadowCryptRoom room{ShadowCryptRoom::EntrySeal};
    ShadowCryptObjective objective{ShadowCryptObjective::BreakSealAnchors};
    int objectiveProgress{};
    int objectiveRequired{};
    int roomsCleared{};
    bool coolingCacheDiscovered{};
    bool coolingCacheCleared{};
    int defeats{};
    int damageTaken{};
    ShadowCryptMissionFocusMode focusMode{ShadowCryptMissionFocusMode::Standard};
    ShadowCryptGuidanceAction guidance{ShadowCryptGuidanceAction::None};
};

// Game-owned coordinator that attaches the already-reviewed Shadow Crypt
// expedition domain to a production game owner. It adds no renderer, platform,
// save backend, networking, engine timing, or second reward currency.
class ShadowCryptMission {
public:
    static constexpr std::size_t CompletionHistoryCapacity = 4;

    bool Begin(const ExplorationFieldGuide& guide) {
        // Begin is the one-time initial-entry gate. Once a run exists, completed
        // state must go through ReplayCompletedRun and suspended state through
        // ResumeSuspendedRun so callers cannot bypass those authority checks.
        if (hasRunHistory_ || checkpointAvailable_ || !expedition_.TryBegin(guide)) return false;
        hasRunHistory_ = true;
        return true;
    }

    bool RecordObjectiveStep() {
        if (!expedition_.RecordObjectiveStep()) return false;
        if (expedition_.Complete()) RecordCompletion();
        return true;
    }

    bool DiscoverCoolingCache() { return expedition_.DiscoverCoolingCache(); }
    bool ClearCoolingCache() { return expedition_.ClearCoolingCache(); }
    int RecordDamageTaken(int amount) { return expedition_.RecordDamageTaken(amount); }
    bool ReportDefeat() { return expedition_.ReportDefeat(); }

    // GAME-134: suspend only at the expedition's validated room boundary. The
    // live run is cleared after capturing the point so a second active timeline
    // cannot coexist with the stored checkpoint.
    bool SuspendAtSafeBoundary() {
        if (checkpointAvailable_) return false;
        const ShadowCryptResumePoint point = expedition_.CreateResumePoint();
        if (!point.valid) return false;
        checkpoint_ = point;
        checkpointAvailable_ = true;
        expedition_ = ShadowCryptExpedition{};
        return true;
    }

    bool ResumeSuspendedRun() {
        if (!checkpointAvailable_ || expedition_.Active()) return false;
        if (!expedition_.ResumeFrom(checkpoint_)) return false;
        checkpoint_ = ShadowCryptResumePoint{};
        checkpointAvailable_ = false;
        hasRunHistory_ = true;
        return true;
    }

    // GAME-136: completed expeditions can restart through the same authoritative
    // lead gate while persistent progression keeps the one-time reward monotonic.
    bool ReplayCompletedRun(const ExplorationFieldGuide& guide) {
        if (!hasRunHistory_ || checkpointAvailable_ || !expedition_.Complete()) return false;
        return expedition_.TryBegin(guide);
    }

    ShadowCryptRewardReport ClaimFirstClearReward(CharacterProgression& progression) {
        return expedition_.ClaimFirstClearReward(progression);
    }

    // GAME-182: focused guidance is a reversible presentation preference. It
    // prioritizes the main room objective but never removes or auto-completes the
    // optional Cooling Cache.
    bool SetFocusMode(ShadowCryptMissionFocusMode mode) {
        if (!ValidFocusMode(mode) || mode == focusMode_) return false;
        focusMode_ = mode;
        return true;
    }

    ShadowCryptMissionFocusMode FocusMode() const { return focusMode_; }

    // GAME-183: expose the exact mission-state and narrative gate that prevents
    // a fresh initial entry. Protagonist ownership is added by the live owner.
    ShadowCryptEntryReport EntryReport(const ExplorationFieldGuide& guide) const {
        ShadowCryptEntryReport report{};
        report.leadProgress = guide.OperationProgress(FieldOperation::ShadowCryptLead);
        if (checkpointAvailable_) {
            report.blocker = ShadowCryptEntryBlocker::CheckpointPending;
            return report;
        }
        if (expedition_.Active()) {
            report.blocker = ShadowCryptEntryBlocker::ActiveRun;
            return report;
        }
        if (hasRunHistory_ && expedition_.Complete()) {
            report.blocker = ShadowCryptEntryBlocker::CompletedRunRequiresReplay;
            return report;
        }
        if (hasRunHistory_) {
            // A historical run that is neither active, suspended, nor complete is
            // not a valid fresh-entry state. Fail closed rather than inventing a
            // second timeline.
            report.blocker = ShadowCryptEntryBlocker::ActiveRun;
            return report;
        }
        if (!report.leadProgress.Complete()) {
            report.blocker = ShadowCryptEntryBlocker::ShadowCryptLeadIncomplete;
            return report;
        }
        report.readyToBegin = true;
        return report;
    }

    // GAME-184: derive a compact next action from authoritative mission and field
    // evidence without mutating which operation the player currently tracks.
    ShadowCryptEntryGuidance EntryGuidance(const ExplorationFieldGuide& guide) const {
        const ShadowCryptEntryReport report = EntryReport(guide);
        switch (report.blocker) {
        case ShadowCryptEntryBlocker::CheckpointPending:
            return ShadowCryptEntryGuidance::ResumeCheckpoint;
        case ShadowCryptEntryBlocker::ActiveRun:
            return ShadowCryptEntryGuidance::ContinueRun;
        case ShadowCryptEntryBlocker::CompletedRunRequiresReplay:
            return ShadowCryptEntryGuidance::ReplayCompletedRun;
        case ShadowCryptEntryBlocker::ShadowCryptLeadIncomplete: {
            ExplorationFieldGuide leadGuide = guide;
            if (!leadGuide.TrackOperation(FieldOperation::ShadowCryptLead)) return ShadowCryptEntryGuidance::None;
            const FieldHint hint = leadGuide.CurrentHint();
            if (hint.kind == FieldHintKind::FindCryptSigil) {
                return ShadowCryptEntryGuidance::FindCryptSigil;
            }
            if (hint.kind == FieldHintKind::AskAboutShadowCrypt) {
                return ShadowCryptEntryGuidance::AskAboutShadowCrypt;
            }
            return ShadowCryptEntryGuidance::None;
        }
        case ShadowCryptEntryBlocker::ProtagonistUnavailable:
            return ShadowCryptEntryGuidance::BindProtagonist;
        case ShadowCryptEntryBlocker::None:
            return report.readyToBegin
                ? ShadowCryptEntryGuidance::EnterShadowCrypt
                : ShadowCryptEntryGuidance::None;
        }
        return ShadowCryptEntryGuidance::None;
    }

    // QOL-038: pre-entry encounter intel is deliberately static and read-only.
    // It exposes only authored mission structure already present in the game.
    ShadowCryptMissionPreview Preview() const {
        ShadowCryptMissionPreview preview{};
        preview.available = true;
        preview.mainRoomCount = ShadowCryptExpedition::MainRoomCount;
        preview.objectiveRequirements = {2, 3, 2, 1};
        preview.optionalCoolingCache = true;
        preview.optionalCacheUnlockAfterRooms = 2;
        preview.riftWardenFinalEncounter = true;
        preview.focusMode = focusMode_;
        return preview;
    }

    ShadowCryptMissionBriefing Briefing() const {
        ShadowCryptMissionBriefing briefing{};
        briefing.focusMode = focusMode_;
        briefing.available = hasRunHistory_ || checkpointAvailable_;
        briefing.checkpointAvailable = checkpointAvailable_;
        if (!briefing.available) return briefing;

        if (checkpointAvailable_) {
            briefing.suspended = true;
            briefing.room = checkpoint_.room;
            briefing.objective = ObjectiveFor(checkpoint_.room);
            briefing.objectiveRequired = ObjectiveRequiredFor(checkpoint_.room);
            briefing.roomsCleared = RoomIndex(checkpoint_.room);
            briefing.coolingCacheDiscovered = checkpoint_.coolingCacheDiscovered;
            briefing.coolingCacheCleared = checkpoint_.coolingCacheCleared;
            briefing.defeats = checkpoint_.defeats;
            briefing.damageTaken = checkpoint_.damageTaken;
            briefing.guidance = ShadowCryptGuidanceAction::ResumeCheckpoint;
            return briefing;
        }

        briefing.active = expedition_.Active();
        briefing.complete = expedition_.Complete();
        briefing.room = expedition_.CurrentRoom();
        briefing.objective = expedition_.CurrentObjective();
        briefing.objectiveProgress = expedition_.ObjectiveProgress();
        briefing.objectiveRequired = expedition_.ObjectiveRequired();
        briefing.roomsCleared = expedition_.RoomsCleared();
        briefing.coolingCacheDiscovered = expedition_.CoolingCacheDiscovered();
        briefing.coolingCacheCleared = expedition_.CoolingCacheCleared();
        briefing.defeats = expedition_.Defeats();
        briefing.damageTaken = expedition_.DamageTaken();
        briefing.safeToSuspend = expedition_.SafeToSuspend();

        if (briefing.complete) {
            briefing.guidance = ShadowCryptGuidanceAction::ReplayRun;
        } else if (briefing.active) {
            briefing.guidance = GuidanceFor(expedition_);
        }
        return briefing;
    }

    // GAME-185: result comparison keeps the latest accepted clear and historical
    // personal best side by side without changing either record.
    ShadowCryptMissionResult LatestResult() const {
        ShadowCryptMissionResult result{};
        if (completionHistoryCount_ == 0 || !bestRecord_.valid) return result;
        result.available = true;
        result.latest = completionHistory_[0];
        result.personalBest = bestRecord_;
        result.scoreBehindBest = std::max(0, bestRecord_.score - result.latest.score);
        result.newPersonalBest = lastClearSetPersonalBest_;
        return result;
    }

    // GAME-186: retain a bounded newest-first archive of accepted completions.
    std::size_t CompletionRecordCount() const { return completionHistoryCount_; }
    ShadowCryptMissionRecord CompletionRecordFromNewest(std::size_t offset) const {
        return offset < completionHistoryCount_ ? completionHistory_[offset]
                                                : ShadowCryptMissionRecord{};
    }

    ShadowCryptMissionRecord BestRecord() const { return bestRecord_; }
    bool HasCheckpoint() const { return checkpointAvailable_; }
    const ShadowCryptExpedition& Expedition() const { return expedition_; }

private:
    static constexpr bool ValidFocusMode(ShadowCryptMissionFocusMode mode) {
        return static_cast<std::uint8_t>(mode)
            < static_cast<std::uint8_t>(ShadowCryptMissionFocusMode::Count);
    }

    static constexpr ShadowCryptObjective ObjectiveFor(ShadowCryptRoom room) {
        switch (room) {
        case ShadowCryptRoom::EntrySeal: return ShadowCryptObjective::BreakSealAnchors;
        case ShadowCryptRoom::ArchiveGallery: return ShadowCryptObjective::RecoverArchiveRecords;
        case ShadowCryptRoom::RiftNave: return ShadowCryptObjective::StabilizeRiftNodes;
        case ShadowCryptRoom::WardenSanctum: return ShadowCryptObjective::DefeatRiftWarden;
        case ShadowCryptRoom::Complete: return ShadowCryptObjective::None;
        }
        return ShadowCryptObjective::None;
    }

    static constexpr int ObjectiveRequiredFor(ShadowCryptRoom room) {
        switch (room) {
        case ShadowCryptRoom::EntrySeal: return 2;
        case ShadowCryptRoom::ArchiveGallery: return 3;
        case ShadowCryptRoom::RiftNave: return 2;
        case ShadowCryptRoom::WardenSanctum: return 1;
        case ShadowCryptRoom::Complete: return 0;
        }
        return 0;
    }

    static constexpr int RoomIndex(ShadowCryptRoom room) {
        switch (room) {
        case ShadowCryptRoom::EntrySeal: return 0;
        case ShadowCryptRoom::ArchiveGallery: return 1;
        case ShadowCryptRoom::RiftNave: return 2;
        case ShadowCryptRoom::WardenSanctum: return 3;
        case ShadowCryptRoom::Complete: return ShadowCryptExpedition::MainRoomCount;
        }
        return 0;
    }

    static constexpr int GradeRank(ShadowCryptGrade grade) {
        switch (grade) {
        case ShadowCryptGrade::Unranked: return 0;
        case ShadowCryptGrade::Bronze: return 1;
        case ShadowCryptGrade::Silver: return 2;
        case ShadowCryptGrade::Gold: return 3;
        }
        return 0;
    }

    static int ScoreFor(const ShadowCryptCompletion& completion) {
        if (completion.grade == ShadowCryptGrade::Unranked) return 0;
        const int gradeScore = GradeRank(completion.grade) * 1000;
        const int cacheBonus = completion.optionalCacheCleared ? 250 : 0;
        const int damagePenalty = std::min(completion.damageTaken, 500);
        const int defeatPenalty = std::min(completion.defeats, 20) * 50;
        return std::max(0, gradeScore + cacheBonus - damagePenalty - defeatPenalty);
    }

    static bool BetterThan(const ShadowCryptMissionRecord& candidate,
        const ShadowCryptMissionRecord& current) {
        if (!current.valid) return true;
        if (candidate.score != current.score) return candidate.score > current.score;
        if (GradeRank(candidate.grade) != GradeRank(current.grade)) {
            return GradeRank(candidate.grade) > GradeRank(current.grade);
        }
        if (candidate.damageTaken != current.damageTaken) {
            return candidate.damageTaken < current.damageTaken;
        }
        if (candidate.defeats != current.defeats) return candidate.defeats < current.defeats;
        return candidate.optionalCacheCleared && !current.optionalCacheCleared;
    }

    ShadowCryptGuidanceAction GuidanceFor(const ShadowCryptExpedition& expedition) const {
        switch (expedition.CurrentRoom()) {
        case ShadowCryptRoom::EntrySeal:
            return ShadowCryptGuidanceAction::BreakSealAnchor;
        case ShadowCryptRoom::ArchiveGallery:
            return ShadowCryptGuidanceAction::RecoverArchiveRecord;
        case ShadowCryptRoom::RiftNave:
            if (focusMode_ == ShadowCryptMissionFocusMode::Focused) {
                return ShadowCryptGuidanceAction::StabilizeRiftNode;
            }
            if (!expedition.CoolingCacheDiscovered()) {
                return ShadowCryptGuidanceAction::DiscoverCoolingCache;
            }
            if (!expedition.CoolingCacheCleared()) {
                return ShadowCryptGuidanceAction::ClearCoolingCache;
            }
            return ShadowCryptGuidanceAction::StabilizeRiftNode;
        case ShadowCryptRoom::WardenSanctum:
            return ShadowCryptGuidanceAction::DefeatRiftWarden;
        case ShadowCryptRoom::Complete:
            return ShadowCryptGuidanceAction::ReplayRun;
        }
        return ShadowCryptGuidanceAction::None;
    }

    void RecordCompletionHistory(const ShadowCryptMissionRecord& record) {
        const std::size_t last = std::min(completionHistoryCount_, CompletionHistoryCapacity - 1);
        for (std::size_t index = last; index > 0; --index) {
            completionHistory_[index] = completionHistory_[index - 1];
        }
        completionHistory_[0] = record;
        if (completionHistoryCount_ < CompletionHistoryCapacity) ++completionHistoryCount_;
    }

    void RecordCompletion() {
        const ShadowCryptCompletion completion = expedition_.CompletionSummary();
        if (completion.grade == ShadowCryptGrade::Unranked) return;
        ShadowCryptMissionRecord candidate{};
        candidate.valid = true;
        candidate.score = ScoreFor(completion);
        candidate.grade = completion.grade;
        candidate.damageTaken = completion.damageTaken;
        candidate.defeats = completion.defeats;
        candidate.optionalCacheCleared = completion.optionalCacheCleared;
        lastClearSetPersonalBest_ = BetterThan(candidate, bestRecord_);
        RecordCompletionHistory(candidate);
        if (lastClearSetPersonalBest_) bestRecord_ = candidate;
    }

    ShadowCryptExpedition expedition_{};
    ShadowCryptResumePoint checkpoint_{};
    ShadowCryptMissionRecord bestRecord_{};
    std::array<ShadowCryptMissionRecord, CompletionHistoryCapacity> completionHistory_{};
    std::size_t completionHistoryCount_{};
    ShadowCryptMissionFocusMode focusMode_{ShadowCryptMissionFocusMode::Standard};
    bool checkpointAvailable_{};
    bool hasRunHistory_{};
    bool lastClearSetPersonalBest_{};
};

} // namespace Astral::Scene
