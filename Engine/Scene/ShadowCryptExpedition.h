#pragma once

#include "Engine/Scene/CharacterProgression.h"
#include "Engine/Scene/ExplorationFieldGuide.h"

#include <algorithm>
#include <cstdint>

namespace Astral::Scene {

enum class ShadowCryptRoom : std::uint8_t {
    EntrySeal,
    ArchiveGallery,
    RiftNave,
    WardenSanctum,
    Complete,
};

enum class ShadowCryptObjective : std::uint8_t {
    BreakSealAnchors,
    RecoverArchiveRecords,
    StabilizeRiftNodes,
    DefeatRiftWarden,
    None,
};

enum class ShadowCryptGrade : std::uint8_t {
    Unranked,
    Bronze,
    Silver,
    Gold,
};

struct ShadowCryptResumePoint {
    static constexpr std::uint32_t SchemaVersion = 1;

    std::uint32_t schemaVersion{SchemaVersion};
    bool valid{};
    bool active{};
    bool complete{};
    ShadowCryptRoom room{ShadowCryptRoom::EntrySeal};
    bool coolingCacheDiscovered{};
    bool coolingCacheCleared{};
    int defeats{};
    int damageTaken{};
    bool firstClearRewardClaimed{};
};

struct ShadowCryptCompletion {
    ShadowCryptGrade grade{ShadowCryptGrade::Unranked};
    int roomsCleared{};
    bool optionalCacheCleared{};
    int defeats{};
    int damageTaken{};
};

struct ShadowCryptRewardReport {
    bool granted{};
    ProgressionRewardReport progression{};
};

class ShadowCryptExpedition {
public:
    static constexpr int MainRoomCount = 4;
    static constexpr int GoldDamageLimit = 50;
    static constexpr int SilverDamageLimit = 150;
    static constexpr int FirstClearExperience = 300;
    static constexpr int FirstClearMastery = 30;
    static constexpr int FirstClearEnhancementMaterials = 40;

    bool TryBegin(const ExplorationFieldGuide& guide) {
        if (!guide.OperationProgress(FieldOperation::ShadowCryptLead).Complete() || active_) {
            return false;
        }
        active_ = true;
        complete_ = false;
        room_ = ShadowCryptRoom::EntrySeal;
        objectiveProgress_ = 0;
        roomsCleared_ = 0;
        coolingCacheDiscovered_ = false;
        coolingCacheCleared_ = false;
        defeats_ = 0;
        damageTaken_ = 0;
        return true;
    }

    bool Active() const { return active_; }
    bool Complete() const { return complete_; }
    ShadowCryptRoom CurrentRoom() const { return room_; }

    ShadowCryptObjective CurrentObjective() const {
        switch (room_) {
        case ShadowCryptRoom::EntrySeal: return ShadowCryptObjective::BreakSealAnchors;
        case ShadowCryptRoom::ArchiveGallery: return ShadowCryptObjective::RecoverArchiveRecords;
        case ShadowCryptRoom::RiftNave: return ShadowCryptObjective::StabilizeRiftNodes;
        case ShadowCryptRoom::WardenSanctum: return ShadowCryptObjective::DefeatRiftWarden;
        case ShadowCryptRoom::Complete: return ShadowCryptObjective::None;
        }
        return ShadowCryptObjective::None;
    }

    int ObjectiveProgress() const { return objectiveProgress_; }
    int ObjectiveRequired() const { return ObjectiveRequiredFor(room_); }
    int RoomsCleared() const { return roomsCleared_; }

    bool RecordObjectiveStep() {
        if (!active_ || complete_) return false;
        const int required = ObjectiveRequired();
        if (required <= 0 || objectiveProgress_ >= required) return false;
        ++objectiveProgress_;
        if (objectiveProgress_ == required) AdvanceRoom();
        return true;
    }

    bool DiscoverCoolingCache() {
        if (!active_ || complete_ || coolingCacheDiscovered_ || roomsCleared_ < 2) {
            return false;
        }
        coolingCacheDiscovered_ = true;
        return true;
    }

    bool ClearCoolingCache() {
        if (!active_ || complete_ || !coolingCacheDiscovered_ || coolingCacheCleared_) {
            return false;
        }
        coolingCacheCleared_ = true;
        return true;
    }

    bool CoolingCacheDiscovered() const { return coolingCacheDiscovered_; }
    bool CoolingCacheCleared() const { return coolingCacheCleared_; }

    int RecordDamageTaken(int amount) {
        if (!active_ || complete_ || amount <= 0) return 0;
        const int room = MaxTrackedDamage - damageTaken_;
        const int applied = std::min(amount, std::max(0, room));
        damageTaken_ += applied;
        return applied;
    }

    int DamageTaken() const { return damageTaken_; }
    int Defeats() const { return defeats_; }

    bool ReportDefeat() {
        if (!active_ || complete_) return false;
        if (defeats_ < MaxTrackedDefeats) ++defeats_;
        objectiveProgress_ = 0;
        return true;
    }

