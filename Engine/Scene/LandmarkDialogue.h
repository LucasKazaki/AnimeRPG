#pragma once

#include <array>
#include <cstddef>

namespace Astral::Scene {

enum class DialogueTopic {
    RiftTheory,
    CivilianSafety,
    LandmarkHistory,
    ShadowCrypt,
    ManaReactor,
    Count,
};

enum class DialogueChoice {
    AskDirectly,
    Reassure,
    Challenge,
    ShareEvidence,
    EndConversation,
};

enum class DialogueResponse {
    Invalid,
    RiftTheoryBrief,
    RiftTheoryReassured,
    RiftTheoryChallenged,
    RiftEvidenceAcknowledged,
    CivilianRiskBrief,
    CivilianRouteShared,
    CivilianRiskChallenged,
    CivilianEvidenceAcknowledged,
    LandmarkHistoryBrief,
    LandmarkHistoryReassured,
    LandmarkHistoryChallenged,
    MonumentPatternConfirmed,
    ShadowCryptBrief,
    ShadowCryptReassured,
    ShadowCryptChallenged,
    CryptSigilConfirmed,
    ManaReactorBrief,
    ManaReactorReassured,
    ManaReactorChallenged,
    CoolingTraceConfirmed,
    ConversationClosed,
    AlreadyDiscussed,
};

enum class DialogueClue {
    RiftResidue,
    CoolingAnomaly,
    CryptSigil,
    Count,
};

enum class LoreEntry {
    NationalMallResonance,
    QuantumCoolingAnomaly,
    ShadowCryptRumor,
    Count,
};

enum class DialogueOutcome {
    None,
    AidCivilians,
    PursueRift,
};

enum class DialogueAvailability {
    Locked,
    Available,
    AdvancePreview,
};

struct LandmarkDialogueContext {
    std::size_t visitedLandmarks{};
    bool objectiveComplete{};
    bool allowAdvanceScreening{};
};

struct DialogueBeat {
    DialogueTopic topic{DialogueTopic::RiftTheory};
    DialogueChoice choice{DialogueChoice::AskDirectly};
    DialogueResponse response{DialogueResponse::Invalid};
    int trustDelta{};
    bool repeatedTopic{};
    bool clueUnlocked{};
    bool loreUnlocked{};
    bool previewed{};
    bool autoAdvanceRecommended{};
};

struct DialogueSynopsis {
    std::size_t topicsDiscussed{};
    std::size_t cluesUnlocked{};
    std::size_t loreUnlocked{};
    std::size_t retainedPreviewBeats{};
    int trust{};
    DialogueOutcome outcome{DialogueOutcome::None};
    DialogueResponse latestResponse{DialogueResponse::Invalid};
};

struct DialogueRecommendation {
    bool available{};
    DialogueTopic topic{DialogueTopic::RiftTheory};
    DialogueAvailability availability{DialogueAvailability::Locked};
};

class LandmarkDialogue {
public:
    static constexpr int MinimumTrust = -3;
    static constexpr int MaximumTrust = 3;
    static constexpr std::size_t TopicCount = static_cast<std::size_t>(DialogueTopic::Count);
    static constexpr std::size_t ClueCount = static_cast<std::size_t>(DialogueClue::Count);
    static constexpr std::size_t LoreCount = static_cast<std::size_t>(LoreEntry::Count);
    static constexpr std::size_t HistoryCapacity = 8;

