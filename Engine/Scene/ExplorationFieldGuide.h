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
    RoutePin,
};

enum class FieldHintKind {
    None,
    VisitLandmark,
    SeekRiftResidue,
    InvestigateCoolingAnomaly,
    FindCryptSigil,
    AskAboutShadowCrypt,
    OperationComplete,
};

enum class FieldPinCategory {
    Objective,
    Resource,
    Note,
    Count,
};

enum class FieldPinFilter {
    All,
    Objective,
    Resource,
    Note,
    Count,
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

struct FieldHint {
    bool available{};
    FieldHintKind kind{FieldHintKind::None};
    bool hasLandmark{};
    LandmarkKind landmark{LandmarkKind::LincolnMemorial};
    FieldOperationProgress progress{};
};

struct FieldRouteStop {
    LandmarkKind landmark{LandmarkKind::LincolnMemorial};
    FieldPinCategory category{FieldPinCategory::Objective};
};

struct FieldGuideBriefing {
    FieldOperation trackedOperation{FieldOperation::MallSurvey};
    FieldOperationProgress operationProgress{};
    FieldHint hint{};
    FieldTarget target{};
    std::size_t routeStops{};
    std::size_t journalUnlocked{};
    std::size_t journalUnread{};
    std::size_t completedOperations{};
};

class ExplorationFieldGuide {
public:
    static constexpr std::size_t SiteCount = 3;
    static constexpr std::size_t OperationCount = static_cast<std::size_t>(FieldOperation::Count);
    static constexpr std::size_t JournalEntryCount =
        static_cast<std::size_t>(FieldJournalEntry::Count);
    static constexpr std::size_t RouteCapacity = SiteCount;
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

    constexpr bool TrackNextIncompleteOperation() {
        if (CompletedOperationCount() >= OperationCount) return false;
        const std::size_t current = static_cast<std::size_t>(trackedOperation_);
        for (std::size_t offset = 1; offset <= OperationCount; ++offset) {
            const std::size_t index = (current + offset) % OperationCount;
            const FieldOperation candidate = static_cast<FieldOperation>(index);
            if (!OperationProgress(candidate).Complete()) {
                trackedOperation_ = candidate;
                return true;
            }
        }
        return false;
    }

