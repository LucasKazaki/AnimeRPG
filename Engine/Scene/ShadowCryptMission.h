#pragma once

#include "Engine/Scene/ShadowCryptExpedition.h"

#include <algorithm>
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

struct ShadowCryptMissionRecord {
    bool valid{};
    int score{};
    ShadowCryptGrade grade{ShadowCryptGrade::Unranked};
    int damageTaken{};
    int defeats{};
    bool optionalCacheCleared{};
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
    ShadowCryptGuidanceAction guidance{ShadowCryptGuidanceAction::None};
};

// Game-owned coordinator that attaches the already-reviewed Shadow Crypt
// expedition domain to a production game owner. It adds no renderer, platform,
// save backend, networking, engine timing, or second reward currency.
class ShadowCryptMission {
public:
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

    ShadowCryptMissionBriefing Briefing() const {
        ShadowCryptMissionBriefing briefing{};
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

    ShadowCryptMissionRecord BestRecord() const { return bestRecord_; }
    bool HasCheckpoint() const { return checkpointAvailable_; }
    const ShadowCryptExpedition& Expedition() const { return expedition_; }

private:
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

    static ShadowCryptGuidanceAction GuidanceFor(const ShadowCryptExpedition& expedition) {
        switch (expedition.CurrentRoom()) {
        case ShadowCryptRoom::EntrySeal:
            return ShadowCryptGuidanceAction::BreakSealAnchor;
        case ShadowCryptRoom::ArchiveGallery:
            return ShadowCryptGuidanceAction::RecoverArchiveRecord;
        case ShadowCryptRoom::RiftNave:
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
        if (BetterThan(candidate, bestRecord_)) bestRecord_ = candidate;
    }

    ShadowCryptExpedition expedition_{};
    ShadowCryptResumePoint checkpoint_{};
    ShadowCryptMissionRecord bestRecord_{};
    bool checkpointAvailable_{};
    bool hasRunHistory_{};
};

} // namespace Astral::Scene