    constexpr DialogueBeat Choose(DialogueTopic topic, DialogueChoice choice,
        LandmarkDialogueContext context = {}) {
        DialogueBeat beat{topic, choice, DialogueResponse::Invalid};
        const std::size_t topicIndex = Index(topic);
        if (topicIndex >= TopicCount || !ValidChoice(choice)) return beat;

        const DialogueAvailability availability = TopicAvailability(topic, context);
        if (choice != DialogueChoice::EndConversation
            && availability == DialogueAvailability::Locked) {
            return beat;
        }

        const bool repeated = discussed_[topicIndex];
        beat.repeatedTopic = repeated;
        beat.previewed = choice != DialogueChoice::EndConversation
            && availability == DialogueAvailability::AdvancePreview;
        beat.autoAdvanceRecommended = choice == DialogueChoice::EndConversation
            || !ShouldPromptForChoice(topic, context);
        beat.response = ResolveResponse(topic, choice, context, repeated);
        if (beat.response == DialogueResponse::Invalid) return beat;

        if (beat.previewed) {
            PushHistory(beat);
            return beat;
        }

        if (repeated) {
            if (choice == DialogueChoice::ShareEvidence) {
                beat.clueUnlocked = UnlockClueFor(topic, choice, context);
                if (beat.clueUnlocked) beat.response = EvidenceResponse(topic, context);
            }
        } else if (choice != DialogueChoice::EndConversation) {
            discussed_[topicIndex] = true;
            beat.loreUnlocked = UnlockLoreForTopic(topic);
            beat.clueUnlocked = UnlockClueFor(topic, choice, context);
            const int previousTrust = trust_;
            trust_ = ClampTrust(trust_ + TrustDelta(choice));
            beat.trustDelta = trust_ - previousTrust;
        }

        PushHistory(beat);
        return beat;
    }

    constexpr DialogueAvailability TopicAvailability(DialogueTopic topic,
        LandmarkDialogueContext context = {}) const {
        const std::size_t topicIndex = Index(topic);
        if (topicIndex >= TopicCount) return DialogueAvailability::Locked;
        if (discussed_[topicIndex]) return DialogueAvailability::Available;

        bool available = false;
        switch (topic) {
        case DialogueTopic::RiftTheory:
        case DialogueTopic::CivilianSafety:
        case DialogueTopic::LandmarkHistory:
            available = true;
            break;
        case DialogueTopic::ShadowCrypt:
            available = context.objectiveComplete || UnlockedClueCount() >= 2;
            break;
        case DialogueTopic::ManaReactor:
            available = context.visitedLandmarks >= 3
                || HasClue(DialogueClue::CoolingAnomaly);
            break;
        case DialogueTopic::Count:
            return DialogueAvailability::Locked;
        }

        if (available) return DialogueAvailability::Available;
        return context.allowAdvanceScreening
            ? DialogueAvailability::AdvancePreview
            : DialogueAvailability::Locked;
    }

    constexpr DialogueRecommendation RecommendedTopic(
        LandmarkDialogueContext context = {}) const {
        constexpr std::array<DialogueTopic, TopicCount> priority{
            DialogueTopic::RiftTheory,
            DialogueTopic::CivilianSafety,
            DialogueTopic::LandmarkHistory,
            DialogueTopic::ShadowCrypt,
            DialogueTopic::ManaReactor,
        };

        for (DialogueTopic topic : priority) {
            if (Discussed(topic)) continue;
            const DialogueAvailability availability = TopicAvailability(topic, context);
            if (availability == DialogueAvailability::Available) {
                return {true, topic, availability};
            }
        }
        if (context.allowAdvanceScreening) {
            for (DialogueTopic topic : priority) {
                if (Discussed(topic)) continue;
                const DialogueAvailability availability = TopicAvailability(topic, context);
                if (availability == DialogueAvailability::AdvancePreview) {
                    return {true, topic, availability};
                }
            }
        }
        return {};
    }

    constexpr bool ShouldPromptForChoice(DialogueTopic topic,
        LandmarkDialogueContext context = {}) const {
        if (TopicAvailability(topic, context) == DialogueAvailability::Locked) return false;
        constexpr std::array<DialogueChoice, 4> choices{
            DialogueChoice::AskDirectly,
            DialogueChoice::Reassure,
            DialogueChoice::Challenge,
            DialogueChoice::ShareEvidence,
        };
        std::array<DialogueResponse, 4> distinct{};
        std::size_t distinctCount = 0;
        for (DialogueChoice choice : choices) {
            const DialogueResponse response = PreviewResponse(topic, choice, context);
            if (response == DialogueResponse::Invalid) continue;
            bool seen = false;
            for (std::size_t index = 0; index < distinctCount; ++index) {
                if (distinct[index] == response) {
                    seen = true;
                    break;
                }
            }
            if (!seen) distinct[distinctCount++] = response;
        }
        return distinctCount > 1;
    }

    static constexpr bool QuickAdvanceSafe(const DialogueBeat& beat) {
        if (beat.response == DialogueResponse::Invalid
            || beat.trustDelta != 0 || beat.clueUnlocked || beat.loreUnlocked) {
            return false;
        }
        return beat.response == DialogueResponse::ConversationClosed
            || beat.response == DialogueResponse::AlreadyDiscussed
            || beat.autoAdvanceRecommended;
    }

