#pragma once

#include "Engine/Scene/WorldBlockout.h"

#include <algorithm>
#include <array>
#include <cstddef>

namespace Astral::Scene {

enum class FieldOperation {
    MallSurvey,
    RiftInvestigation,
    ShadowCryptLead,
    Count,
};

enum class FieldJournalEntry {
    LincolnFieldNote,
    ReflectingPoolFieldNote,
    MonumentFieldNote,
    RiftEvidence,
    ShadowCryptLead,
    Count,
};

enum class FieldTargetSource {
    None,
    Recommended,
    PlayerPin,
};

struct FieldNarrativeSnapshot {
    bool hasRiftResidue{};
    bool hasCoolingAnomaly{};
    bool hasCryptSigil{};
    bool hasShadowCryptLore{};
};

struct FieldOperationProgress {
    int current{};
    int required{};

    constexpr bool Complete() const { return required > 0 && current >= required; }
};

struct FieldTarget {
    bool available{};
    LandmarkKind landmark{LandmarkKind::LincolnMemorial};
    FieldTargetSource source{FieldTargetSource::None};
};

class ExplorationFieldGuide {
public:
    static constexpr std::size_t SiteCount = 3;
    static constexpr std::size_t OperationCount = static_cast<std::size_t>(FieldOperation::Count);
    static constexpr std::size_t JournalEntryCount =
        static_cast<std::size_t>(FieldJournalEntry::Count);
    static constexpr std::array<LandmarkKind, SiteCount> SurveyRoute{
        LandmarkKind::LincolnMemorial,
        LandmarkKind::ReflectingPool,
        LandmarkKind::WashingtonMonument,
    };

    constexpr bool TrackOperation(FieldOperation operation) {
        if (!ValidOperation(operation)) return false;
        trackedOperation_ = operation;
        return true;
    }

    constexpr FieldOperation TrackedOperation() const { return trackedOperation_; }

    constexpr bool RecordLandmark(LandmarkKind landmark) {
        const std::size_t index = LandmarkIndex(landmark);
        if (index >= SiteCount || visited_[index]) return false;

        visited_[index] = true;
        discoveryOrder_[discoveryCount_++] = landmark;
        UnlockJournal(SiteJournalEntry(landmark));
        if (pinActive_ && pinnedLandmark_ == landmark) pinActive_ = false;
        return true;
    }

    constexpr bool IsVisited(LandmarkKind landmark) const {
        const std::size_t index = LandmarkIndex(landmark);
        return index < SiteCount && visited_[index];
    }

    constexpr std::size_t DiscoveryCount() const { return discoveryCount_; }

    constexpr LandmarkKind DiscoveryFromOldest(std::size_t offset) const {
        return offset < discoveryCount_ ? discoveryOrder_[offset] : LandmarkKind::LincolnMemorial;
    }

    constexpr void SyncNarrativeEvidence(const FieldNarrativeSnapshot& snapshot) {
        hasRiftResidue_ = hasRiftResidue_ || snapshot.hasRiftResidue;
        hasCoolingAnomaly_ = hasCoolingAnomaly_ || snapshot.hasCoolingAnomaly;
        hasCryptSigil_ = hasCryptSigil_ || snapshot.hasCryptSigil;
        hasShadowCryptLore_ = hasShadowCryptLore_ || snapshot.hasShadowCryptLore;
        observedClueCount_ = static_cast<std::size_t>(hasRiftResidue_)
            + static_cast<std::size_t>(hasCoolingAnomaly_)
            + static_cast<std::size_t>(hasCryptSigil_);

        if (hasRiftResidue_ || hasCoolingAnomaly_) {
            UnlockJournal(FieldJournalEntry::RiftEvidence);
        }
        if (hasCryptSigil_ && hasShadowCryptLore_) {
            UnlockJournal(FieldJournalEntry::ShadowCryptLead);
        }
    }

    constexpr FieldOperationProgress OperationProgress(FieldOperation operation) const {
        if (!ValidOperation(operation)) return {};
        switch (operation) {
        case FieldOperation::MallSurvey:
            return {static_cast<int>(discoveryCount_), static_cast<int>(SiteCount)};
        case FieldOperation::RiftInvestigation:
            return {static_cast<int>(std::min<std::size_t>(observedClueCount_, 2)), 2};
        case FieldOperation::ShadowCryptLead:
            return {static_cast<int>(hasCryptSigil_) + static_cast<int>(hasShadowCryptLore_), 2};
        case FieldOperation::Count:
            break;
        }
        return {};
    }

    constexpr std::size_t CompletedOperationCount() const {
        std::size_t count = 0;
        for (std::size_t index = 0; index < OperationCount; ++index) {
            if (OperationProgress(static_cast<FieldOperation>(index)).Complete()) ++count;
        }
        return count;
    }

    constexpr bool HasJournalEntry(FieldJournalEntry entry) const {
        const std::size_t index = static_cast<std::size_t>(entry);
        return index < JournalEntryCount && journal_[index];
    }

