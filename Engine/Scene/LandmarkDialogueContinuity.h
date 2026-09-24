#pragma once

#include "Engine/Scene/LandmarkDialogue.h"

#include <array>
#include <cstddef>
#include <limits>

namespace Astral::Scene {

enum class DialogueRelationshipTier {
    Wary,
    Acquainted,
    Confidant,
};

enum class DialogueStoryEpisode {
    MallWitness,
    RiftConfidant,
    CryptPartner,
    Count,
};

struct DialogueCheckpoint {
    bool available{};
    DialogueTopic topic{DialogueTopic::RiftTheory};
    DialogueBeat beat{};
    std::size_t sequence{};
};

struct DialogueRevisit {
    bool available{};
    DialogueBeat beat{};
    std::size_t originalSequence{};
    bool mutatesStory{};
};

struct DialogueEndingArchive {
    bool aidCivilians{};
    bool pursueRift{};
    std::size_t unlockedCount{};
};

struct InterruptedDialogueBeat {
    bool pending{};
    DialogueBeat beat{};
    std::size_t sequence{};
};

class LandmarkDialogueContinuity {
public:
    static constexpr std::size_t EpisodeCount =
        static_cast<std::size_t>(DialogueStoryEpisode::Count);

    constexpr bool RecordBeat(const LandmarkDialogue& dialogue, const DialogueBeat& beat) {
        const std::size_t topicIndex = TopicIndex(beat.topic);
        if (beat.response == DialogueResponse::Invalid
            || topicIndex >= LandmarkDialogue::TopicCount
            || sequence_ == std::numeric_limits<std::size_t>::max()) {
            return false;
        }

        ++sequence_;
        latestBeat_ = beat;
        latestSequence_ = sequence_;
        hasLatestBeat_ = true;

        if (IsCheckpointBeat(beat)) {
            checkpoints_[topicIndex] = {true, beat.topic, beat, sequence_};
        }

        SyncEpisodes(dialogue);
        return true;
    }

    constexpr DialogueRelationshipTier RelationshipTier(const LandmarkDialogue& dialogue) const {
        if (dialogue.Trust() <= -1) return DialogueRelationshipTier::Wary;
        if (dialogue.Trust() >= 2) return DialogueRelationshipTier::Confidant;
        return DialogueRelationshipTier::Acquainted;
    }

    constexpr DialogueCheckpoint Checkpoint(DialogueTopic topic) const {
        const std::size_t index = TopicIndex(topic);
        if (index >= LandmarkDialogue::TopicCount) return {};
        return checkpoints_[index];
    }

    constexpr DialogueRevisit Revisit(DialogueTopic topic) const {
        const DialogueCheckpoint checkpoint = Checkpoint(topic);
        if (!checkpoint.available) return {};
        return {true, checkpoint.beat, checkpoint.sequence, false};
    }

    constexpr bool RecordCommittedOutcome(const LandmarkDialogue& dialogue,
        DialogueOutcome outcome) {
        if (outcome == DialogueOutcome::None || dialogue.Outcome() != outcome) return false;

        bool* destination = nullptr;
        if (outcome == DialogueOutcome::AidCivilians) {
            destination = &aidCiviliansEnding_;
        } else if (outcome == DialogueOutcome::PursueRift) {
            destination = &pursueRiftEnding_;
        }
        if (destination == nullptr || *destination) return false;
        *destination = true;
        SyncEpisodes(dialogue);
        return true;
    }

    constexpr DialogueEndingArchive EndingArchive() const {
        return {
            aidCiviliansEnding_,
            pursueRiftEnding_,
            static_cast<std::size_t>(aidCiviliansEnding_)
                + static_cast<std::size_t>(pursueRiftEnding_),
        };
    }

    constexpr bool StoryEpisodeUnlocked(DialogueStoryEpisode episode) const {
        const std::size_t index = EpisodeIndex(episode);
        return index < EpisodeCount && episodes_[index];
    }