    constexpr DialogueSynopsis Synopsis() const {
        DialogueSynopsis synopsis{};
        for (bool discussed : discussed_) {
            synopsis.topicsDiscussed += static_cast<std::size_t>(discussed);
        }
        synopsis.cluesUnlocked = UnlockedClueCount();
        synopsis.loreUnlocked = UnlockedLoreCount();
        for (std::size_t index = 0; index < historyCount_; ++index) {
            synopsis.retainedPreviewBeats += static_cast<std::size_t>(history_[index].previewed);
        }
        synopsis.trust = trust_;
        synopsis.outcome = outcome_;
        if (historyCount_ > 0) synopsis.latestResponse = HistoryFromNewest(0).response;
        return synopsis;
    }

    constexpr int Trust() const { return trust_; }
    constexpr bool Discussed(DialogueTopic topic) const {
        const std::size_t index = Index(topic);
        return index < TopicCount && discussed_[index];
    }
    constexpr bool HasClue(DialogueClue clue) const {
        const std::size_t index = Index(clue);
        return index < ClueCount && clues_[index];
    }
    constexpr bool HasLoreEntry(LoreEntry entry) const {
        const std::size_t index = Index(entry);
        return index < LoreCount && lore_[index];
    }
    constexpr std::size_t UnlockedClueCount() const {
        std::size_t count = 0;
        for (bool unlocked : clues_) count += static_cast<std::size_t>(unlocked);
        return count;
    }
    constexpr std::size_t UnlockedLoreCount() const {
        std::size_t count = 0;
        for (bool unlocked : lore_) count += static_cast<std::size_t>(unlocked);
        return count;
    }

    constexpr DialogueOutcome CommitOutcome() {
        if (outcome_ != DialogueOutcome::None) return outcome_;
        if (trust_ >= 2 && Discussed(DialogueTopic::CivilianSafety)) {
            outcome_ = DialogueOutcome::AidCivilians;
        } else if (UnlockedClueCount() >= 2) {
            outcome_ = DialogueOutcome::PursueRift;
        }
        return outcome_;
    }
    constexpr DialogueOutcome Outcome() const { return outcome_; }

    constexpr std::size_t HistoryCount() const { return historyCount_; }
    constexpr DialogueBeat HistoryFromNewest(std::size_t offset) const {
        if (offset >= historyCount_) return {};
        const std::size_t newest = (historyWrite_ + HistoryCapacity - 1) % HistoryCapacity;
        const std::size_t index = (newest + HistoryCapacity - offset) % HistoryCapacity;
        return history_[index];
    }

private:
    static constexpr bool ValidChoice(DialogueChoice choice) {
        switch (choice) {
        case DialogueChoice::AskDirectly:
        case DialogueChoice::Reassure:
        case DialogueChoice::Challenge:
        case DialogueChoice::ShareEvidence:
        case DialogueChoice::EndConversation:
            return true;
        }
        return false;
    }

    static constexpr int TrustDelta(DialogueChoice choice) {
        switch (choice) {
        case DialogueChoice::Reassure:
        case DialogueChoice::ShareEvidence:
            return 1;
        case DialogueChoice::Challenge:
            return -1;
        case DialogueChoice::AskDirectly:
        case DialogueChoice::EndConversation:
            return 0;
        }
        return 0;
    }

    static constexpr int ClampTrust(int value) {
        if (value < MinimumTrust) return MinimumTrust;
        if (value > MaximumTrust) return MaximumTrust;
        return value;
    }

    static constexpr std::size_t Index(DialogueTopic topic) {
        return static_cast<std::size_t>(topic);
    }
    static constexpr std::size_t Index(DialogueClue clue) {
        return static_cast<std::size_t>(clue);
    }
    static constexpr std::size_t Index(LoreEntry entry) {
        return static_cast<std::size_t>(entry);
    }