    ShadowCryptCompletion CompletionSummary() const {
        ShadowCryptCompletion summary{};
        summary.roomsCleared = roomsCleared_;
        summary.optionalCacheCleared = coolingCacheCleared_;
        summary.defeats = defeats_;
        summary.damageTaken = damageTaken_;
        if (!complete_) return summary;

        if (coolingCacheCleared_ && defeats_ == 0 && damageTaken_ <= GoldDamageLimit) {
            summary.grade = ShadowCryptGrade::Gold;
        } else if (defeats_ <= 1 && damageTaken_ <= SilverDamageLimit) {
            summary.grade = ShadowCryptGrade::Silver;
        } else {
            summary.grade = ShadowCryptGrade::Bronze;
        }
        return summary;
    }

    ShadowCryptRewardReport ClaimFirstClearReward(CharacterProgression& progression) {
        ShadowCryptRewardReport report{};
        if (!complete_ || firstClearRewardClaimed_) return report;
        firstClearRewardClaimed_ = true;
        report.granted = true;
        report.progression = progression.GrantRewards(
            FirstClearExperience, FirstClearMastery, FirstClearEnhancementMaterials);
        return report;
    }

    bool FirstClearRewardClaimed() const { return firstClearRewardClaimed_; }

    bool SafeToSuspend() const {
        return active_ && !complete_ && objectiveProgress_ == 0;
    }

    ShadowCryptResumePoint CreateResumePoint() const {
        ShadowCryptResumePoint point{};
        if (!SafeToSuspend()) return point;
        point.valid = true;
        point.active = active_;
        point.complete = complete_;
        point.room = room_;
        point.coolingCacheDiscovered = coolingCacheDiscovered_;
        point.coolingCacheCleared = coolingCacheCleared_;
        point.defeats = defeats_;
        point.damageTaken = damageTaken_;
        point.firstClearRewardClaimed = firstClearRewardClaimed_;
        return point;
    }

    bool ResumeFrom(const ShadowCryptResumePoint& point) {
        if (active_ || !ValidResumePoint(point)) return false;
        active_ = true;
        complete_ = false;
        room_ = point.room;
        objectiveProgress_ = 0;
        roomsCleared_ = RoomIndex(point.room);
        coolingCacheDiscovered_ = point.coolingCacheDiscovered;
        coolingCacheCleared_ = point.coolingCacheCleared;
        defeats_ = point.defeats;
        damageTaken_ = point.damageTaken;
        firstClearRewardClaimed_ = point.firstClearRewardClaimed;
        return true;
    }

private:
    static constexpr int MaxTrackedDamage = 1000000;
    static constexpr int MaxTrackedDefeats = 1000000;

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
        case ShadowCryptRoom::Complete: return MainRoomCount;
        }
        return MainRoomCount + 1;
    }

    static constexpr ShadowCryptRoom NextRoom(ShadowCryptRoom room) {
        switch (room) {
        case ShadowCryptRoom::EntrySeal: return ShadowCryptRoom::ArchiveGallery;
        case ShadowCryptRoom::ArchiveGallery: return ShadowCryptRoom::RiftNave;
        case ShadowCryptRoom::RiftNave: return ShadowCryptRoom::WardenSanctum;
        case ShadowCryptRoom::WardenSanctum: return ShadowCryptRoom::Complete;
        case ShadowCryptRoom::Complete: return ShadowCryptRoom::Complete;
        }
        return ShadowCryptRoom::Complete;
    }

    void AdvanceRoom() {
        ++roomsCleared_;
        room_ = NextRoom(room_);
        objectiveProgress_ = 0;
        if (room_ == ShadowCryptRoom::Complete) {
            complete_ = true;
            active_ = false;
        }
    }

    static bool ValidResumePoint(const ShadowCryptResumePoint& point) {
        if (point.schemaVersion != ShadowCryptResumePoint::SchemaVersion || !point.valid) {
            return false;
        }
        if (!point.active || point.complete) return false;
        const int roomIndex = RoomIndex(point.room);
        if (roomIndex < 0 || roomIndex >= MainRoomCount) return false;
        if (point.defeats < 0 || point.defeats > MaxTrackedDefeats
            || point.damageTaken < 0 || point.damageTaken > MaxTrackedDamage) {
            return false;
        }
        if (point.coolingCacheCleared && !point.coolingCacheDiscovered) return false;
        if (roomIndex < 2 && point.coolingCacheDiscovered) return false;
        return true;
    }

    bool active_{};
    bool complete_{};
    ShadowCryptRoom room_{ShadowCryptRoom::EntrySeal};
    int objectiveProgress_{};
    int roomsCleared_{};
    bool coolingCacheDiscovered_{};
    bool coolingCacheCleared_{};
    int defeats_{};
    int damageTaken_{};
    bool firstClearRewardClaimed_{};
};

} // namespace Astral::Scene