    constexpr bool CaptureInterruption() {
        if (interruptionPending_ || !hasLatestBeat_
            || latestSequence_ <= acknowledgedSequence_) {
            return false;
        }
        interruptionPending_ = true;
        interruptedBeat_ = latestBeat_;
        interruptedSequence_ = latestSequence_;
        return true;
    }

    constexpr InterruptedDialogueBeat InterruptedBeat() const {
        if (!interruptionPending_) return {};
        return {true, interruptedBeat_, interruptedSequence_};
    }

    constexpr bool AcknowledgeInterruptedBeat() {
        if (!interruptionPending_) return false;
        acknowledgedSequence_ = interruptedSequence_;
        interruptionPending_ = false;
        interruptedBeat_ = {};
        interruptedSequence_ = 0;
        return true;
    }

    constexpr std::size_t Sequence() const { return sequence_; }

private:
    static constexpr std::size_t TopicIndex(DialogueTopic topic) {
        return static_cast<std::size_t>(topic);
    }

    static constexpr std::size_t EpisodeIndex(DialogueStoryEpisode episode) {
        return static_cast<std::size_t>(episode);
    }

    static constexpr bool IsCheckpointBeat(const DialogueBeat& beat) {
        return !beat.previewed
            && beat.response != DialogueResponse::Invalid
            && beat.response != DialogueResponse::ConversationClosed
            && beat.response != DialogueResponse::AlreadyDiscussed;
    }

    constexpr void SyncEpisodes(const LandmarkDialogue& dialogue) {
        if (dialogue.HasLoreEntry(LoreEntry::NationalMallResonance)) {
            episodes_[EpisodeIndex(DialogueStoryEpisode::MallWitness)] = true;
        }
        if ((dialogue.UnlockedClueCount() >= 2
                && RelationshipTier(dialogue) != DialogueRelationshipTier::Wary)
            || pursueRiftEnding_) {
            episodes_[EpisodeIndex(DialogueStoryEpisode::RiftConfidant)] = true;
        }
        if (RelationshipTier(dialogue) == DialogueRelationshipTier::Confidant
            && dialogue.HasLoreEntry(LoreEntry::ShadowCryptRumor)
            && dialogue.HasClue(DialogueClue::CryptSigil)) {
            episodes_[EpisodeIndex(DialogueStoryEpisode::CryptPartner)] = true;
        }
    }