    constexpr DialogueResponse ResolveResponse(DialogueTopic topic, DialogueChoice choice,
        LandmarkDialogueContext context, bool repeated) const {
        if (choice == DialogueChoice::EndConversation) return DialogueResponse::ConversationClosed;
        if (repeated) return DialogueResponse::AlreadyDiscussed;

        switch (topic) {
        case DialogueTopic::RiftTheory:
            switch (choice) {
            case DialogueChoice::AskDirectly: return DialogueResponse::RiftTheoryBrief;
            case DialogueChoice::Reassure: return DialogueResponse::RiftTheoryReassured;
            case DialogueChoice::Challenge: return DialogueResponse::RiftTheoryChallenged;
            case DialogueChoice::ShareEvidence:
                return context.visitedLandmarks >= 2
                    ? DialogueResponse::RiftEvidenceAcknowledged
                    : DialogueResponse::RiftTheoryBrief;
            case DialogueChoice::EndConversation: break;
            }
            break;
        case DialogueTopic::CivilianSafety:
            switch (choice) {
            case DialogueChoice::AskDirectly: return DialogueResponse::CivilianRiskBrief;
            case DialogueChoice::Reassure: return DialogueResponse::CivilianRouteShared;
            case DialogueChoice::Challenge: return DialogueResponse::CivilianRiskChallenged;
            case DialogueChoice::ShareEvidence: return DialogueResponse::CivilianEvidenceAcknowledged;
            case DialogueChoice::EndConversation: break;
            }
            break;
        case DialogueTopic::LandmarkHistory:
            switch (choice) {
            case DialogueChoice::AskDirectly: return DialogueResponse::LandmarkHistoryBrief;
            case DialogueChoice::Reassure: return DialogueResponse::LandmarkHistoryReassured;
            case DialogueChoice::Challenge: return DialogueResponse::LandmarkHistoryChallenged;
            case DialogueChoice::ShareEvidence:
                return context.objectiveComplete
                    ? DialogueResponse::MonumentPatternConfirmed
                    : DialogueResponse::LandmarkHistoryBrief;
            case DialogueChoice::EndConversation: break;
            }
            break;
        case DialogueTopic::ShadowCrypt:
            switch (choice) {
            case DialogueChoice::AskDirectly: return DialogueResponse::ShadowCryptBrief;
            case DialogueChoice::Reassure: return DialogueResponse::ShadowCryptReassured;
            case DialogueChoice::Challenge: return DialogueResponse::ShadowCryptChallenged;
            case DialogueChoice::ShareEvidence:
                return trust_ >= 1
                    ? DialogueResponse::CryptSigilConfirmed
                    : DialogueResponse::ShadowCryptBrief;
            case DialogueChoice::EndConversation: break;
            }
            break;
        case DialogueTopic::ManaReactor:
            switch (choice) {
            case DialogueChoice::AskDirectly: return DialogueResponse::ManaReactorBrief;
            case DialogueChoice::Reassure: return DialogueResponse::ManaReactorReassured;
            case DialogueChoice::Challenge: return DialogueResponse::ManaReactorChallenged;
            case DialogueChoice::ShareEvidence:
                return HasClue(DialogueClue::CoolingAnomaly)
                    ? DialogueResponse::CoolingTraceConfirmed
                    : DialogueResponse::ManaReactorBrief;
            case DialogueChoice::EndConversation: break;
            }
            break;
        case DialogueTopic::Count:
            break;
        }
        return DialogueResponse::Invalid;
    }

    constexpr DialogueResponse PreviewResponse(DialogueTopic topic, DialogueChoice choice,
        LandmarkDialogueContext context) const {
        const bool repeated = Discussed(topic);
        DialogueResponse response = ResolveResponse(topic, choice, context, repeated);
        if (repeated && choice == DialogueChoice::ShareEvidence
            && CanUnlockClueFor(topic, choice, context)) {
            response = EvidenceResponse(topic, context);
        }
        return response;
    }

    constexpr DialogueResponse EvidenceResponse(DialogueTopic topic,
        LandmarkDialogueContext context) const {
        switch (topic) {
        case DialogueTopic::RiftTheory:
            return context.visitedLandmarks >= 2
                ? DialogueResponse::RiftEvidenceAcknowledged
                : DialogueResponse::AlreadyDiscussed;
        case DialogueTopic::LandmarkHistory:
            return context.objectiveComplete
                ? DialogueResponse::MonumentPatternConfirmed
                : DialogueResponse::AlreadyDiscussed;
        case DialogueTopic::ShadowCrypt:
            return trust_ >= 1
                ? DialogueResponse::CryptSigilConfirmed
                : DialogueResponse::AlreadyDiscussed;
        case DialogueTopic::ManaReactor:
            return HasClue(DialogueClue::CoolingAnomaly)
                ? DialogueResponse::CoolingTraceConfirmed
                : DialogueResponse::AlreadyDiscussed;
        case DialogueTopic::CivilianSafety:
        case DialogueTopic::Count:
            return DialogueResponse::AlreadyDiscussed;
        }
        return DialogueResponse::AlreadyDiscussed;
    }

