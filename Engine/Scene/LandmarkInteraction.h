#pragma once

#include "Engine/Scene/CharacterProgression.h"
#include "Engine/Scene/ExplorationFieldGuide.h"
#include "Engine/Scene/LandmarkDialogue.h"
#include "Engine/Scene/ShadowbladeActions.h"
#include "Engine/Scene/WorldBlockout.h"

#include <array>
#include <cstddef>

namespace Astral::Scene {

enum class LandmarkInteractionResult {
    None,
    OutOfRange,
    Discovered,
    AlreadyVisited,
    ObjectiveAdvanced,
};

enum class LandmarkObjectiveStage {
    AwaitingStart,
    DiscoverLincoln,
    DiscoverReflectingPool,
    DiscoverWashingtonMonument,
    Complete,
};

enum class LandmarkObjectiveActivationMode {
    AutoStart,
    ManualStart,
};

struct LandmarkInteractionReport {
    LandmarkInteractionResult result{LandmarkInteractionResult::None};
    LandmarkKind landmark{LandmarkKind::LincolnMemorial};
    float rewardApplied{};
    ProgressionRewardReport progressionReward{};
    const ShadowbladeActions* actionOwner{};
};

class LandmarkInteraction {
public:
    static constexpr float ProximityRadius = 3.0f;
    static constexpr float LincolnReward = 20.0f;
    static constexpr float ObjectiveCompletionReward = 15.0f;
    static constexpr float OrderedResonanceReward = 10.0f;
    static constexpr int ObjectiveExperienceReward = 180;
    static constexpr int ObjectiveMasteryReward = 40;
    static constexpr int ObjectiveEnhancementMaterialReward = 15;
    static constexpr std::size_t LedgerCapacity = 3;

    bool UpdateSelection(const Math::Vec3& playerPosition, const WorldBlockout& world);
    LandmarkInteractionReport TryInteract(const Math::Vec3& playerPosition,
        const WorldBlockout& world, ShadowbladeActions& shadowbladeActions);
    void SetCharacterProgression(CharacterProgression* progression) { progression_ = progression; }

    DialogueBeat TryDialogueChoice(DialogueTopic topic, DialogueChoice choice,
        bool allowAdvanceScreening = false) {
        const DialogueBeat beat = dialogue_.Choose(
            topic, choice, DialogueContext(allowAdvanceScreening));
        SyncFieldGuideNarrative();
        return beat;
    }
    DialogueOutcome CommitDialogueOutcome() {
        const DialogueOutcome outcome = dialogue_.CommitOutcome();
        SyncFieldGuideNarrative();
        return outcome;
    }
    DialogueRecommendation RecommendedDialogueTopic(
        bool allowAdvanceScreening = false) const {
        return dialogue_.RecommendedTopic(DialogueContext(allowAdvanceScreening));
    }
    DialogueSynopsis DialogueSummary() const { return dialogue_.Synopsis(); }
    bool ShouldPromptForDialogueChoice(DialogueTopic topic,
        bool allowAdvanceScreening = false) const {
        return dialogue_.ShouldPromptForChoice(topic, DialogueContext(allowAdvanceScreening));
    }
    static constexpr bool DialogueQuickAdvanceSafe(const DialogueBeat& beat) {
        return LandmarkDialogue::QuickAdvanceSafe(beat);
    }
    const LandmarkDialogue& Dialogue() const { return dialogue_; }

    bool TrackFieldOperation(FieldOperation operation) {
        return fieldGuide_.TrackOperation(operation);
    }
    bool PinFieldTarget(LandmarkKind landmark) { return fieldGuide_.PinTarget(landmark); }
    void ClearPinnedFieldTarget() { fieldGuide_.ClearPinnedTarget(); }
    const ExplorationFieldGuide& FieldGuide() const { return fieldGuide_; }

    bool SetObjectiveActivationMode(LandmarkObjectiveActivationMode mode);
    bool StartObjective();
    LandmarkObjectiveActivationMode ObjectiveActivationMode() const {
        return objectiveActivationMode_;
    }
    bool ObjectiveActive() const { return objectiveStarted_; }

    bool HasSelection() const { return selectedIndex_ < LedgerCapacity; }
    std::size_t SelectedIndex() const { return selectedIndex_; }
    LandmarkKind SelectedKind() const;
    bool IsVisited(LandmarkKind kind) const;
    std::size_t VisitedCount() const;
    std::size_t ObjectiveProgress() const;
    LandmarkObjectiveStage CurrentObjective() const;
    bool ObjectiveComplete() const {
        return objectiveStarted_ && CurrentObjective() == LandmarkObjectiveStage::Complete;
    }
    bool ObjectiveCompletionRewardGranted() const {
        return objectiveCompletionRewardGranted_;
    }
    std::size_t OrderedDiscoveryProgress() const { return orderedDiscoveryProgress_; }
    bool OrderedSequenceIntact() const { return orderedSequenceIntact_; }
    bool OrderedResonanceRewardGranted() const { return orderedResonanceRewardGranted_; }
    const LandmarkInteractionReport& LastReport() const { return lastReport_; }

private:
    LandmarkDialogueContext DialogueContext(bool allowAdvanceScreening) const {
        const std::size_t visited = static_cast<std::size_t>(visited_[0])
            + static_cast<std::size_t>(visited_[1])
            + static_cast<std::size_t>(visited_[2]);
        const bool objectiveComplete = objectiveStarted_
            && objectiveVisited_[0] && objectiveVisited_[1] && objectiveVisited_[2];
        return {visited, objectiveComplete, allowAdvanceScreening};
    }
    static std::size_t IndexOf(LandmarkKind kind);
    void RecordObjectiveVisit(std::size_t index);
    void ApplyObjectiveRewards(ShadowbladeActions& shadowbladeActions,
        float& reward, ProgressionRewardReport& progressionReward);
    void SyncFieldGuideNarrative() {
        fieldGuide_.SyncNarrativeEvidence({
            dialogue_.HasClue(DialogueClue::RiftResidue),
            dialogue_.HasClue(DialogueClue::CoolingAnomaly),
            dialogue_.HasClue(DialogueClue::CryptSigil),
            dialogue_.HasLoreEntry(LoreEntry::ShadowCryptRumor),
        });
    }

    std::array<bool, LedgerCapacity> visited_{};
    std::array<bool, LedgerCapacity> objectiveVisited_{};
    std::size_t selectedIndex_{LedgerCapacity};
    bool objectiveCompletionRewardGranted_{};
    std::size_t orderedDiscoveryProgress_{};
    bool orderedSequenceIntact_{true};
    bool orderedResonanceRewardGranted_{};
    LandmarkObjectiveActivationMode objectiveActivationMode_{
        LandmarkObjectiveActivationMode::AutoStart};
    bool objectiveStarted_{true};
    CharacterProgression* progression_{}; // Non-owning; caller controls the progression lifetime.
    LandmarkDialogue dialogue_{};
    ExplorationFieldGuide fieldGuide_{};
    LandmarkInteractionReport lastReport_{};
};

} // namespace Astral::Scene