    constexpr bool RecordLandmark(LandmarkKind landmark) {
        const std::size_t index = LandmarkIndex(landmark);
        if (index >= SiteCount || visited_[index]) return false;

        visited_[index] = true;
        discoveryOrder_[discoveryCount_++] = landmark;
        UnlockJournal(SiteJournalEntry(landmark));
        if (pinActive_ && pinnedLandmark_ == landmark) pinActive_ = false;
        RemoveRouteTargetInternal(landmark);
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
        observedRiftClueCount_ = static_cast<std::size_t>(hasRiftResidue_)
            + static_cast<std::size_t>(hasCoolingAnomaly_);

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
            return {static_cast<int>(std::min<std::size_t>(observedRiftClueCount_, 2)), 2};
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

    constexpr FieldHint CurrentHint() const {
        FieldHint hint{};
        hint.progress = OperationProgress(trackedOperation_);
        if (hint.progress.Complete()) {
            hint.available = true;
            hint.kind = FieldHintKind::OperationComplete;
            return hint;
        }
        hint.available = true;
        switch (trackedOperation_) {
        case FieldOperation::MallSurvey:
            for (LandmarkKind landmark : SurveyRoute) {
                if (!IsVisited(landmark)) {
                    hint.kind = FieldHintKind::VisitLandmark;
                    hint.hasLandmark = true;
                    hint.landmark = landmark;
                    return hint;
                }
            }
            break;
        case FieldOperation::RiftInvestigation:
            hint.kind = !hasRiftResidue_
                ? FieldHintKind::SeekRiftResidue
                : FieldHintKind::InvestigateCoolingAnomaly;
            return hint;
        case FieldOperation::ShadowCryptLead:
            hint.kind = !hasCryptSigil_
                ? FieldHintKind::FindCryptSigil
                : FieldHintKind::AskAboutShadowCrypt;
            return hint;
        case FieldOperation::Count:
            break;
        }
        return {};
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

    constexpr std::size_t UnreadJournalCount() const {
        std::size_t count = 0;
        for (std::size_t index = 0; index < JournalEntryCount; ++index) {
            count += static_cast<std::size_t>(journal_[index] && !journalRead_[index]);
        }
        return count;
    }

    constexpr bool MarkJournalRead(FieldJournalEntry entry) {
        const std::size_t index = static_cast<std::size_t>(entry);
        if (index >= JournalEntryCount || !journal_[index] || journalRead_[index]) return false;
        journalRead_[index] = true;
        return true;
    }

    constexpr std::size_t MarkAllJournalRead() {
        std::size_t marked = 0;
        for (std::size_t index = 0; index < JournalEntryCount; ++index) {
            if (journal_[index] && !journalRead_[index]) {
                journalRead_[index] = true;
                ++marked;
            }
        }
        return marked;
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

    constexpr bool AddRouteTarget(LandmarkKind landmark,
        FieldPinCategory category = FieldPinCategory::Objective) {
        if (LandmarkIndex(landmark) >= SiteCount || !ValidPinCategory(category)
            || IsVisited(landmark) || routeCount_ >= RouteCapacity) {
            return false;
        }
        for (std::size_t index = 0; index < routeCount_; ++index) {
            if (route_[index].landmark == landmark) return false;
        }
        route_[routeCount_++] = {landmark, category};
        return true;
    }

    constexpr bool RemoveRouteTarget(LandmarkKind landmark) {
        return RemoveRouteTargetInternal(landmark);
    }

    constexpr std::size_t ClearRouteTargets() {
        const std::size_t cleared = routeCount_;
        routeCount_ = 0;
        return cleared;
    }

    constexpr std::size_t RouteTargetCount() const { return routeCount_; }

    constexpr FieldRouteStop RouteTarget(std::size_t offset) const {
        return offset < routeCount_ ? route_[offset] : FieldRouteStop{};
    }

    constexpr bool SetRouteFilter(FieldPinFilter filter) {
        if (!ValidPinFilter(filter)) return false;
        routeFilter_ = filter;
        return true;
    }

    constexpr FieldPinFilter RouteFilter() const { return routeFilter_; }

    constexpr FieldTarget CurrentTarget() const {
        if (pinActive_ && !IsVisited(pinnedLandmark_)) {
            return {true, pinnedLandmark_, FieldTargetSource::PlayerPin};
        }
        for (std::size_t index = 0; index < routeCount_; ++index) {
            if (PinVisible(route_[index].category) && !IsVisited(route_[index].landmark)) {
                return {true, route_[index].landmark, FieldTargetSource::RoutePin};
            }
        }
        const FieldHint hint = CurrentHint();
        if (hint.available && hint.hasLandmark && !IsVisited(hint.landmark)) {
            return {true, hint.landmark, FieldTargetSource::Recommended};
        }
        return {};
    }

    constexpr FieldGuideBriefing Briefing() const {
        FieldGuideBriefing briefing{};
        briefing.trackedOperation = trackedOperation_;
        briefing.operationProgress = OperationProgress(trackedOperation_);
        briefing.hint = CurrentHint();
        briefing.target = CurrentTarget();
        briefing.routeStops = routeCount_;
        briefing.journalUnlocked = JournalEntryUnlockedCount();
        briefing.journalUnread = UnreadJournalCount();
        briefing.completedOperations = CompletedOperationCount();
        return briefing;
    }

private:
    static constexpr bool ValidOperation(FieldOperation operation) {
        return static_cast<std::size_t>(operation) < OperationCount;
    }

    static constexpr bool ValidPinCategory(FieldPinCategory category) {
        return static_cast<std::size_t>(category) < static_cast<std::size_t>(FieldPinCategory::Count);
    }

    static constexpr bool ValidPinFilter(FieldPinFilter filter) {
        return static_cast<std::size_t>(filter) < static_cast<std::size_t>(FieldPinFilter::Count);
    }

    constexpr bool PinVisible(FieldPinCategory category) const {
        if (routeFilter_ == FieldPinFilter::All) return true;
        switch (routeFilter_) {
        case FieldPinFilter::Objective: return category == FieldPinCategory::Objective;
        case FieldPinFilter::Resource: return category == FieldPinCategory::Resource;
        case FieldPinFilter::Note: return category == FieldPinCategory::Note;
        case FieldPinFilter::All:
        case FieldPinFilter::Count:
            break;
        }
        return false;
    }

    constexpr bool RemoveRouteTargetInternal(LandmarkKind landmark) {
        for (std::size_t index = 0; index < routeCount_; ++index) {
            if (route_[index].landmark != landmark) continue;
            for (std::size_t shift = index + 1; shift < routeCount_; ++shift) {
                route_[shift - 1] = route_[shift];
            }
            --routeCount_;
            return true;
        }
        return false;
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
        if (index < JournalEntryCount && !journal_[index]) {
            journal_[index] = true;
            journalRead_[index] = false;
        }
    }

    std::array<bool, SiteCount> visited_{};
    std::array<LandmarkKind, SiteCount> discoveryOrder_{};
    std::size_t discoveryCount_{};
    std::array<bool, JournalEntryCount> journal_{};
    std::array<bool, JournalEntryCount> journalRead_{};
    std::size_t observedRiftClueCount_{};
    bool hasRiftResidue_{};
    bool hasCoolingAnomaly_{};
    bool hasCryptSigil_{};
    bool hasShadowCryptLore_{};
    FieldOperation trackedOperation_{FieldOperation::MallSurvey};
    bool pinActive_{};
    LandmarkKind pinnedLandmark_{LandmarkKind::LincolnMemorial};
    std::array<FieldRouteStop, RouteCapacity> route_{};
    std::size_t routeCount_{};
    FieldPinFilter routeFilter_{FieldPinFilter::All};
};

namespace Detail {
constexpr bool ExplorationFieldGuideContract() {
    ExplorationFieldGuide guide;
    if (!guide.CurrentTarget().available
        || guide.CurrentTarget().landmark != LandmarkKind::LincolnMemorial
        || guide.CurrentTarget().source != FieldTargetSource::Recommended) return false;
    if (guide.CurrentHint().kind != FieldHintKind::VisitLandmark
        || guide.CurrentHint().landmark != LandmarkKind::LincolnMemorial) return false;
    if (!guide.PinTarget(LandmarkKind::WashingtonMonument)
        || guide.CurrentTarget().landmark != LandmarkKind::WashingtonMonument
        || guide.CurrentTarget().source != FieldTargetSource::PlayerPin) return false;
    if (!guide.RecordLandmark(LandmarkKind::WashingtonMonument)
        || guide.HasPinnedTarget()
        || guide.RecordLandmark(LandmarkKind::WashingtonMonument)) return false;
    if (guide.DiscoveryCount() != 1
        || guide.DiscoveryFromOldest(0) != LandmarkKind::WashingtonMonument
        || !guide.HasJournalEntry(FieldJournalEntry::MonumentFieldNote)
        || guide.UnreadJournalCount() != 1) return false;

    FieldNarrativeSnapshot evidence{};
    evidence.hasRiftResidue = true;
    evidence.hasCryptSigil = true;
    guide.SyncNarrativeEvidence(evidence);
    if (guide.OperationProgress(FieldOperation::RiftInvestigation).current != 1
        || guide.OperationProgress(FieldOperation::RiftInvestigation).Complete()) return false;

    evidence.hasCoolingAnomaly = true;
    guide.SyncNarrativeEvidence(evidence);
    if (!guide.OperationProgress(FieldOperation::RiftInvestigation).Complete()
        || !guide.HasJournalEntry(FieldJournalEntry::RiftEvidence)) return false;

    evidence.hasRiftResidue = false;
    evidence.hasCoolingAnomaly = false;
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