    constexpr bool CanUnlockClueFor(DialogueTopic topic, DialogueChoice choice,
        LandmarkDialogueContext context) const {
        if (choice != DialogueChoice::ShareEvidence) return false;
        DialogueClue clue = DialogueClue::RiftResidue;
        bool qualifies = false;
        if (topic == DialogueTopic::RiftTheory && context.visitedLandmarks >= 2) {
            clue = DialogueClue::RiftResidue;
            qualifies = true;
        } else if (topic == DialogueTopic::LandmarkHistory && context.objectiveComplete) {
            clue = DialogueClue::CoolingAnomaly;
            qualifies = true;
        } else if (topic == DialogueTopic::ShadowCrypt && trust_ >= 1) {
            clue = DialogueClue::CryptSigil;
            qualifies = true;
        }
        return qualifies && !HasClue(clue);
    }

    constexpr bool UnlockLoreForTopic(DialogueTopic topic) {
        LoreEntry entry = LoreEntry::NationalMallResonance;
        switch (topic) {
        case DialogueTopic::LandmarkHistory:
            entry = LoreEntry::NationalMallResonance;
            break;
        case DialogueTopic::RiftTheory:
        case DialogueTopic::ManaReactor:
            entry = LoreEntry::QuantumCoolingAnomaly;
            break;
        case DialogueTopic::ShadowCrypt:
            entry = LoreEntry::ShadowCryptRumor;
            break;
        case DialogueTopic::CivilianSafety:
        case DialogueTopic::Count:
            return false;
        }
        const std::size_t index = Index(entry);
        if (lore_[index]) return false;
        lore_[index] = true;
        return true;
    }

    constexpr bool UnlockClueFor(DialogueTopic topic, DialogueChoice choice,
        LandmarkDialogueContext context) {
        if (!CanUnlockClueFor(topic, choice, context)) return false;
        DialogueClue clue = DialogueClue::RiftResidue;
        if (topic == DialogueTopic::LandmarkHistory) {
            clue = DialogueClue::CoolingAnomaly;
        } else if (topic == DialogueTopic::ShadowCrypt) {
            clue = DialogueClue::CryptSigil;
        }
        clues_[Index(clue)] = true;
        return true;
    }

    constexpr void PushHistory(const DialogueBeat& beat) {
        history_[historyWrite_] = beat;
        historyWrite_ = (historyWrite_ + 1) % HistoryCapacity;
        if (historyCount_ < HistoryCapacity) ++historyCount_;
    }

