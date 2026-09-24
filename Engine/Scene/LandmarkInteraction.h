#pragma once

#include "Engine/Scene/CharacterProgression.h"
#include "Engine/Scene/ExplorationFieldGuide.h"
#include "Engine/Scene/LandmarkDialogue.h"
#include "Engine/Scene/LandmarkDialogueContinuity.h"
#include "Engine/Scene/MallResonancePuzzle.h"
#include "Engine/Scene/ManaReactorMission.h"
#include "Engine/Scene/RiftWardenTrial.h"
#include "Engine/Scene/ShadowCryptMission.h"
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
        dialogueContinuity_.RecordBeat(dialogue_, beat);
        SyncFieldGuideNarrative();
        return beat;
    }
    DialogueOutcome CommitDialogueOutcome() {
        const DialogueOutcome outcome = dialogue_.CommitOutcome();
        dialogueContinuity_.RecordCommittedOutcome(dialogue_, outcome);
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
    DialogueRelationshipTier DialogueRelationship() const {
        return dialogueContinuity_.RelationshipTier(dialogue_);
    }
    DialogueCheckpoint DialogueCheckpointFor(DialogueTopic topic) const {
        return dialogueContinuity_.Checkpoint(topic);
    }
    DialogueRevisit RevisitDialogueCheckpoint(DialogueTopic topic) const {
        return dialogueContinuity_.Revisit(topic);
    }
    DialogueEndingArchive DialogueEndings() const {
        return dialogueContinuity_.EndingArchive();
    }
    bool DialogueStoryEpisodeUnlocked(DialogueStoryEpisode episode) const {
        return dialogueContinuity_.StoryEpisodeUnlocked(episode);
    }
    bool CaptureDialogueInterruption() { return dialogueContinuity_.CaptureInterruption(); }
    InterruptedDialogueBeat InterruptedDialogue() const {
        return dialogueContinuity_.InterruptedBeat();
    }
    bool AcknowledgeInterruptedDialogue() {
        return dialogueContinuity_.AcknowledgeInterruptedBeat();
    }
    const LandmarkDialogueContinuity& DialogueContinuity() const {
        return dialogueContinuity_;
    }
    const LandmarkDialogue& Dialogue() const { return dialogue_; }

    bool TrackFieldOperation(FieldOperation operation) {
        return fieldGuide_.TrackOperation(operation);
    }
    bool TrackNextIncompleteFieldOperation() {
        return fieldGuide_.TrackNextIncompleteOperation();
    }
    bool PinFieldTarget(LandmarkKind landmark) { return fieldGuide_.PinTarget(landmark); }
    void ClearPinnedFieldTarget() { fieldGuide_.ClearPinnedTarget(); }
    bool AddFieldRouteTarget(LandmarkKind landmark,
        FieldPinCategory category = FieldPinCategory::Objective) {
        return fieldGuide_.AddRouteTarget(landmark, category);
    }
    bool RemoveFieldRouteTarget(LandmarkKind landmark) {
        return fieldGuide_.RemoveRouteTarget(landmark);
    }
    std::size_t ClearFieldRouteTargets() { return fieldGuide_.ClearRouteTargets(); }
    bool SetFieldRouteFilter(FieldPinFilter filter) {
        return fieldGuide_.SetRouteFilter(filter);
    }
    bool MarkFieldJournalRead(FieldJournalEntry entry) {
        return fieldGuide_.MarkJournalRead(entry);
    }
    std::size_t MarkAllFieldJournalRead() { return fieldGuide_.MarkAllJournalRead(); }
    FieldGuideBriefing FieldBriefing() const { return fieldGuide_.Briefing(); }
    const ExplorationFieldGuide& FieldGuide() const { return fieldGuide_; }

    bool BeginMallResonancePuzzle(MallResonanceDifficulty difficulty,
        MallResonanceAssistMode assistMode = MallResonanceAssistMode::Off) {
        if (progression_ == nullptr || !ObjectiveComplete()
            || (mallResonanceProgressionOwner_ != nullptr
                && mallResonanceProgressionOwner_ != progression_)) {
            return false;
        }
        if (!mallResonancePuzzle_.Begin(true, difficulty, assistMode)) return false;
        mallResonanceProgressionOwner_ = progression_;
        return true;
    }
    MallResonanceActionReport RotateMallResonanceAnchor(MallResonanceAnchor anchor) {
        return mallResonancePuzzle_.Rotate(anchor);
    }
    bool AdvanceMallResonancePuzzle(float deltaSeconds) {
        return mallResonancePuzzle_.AdvanceTime(deltaSeconds);
    }
    MallResonanceActionReport StabilizeMallResonancePuzzle() {
        return mallResonancePuzzle_.TryStabilize();
    }
    bool ResetMallResonancePuzzle() { return mallResonancePuzzle_.ResetActiveRun(); }
    MallResonanceBriefing MallResonancePuzzleBriefing() const {
        MallResonanceBriefing briefing = mallResonancePuzzle_.Briefing();
        if (progression_ == nullptr || progression_ != mallResonanceProgressionOwner_
            || progression_->MallResonanceFirstClearClaimed()) {
            briefing.firstClearRewardAvailable = false;
        }
        return briefing;
    }
    MallResonanceRecord MallResonanceBestRecord(MallResonanceDifficulty difficulty) const {
        return mallResonancePuzzle_.BestRecord(difficulty);
    }
    MallResonanceRewardReport ClaimMallResonanceFirstClearReward() {
        MallResonanceRewardReport report{};
        if (progression_ == nullptr || progression_ != mallResonanceProgressionOwner_
            || progression_->MallResonanceFirstClearClaimed()) {
            return report;
        }
        report = mallResonancePuzzle_.ClaimFirstClearReward(*progression_);
        if (!report.granted) return report;
        bool ledgerGranted = false;
        progression_->ClaimMallResonanceFirstClearReward(0, 0, 0, ledgerGranted);
        if (!ledgerGranted) return {};
        return report;
    }
    const MallResonancePuzzle& MallResonancePuzzleState() const {
        return mallResonancePuzzle_;
    }

    bool BeginManaReactor(ManaReactorMode mode = ManaReactorMode::Expedition,
        ManaReactorDifficulty difficulty = ManaReactorDifficulty::Standard,
        ManaReactorProtocol protocol = ManaReactorProtocol::Baseline) {
        return manaReactorMission_.Begin(fieldGuide_, mode, difficulty, protocol);
    }
    ManaReactorControlResult ApplyManaReactorControl(ManaReactorControl control) {
        return manaReactorMission_.ApplyControl(control);
    }
    bool UseManaReactorEmergencyVent() { return manaReactorMission_.UseEmergencyVent(); }
    bool RetryManaReactorStage() { return manaReactorMission_.RetryCurrentStage(); }
    bool ReplayCompletedManaReactor() { return manaReactorMission_.ReplayCompletedRun(); }
    ManaReactorMissionBriefing ManaReactorBriefing() const {
        return manaReactorMission_.Briefing();
    }
    ManaReactorMissionRecord ManaReactorBestRecord(ManaReactorDifficulty difficulty,
        ManaReactorProtocol protocol) const {
        return manaReactorMission_.BestRecord(difficulty, protocol);
    }
    ManaReactorRewardReport ClaimManaReactorFirstClearReward() {
        if (progression_ == nullptr) return {};
        return manaReactorMission_.ClaimFirstClearReward(*progression_);
    }
    const ManaReactorMission& ManaReactor() const { return manaReactorMission_; }

    // Pass 27 production-owned Shadow Crypt path. These methods expose the
    // already-merged expedition rules through the same live landmark/narrative
    // owner that produces the authoritative ShadowCryptLead evidence.
    bool BeginShadowCrypt() {
        if (progression_ == nullptr || !shadowCryptMission_.Begin(fieldGuide_)) return false;
        // Bind reward/replay authority to the protagonist owner that entered the
        // run. A later pointer swap cannot redirect a completed clear to a
        // different progression object.
        shadowCryptProgressionOwner_ = progression_;
        return true;
    }
    bool AdvanceShadowCryptObjective() { return shadowCryptMission_.RecordObjectiveStep(); }
    bool DiscoverShadowCryptCoolingCache() {
        return shadowCryptMission_.DiscoverCoolingCache();
    }
    bool ClearShadowCryptCoolingCache() { return shadowCryptMission_.ClearCoolingCache(); }
    int RecordShadowCryptDamageTaken(int amount) {
        return shadowCryptMission_.RecordDamageTaken(amount);
    }
    bool ReportShadowCryptDefeat() { return shadowCryptMission_.ReportDefeat(); }
    bool SuspendShadowCrypt() { return shadowCryptMission_.SuspendAtSafeBoundary(); }
    bool ResumeShadowCrypt() { return shadowCryptMission_.ResumeSuspendedRun(); }
    bool ReplayCompletedShadowCrypt() {
        if (progression_ != shadowCryptProgressionOwner_) return false;
        return shadowCryptMission_.ReplayCompletedRun(fieldGuide_);
    }
    ShadowCryptMissionBriefing ShadowCryptBriefing() const {
        return shadowCryptMission_.Briefing();
    }
    ShadowCryptMissionRecord ShadowCryptBestRecord() const {
        return shadowCryptMission_.BestRecord();
    }
    ShadowCryptRewardReport ClaimShadowCryptFirstClearReward() {
        if (progression_ == nullptr || progression_ != shadowCryptProgressionOwner_) return {};
        return shadowCryptMission_.ClaimFirstClearReward(*progression_);
    }
    const ShadowCryptMission& ShadowCrypt() const { return shadowCryptMission_; }

    // Pass 32 game-owned Rift Warden mastery trial. Entry is dependency-ready:
    // it consumes the already-authoritative completed Shadow Crypt state and a
    // persistent protagonist owner, without changing engine/runtime facilities.
    bool BeginRiftWardenTrial(RiftWardenDifficulty difficulty) {
        if (progression_ == nullptr || progression_ != shadowCryptProgressionOwner_
            || !shadowCryptMission_.Briefing().complete
            || (riftWardenProgressionOwner_ != nullptr
                && riftWardenProgressionOwner_ != progression_)) {
            return false;
        }
        if (!riftWardenTrial_.Begin(true, difficulty)) return false;
        riftWardenProgressionOwner_ = progression_;
        return true;
    }
    bool BeginRiftWardenPractice(RiftWardenDifficulty difficulty,
        RiftWardenPhase phase) {
        if (progression_ == nullptr || progression_ != riftWardenProgressionOwner_
            || !shadowCryptMission_.Briefing().complete) {
            return false;
        }
        return riftWardenTrial_.BeginPractice(true, difficulty, phase);
    }
    bool BeginFocusedRiftWardenPractice(RiftWardenDifficulty difficulty,
        RiftWardenAttack attack,
        RiftWardenPracticePace pace = RiftWardenPracticePace::Standard,
        bool looping = false) {
        if (progression_ == nullptr || progression_ != riftWardenProgressionOwner_
            || !shadowCryptMission_.Briefing().complete) {
            return false;
        }
        return riftWardenTrial_.BeginFocusedPractice(
            true, difficulty, attack, pace, looping);
    }
    bool SetRiftWardenPracticePace(RiftWardenPracticePace pace) {
        if (progression_ == nullptr || progression_ != riftWardenProgressionOwner_) return false;
        return riftWardenTrial_.SetPracticePace(pace);
    }
    bool SetRiftWardenPracticePaused(bool paused) {
        if (progression_ == nullptr || progression_ != riftWardenProgressionOwner_) return false;
        return riftWardenTrial_.SetPracticePaused(paused);
    }
    bool SetRiftWardenLoopingFocusedPractice(bool looping) {
        if (progression_ == nullptr || progression_ != riftWardenProgressionOwner_) return false;
        return riftWardenTrial_.SetLoopingFocusedPractice(looping);
    }
    RiftWardenActionReport ResolveRiftWardenAction(RiftWardenResponse response,
        double reactionSeconds) {
        if (progression_ == nullptr || progression_ != riftWardenProgressionOwner_) return {};
        return riftWardenTrial_.Resolve(response, reactionSeconds);
    }
    bool AdvanceRiftWardenTrial(double deltaSeconds) {
        if (progression_ == nullptr || progression_ != riftWardenProgressionOwner_) return false;
        return riftWardenTrial_.AdvanceTime(deltaSeconds);
    }
    RiftWardenBriefing RiftWardenTrialBriefing() const {
        return riftWardenTrial_.Briefing();
    }
    RiftWardenAttackGuide RiftWardenAttackGuideFor(RiftWardenAttack attack,
        RiftWardenDifficulty difficulty) const {
        return riftWardenTrial_.AttackGuide(attack, difficulty);
    }
    RiftWardenRecord RiftWardenBestRecord(RiftWardenDifficulty difficulty) const {
        return riftWardenTrial_.BestRecord(difficulty);
    }
    bool RiftWardenDifficultyUnlocked(RiftWardenDifficulty difficulty) const {
        return riftWardenTrial_.DifficultyUnlocked(difficulty);
    }
    const RiftWardenTrial& RiftWarden() const { return riftWardenTrial_; }

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
    CharacterProgression* shadowCryptProgressionOwner_{}; // Identity only; never dereferenced directly.
    CharacterProgression* mallResonanceProgressionOwner_{}; // Identity only; puzzle reward authority.
    CharacterProgression* riftWardenProgressionOwner_{}; // Identity only; trial action authority.
    LandmarkDialogue dialogue_{};
    LandmarkDialogueContinuity dialogueContinuity_{};
    ExplorationFieldGuide fieldGuide_{};
    MallResonancePuzzle mallResonancePuzzle_{};
    ManaReactorMission manaReactorMission_{};
    ShadowCryptMission shadowCryptMission_{};
    RiftWardenTrial riftWardenTrial_{};
    LandmarkInteractionReport lastReport_{};
};

} // namespace Astral::Scene