    constexpr std::size_t JournalEntryUnlockedCount() const {
        std::size_t count = 0;
        for (bool unlocked : journal_) count += static_cast<std::size_t>(unlocked);
        return count;
    }

    constexpr bool PinTarget(LandmarkKind landmark) {
        const std::size_t index = LandmarkIndex(landmark);
        if (index >= SiteCount || visited_[index]) return false;
        pinnedLandmark_ = landmark;
        pinActive_ = true;
        return true;
    }

    constexpr void ClearPinnedTarget() { pinActive_ = false; }
    constexpr bool HasPinnedTarget() const { return pinActive_; }
    constexpr LandmarkKind PinnedTarget() const { return pinnedLandmark_; }

    constexpr FieldTarget CurrentTarget() const {
        if (pinActive_ && !IsVisited(pinnedLandmark_)) {
            return {true, pinnedLandmark_, FieldTargetSource::PlayerPin};
        }
        if (trackedOperation_ == FieldOperation::MallSurvey) {
            for (LandmarkKind landmark : SurveyRoute) {
                if (!IsVisited(landmark)) {
                    return {true, landmark, FieldTargetSource::Recommended};
                }
            }
        }
        return {};
    }

private:
    static constexpr bool ValidOperation(FieldOperation operation) {
        return static_cast<std::size_t>(operation) < OperationCount;
    }

    static constexpr std::size_t LandmarkIndex(LandmarkKind landmark) {
        switch (landmark) {
        case LandmarkKind::LincolnMemorial: return 0;
        case LandmarkKind::ReflectingPool: return 1;
        case LandmarkKind::WashingtonMonument: return 2;
        }
        return SiteCount;
    }

    static constexpr FieldJournalEntry SiteJournalEntry(LandmarkKind landmark) {
        switch (landmark) {
        case LandmarkKind::LincolnMemorial: return FieldJournalEntry::LincolnFieldNote;
        case LandmarkKind::ReflectingPool: return FieldJournalEntry::ReflectingPoolFieldNote;
        case LandmarkKind::WashingtonMonument: return FieldJournalEntry::MonumentFieldNote;
        }
        return FieldJournalEntry::Count;
    }

    constexpr void UnlockJournal(FieldJournalEntry entry) {
        const std::size_t index = static_cast<std::size_t>(entry);
        if (index < JournalEntryCount) journal_[index] = true;
    }

    std::array<bool, SiteCount> visited_{};
    std::array<LandmarkKind, SiteCount> discoveryOrder_{};
    std::size_t discoveryCount_{};
    std::array<bool, JournalEntryCount> journal_{};
    std::size_t observedClueCount_{};
    bool hasRiftResidue_{};
    bool hasCoolingAnomaly_{};
    bool hasCryptSigil_{};
    bool hasShadowCryptLore_{};
    FieldOperation trackedOperation_{FieldOperation::MallSurvey};
    bool pinActive_{};
    LandmarkKind pinnedLandmark_{LandmarkKind::LincolnMemorial};
};

namespace Detail {
constexpr bool ExplorationFieldGuideContract() {
    ExplorationFieldGuide guide;
    if (!guide.CurrentTarget().available
        || guide.CurrentTarget().landmark != LandmarkKind::LincolnMemorial
        || guide.CurrentTarget().source != FieldTargetSource::Recommended) return false;
    if (!guide.PinTarget(LandmarkKind::WashingtonMonument)
        || guide.CurrentTarget().landmark != LandmarkKind::WashingtonMonument
        || guide.CurrentTarget().source != FieldTargetSource::PlayerPin) return false;
    if (!guide.RecordLandmark(LandmarkKind::WashingtonMonument)
        || guide.HasPinnedTarget()
        || guide.RecordLandmark(LandmarkKind::WashingtonMonument)) return false;
    if (guide.DiscoveryCount() != 1
        || guide.DiscoveryFromOldest(0) != LandmarkKind::WashingtonMonument
        || !guide.HasJournalEntry(FieldJournalEntry::MonumentFieldNote)) return false;

    FieldNarrativeSnapshot evidence{};
    evidence.hasRiftResidue = true;
    evidence.hasCoolingAnomaly = true;
    guide.SyncNarrativeEvidence(evidence);
    if (!guide.OperationProgress(FieldOperation::RiftInvestigation).Complete()
        || !guide.HasJournalEntry(FieldJournalEntry::RiftEvidence)) return false;

    evidence.hasRiftResidue = false;
    evidence.hasCoolingAnomaly = false;
    evidence.hasCryptSigil = true;
    evidence.hasShadowCryptLore = true;
    guide.SyncNarrativeEvidence(evidence);
    if (!guide.OperationProgress(FieldOperation::ShadowCryptLead).Complete()
        || !guide.HasJournalEntry(FieldJournalEntry::ShadowCryptLead)) return false;
    return guide.CompletedOperationCount() == 2;
}
}

static_assert(Detail::ExplorationFieldGuideContract(),
    "exploration field guide contract must remain deterministic and bounded");

} // namespace Astral::Scene