    std::array<bool, TopicCount> discussed_{};
    std::array<bool, ClueCount> clues_{};
    std::array<bool, LoreCount> lore_{};
    std::array<DialogueBeat, HistoryCapacity> history_{};
    std::size_t historyWrite_{};
    std::size_t historyCount_{};
    int trust_{};
    DialogueOutcome outcome_{DialogueOutcome::None};
};

namespace Detail {
constexpr bool LandmarkDialogueContract() {
    LandmarkDialogue direct;
    const DialogueBeat directBeat = direct.Choose(
        DialogueTopic::RiftTheory, DialogueChoice::AskDirectly);
    LandmarkDialogue reassured;
    const DialogueBeat reassuredBeat = reassured.Choose(
        DialogueTopic::RiftTheory, DialogueChoice::Reassure);
    if (directBeat.response != DialogueResponse::RiftTheoryBrief
        || reassuredBeat.response != DialogueResponse::RiftTheoryReassured
        || directBeat.response == reassuredBeat.response) return false;

    LandmarkDialogue story;
    const DialogueBeat civilian = story.Choose(
        DialogueTopic::CivilianSafety, DialogueChoice::Reassure);
    const DialogueBeat rift = story.Choose(
        DialogueTopic::RiftTheory, DialogueChoice::ShareEvidence, {2, false});
    const DialogueBeat landmark = story.Choose(
        DialogueTopic::LandmarkHistory, DialogueChoice::ShareEvidence, {3, true});
    const DialogueBeat crypt = story.Choose(
        DialogueTopic::ShadowCrypt, DialogueChoice::ShareEvidence, {3, true});
    if (civilian.response != DialogueResponse::CivilianRouteShared
        || rift.response != DialogueResponse::RiftEvidenceAcknowledged
        || landmark.response != DialogueResponse::MonumentPatternConfirmed
        || crypt.response != DialogueResponse::CryptSigilConfirmed
        || story.Trust() != LandmarkDialogue::MaximumTrust
        || story.UnlockedClueCount() != 3
        || story.UnlockedLoreCount() != 3) return false;
    if (!story.HasClue(DialogueClue::RiftResidue)
        || !story.HasClue(DialogueClue::CoolingAnomaly)
        || !story.HasClue(DialogueClue::CryptSigil)
        || !story.HasLoreEntry(LoreEntry::NationalMallResonance)
        || !story.HasLoreEntry(LoreEntry::QuantumCoolingAnomaly)
        || !story.HasLoreEntry(LoreEntry::ShadowCryptRumor)) return false;
    if (story.CommitOutcome() != DialogueOutcome::AidCivilians
        || story.CommitOutcome() != DialogueOutcome::AidCivilians) return false;
    const DialogueBeat repeated = story.Choose(
        DialogueTopic::CivilianSafety, DialogueChoice::Challenge, {3, true});
    if (!repeated.repeatedTopic || repeated.response != DialogueResponse::AlreadyDiscussed
        || repeated.trustDelta != 0 || story.Trust() != LandmarkDialogue::MaximumTrust)
        return false;

    LandmarkDialogue trustGate;
    const DialogueBeat earlyCrypt = trustGate.Choose(
        DialogueTopic::ShadowCrypt, DialogueChoice::ShareEvidence, {3, true});
    if (earlyCrypt.response != DialogueResponse::ShadowCryptBrief
        || earlyCrypt.clueUnlocked
        || trustGate.HasClue(DialogueClue::CryptSigil)
        || trustGate.Trust() != 1) return false;

    LandmarkDialogue delayedEvidence;
    delayedEvidence.Choose(DialogueTopic::RiftTheory, DialogueChoice::AskDirectly, {0, false});
    const DialogueBeat delayedRift = delayedEvidence.Choose(
        DialogueTopic::RiftTheory, DialogueChoice::ShareEvidence, {2, false});
    if (!delayedRift.repeatedTopic
        || delayedRift.response != DialogueResponse::RiftEvidenceAcknowledged
        || !delayedRift.clueUnlocked
        || delayedRift.trustDelta != 0
        || delayedEvidence.Trust() != 0) return false;

    LandmarkDialogue pursuit;
    pursuit.Choose(DialogueTopic::RiftTheory, DialogueChoice::ShareEvidence, {2, false});
    pursuit.Choose(DialogueTopic::LandmarkHistory, DialogueChoice::ShareEvidence, {3, true});
    if (pursuit.CommitOutcome() != DialogueOutcome::PursueRift) return false;

    LandmarkDialogue history;
    constexpr DialogueTopic topics[] = {
        DialogueTopic::RiftTheory,
        DialogueTopic::CivilianSafety,
        DialogueTopic::LandmarkHistory,
        DialogueTopic::ShadowCrypt,
        DialogueTopic::ManaReactor,
    };
    for (std::size_t index = 0; index < LandmarkDialogue::HistoryCapacity + 2; ++index) {
        history.Choose(topics[index % 5], DialogueChoice::EndConversation);
    }
    if (history.HistoryCount() != LandmarkDialogue::HistoryCapacity
        || history.HistoryFromNewest(0).topic != DialogueTopic::ManaReactor
        || history.HistoryFromNewest(7).topic != DialogueTopic::LandmarkHistory
        || history.HistoryFromNewest(8).response != DialogueResponse::Invalid) return false;

    return true;
}
static_assert(LandmarkDialogueContract(), "Landmark dialogue contract regression");
} // namespace Detail

} // namespace Astral::Scene