    std::array<DialogueCheckpoint, LandmarkDialogue::TopicCount> checkpoints_{};
    std::array<bool, EpisodeCount> episodes_{};
    std::size_t sequence_{};
    DialogueBeat latestBeat_{};
    std::size_t latestSequence_{};
    std::size_t acknowledgedSequence_{};
    bool hasLatestBeat_{};
    bool aidCiviliansEnding_{};
    bool pursueRiftEnding_{};
    bool interruptionPending_{};
    DialogueBeat interruptedBeat_{};
    std::size_t interruptedSequence_{};
};

namespace Detail {
constexpr bool LandmarkDialogueContinuityContract() {
    LandmarkDialogue dialogue;
    LandmarkDialogueContinuity continuity;

    const DialogueBeat direct = dialogue.Choose(
        DialogueTopic::RiftTheory, DialogueChoice::AskDirectly);
    if (!continuity.RecordBeat(dialogue, direct)
        || continuity.Sequence() != 1
        || continuity.RelationshipTier(dialogue) != DialogueRelationshipTier::Acquainted) {
        return false;
    }

    const DialogueCheckpoint first = continuity.Checkpoint(DialogueTopic::RiftTheory);
    if (!first.available || first.sequence != 1
        || first.beat.response != DialogueResponse::RiftTheoryBrief) return false;

    const DialogueRevisit revisit = continuity.Revisit(DialogueTopic::RiftTheory);
    if (!revisit.available || revisit.mutatesStory || revisit.originalSequence != 1
        || dialogue.Trust() != 0 || dialogue.HistoryCount() != 1) return false;

    if (!continuity.CaptureInterruption()) return false;
    const InterruptedDialogueBeat interrupted = continuity.InterruptedBeat();
    if (!interrupted.pending || interrupted.sequence != 1
        || interrupted.beat.response != DialogueResponse::RiftTheoryBrief) return false;
    if (continuity.CaptureInterruption()) return false;
    if (!continuity.AcknowledgeInterruptedBeat()
        || continuity.AcknowledgeInterruptedBeat()
        || continuity.InterruptedBeat().pending
        || continuity.CaptureInterruption()) return false;

    const DialogueBeat nextBeat = dialogue.Choose(
        DialogueTopic::CivilianSafety, DialogueChoice::AskDirectly);
    if (!continuity.RecordBeat(dialogue, nextBeat)
        || !continuity.CaptureInterruption()
        || continuity.InterruptedBeat().sequence != 2
        || !continuity.AcknowledgeInterruptedBeat()) return false;

    DialogueBeat invalidTopic = direct;
    invalidTopic.topic = static_cast<DialogueTopic>(999);
    if (continuity.RecordBeat(dialogue, invalidTopic)
        || continuity.Sequence() != 2) return false;

    LandmarkDialogue confidant;
    LandmarkDialogueContinuity confidantContinuity;
    const DialogueBeat civilian = confidant.Choose(
        DialogueTopic::CivilianSafety, DialogueChoice::Reassure);
    confidantContinuity.RecordBeat(confidant, civilian);
    const DialogueBeat history = confidant.Choose(
        DialogueTopic::LandmarkHistory, DialogueChoice::Reassure, {3, true});
    confidantContinuity.RecordBeat(confidant, history);
    if (confidantContinuity.RelationshipTier(confidant)
        != DialogueRelationshipTier::Confidant) return false;
    if (!confidantContinuity.StoryEpisodeUnlocked(DialogueStoryEpisode::MallWitness)) {
        return false;
    }

    const DialogueBeat rift = confidant.Choose(
        DialogueTopic::RiftTheory, DialogueChoice::ShareEvidence, {2, false});
    confidantContinuity.RecordBeat(confidant, rift);
    const DialogueBeat landmark = confidant.Choose(
        DialogueTopic::LandmarkHistory, DialogueChoice::ShareEvidence, {3, true});
    confidantContinuity.RecordBeat(confidant, landmark);
    if (!confidantContinuity.StoryEpisodeUnlocked(DialogueStoryEpisode::RiftConfidant)) {
        return false;
    }

    const DialogueBeat crypt = confidant.Choose(
        DialogueTopic::ShadowCrypt, DialogueChoice::ShareEvidence, {3, true});
    confidantContinuity.RecordBeat(confidant, crypt);
    if (!confidantContinuity.StoryEpisodeUnlocked(DialogueStoryEpisode::CryptPartner)) {
        return false;
    }

    if (confidant.CommitOutcome() != DialogueOutcome::AidCivilians) return false;
    if (!confidantContinuity.RecordCommittedOutcome(confidant, DialogueOutcome::AidCivilians)
        || confidantContinuity.RecordCommittedOutcome(confidant, DialogueOutcome::AidCivilians)
        || confidantContinuity.RecordCommittedOutcome(confidant, DialogueOutcome::PursueRift)) {
        return false;
    }
    const DialogueEndingArchive endings = confidantContinuity.EndingArchive();
    if (!endings.aidCivilians || endings.pursueRift || endings.unlockedCount != 1) return false;

    LandmarkDialogue wary;
    LandmarkDialogueContinuity waryContinuity;
    const DialogueBeat challenge = wary.Choose(
        DialogueTopic::RiftTheory, DialogueChoice::Challenge);
    waryContinuity.RecordBeat(wary, challenge);
    if (waryContinuity.RelationshipTier(wary) != DialogueRelationshipTier::Wary) return false;

    return true;
}
static_assert(LandmarkDialogueContinuityContract(),
    "landmark dialogue continuity regression");
} // namespace Detail

} // namespace Astral::Scene
