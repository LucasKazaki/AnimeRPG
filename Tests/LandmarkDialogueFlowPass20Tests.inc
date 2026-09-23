void TestLandmarkDialogueFlowPass20() {
    using namespace Astral::Scene;

    {
        LandmarkDialogue dialogue;
        const DialogueBeat lockedCrypt = dialogue.Choose(
            DialogueTopic::ShadowCrypt, DialogueChoice::AskDirectly, {0, false, false});
        const DialogueBeat lockedReactor = dialogue.Choose(
            DialogueTopic::ManaReactor, DialogueChoice::Reassure, {0, false, false});
        Expect(lockedCrypt.response == DialogueResponse::Invalid
                && lockedReactor.response == DialogueResponse::Invalid
                && dialogue.HistoryCount() == 0 && dialogue.Trust() == 0
                && !dialogue.Discussed(DialogueTopic::ShadowCrypt)
                && !dialogue.Discussed(DialogueTopic::ManaReactor)
                && dialogue.UnlockedClueCount() == 0 && dialogue.UnlockedLoreCount() == 0,
            "pass20 story prerequisites reject locked topics without dialogue-state mutation");
        Expect(dialogue.TopicAvailability(DialogueTopic::ShadowCrypt, {0, true, false})
                    == DialogueAvailability::Available
                && dialogue.TopicAvailability(DialogueTopic::ManaReactor, {3, false, false})
                    == DialogueAvailability::Available,
            "pass20 authoritative objective and exploration state unlock advanced story topics");
    }

    {
        LandmarkDialogue dialogue;
        const DialogueBeat preview = dialogue.Choose(
            DialogueTopic::ShadowCrypt, DialogueChoice::Reassure, {0, false, true});
        Expect(preview.response == DialogueResponse::ShadowCryptReassured
                && preview.previewed && !preview.repeatedTopic
                && preview.trustDelta == 0 && !preview.clueUnlocked && !preview.loreUnlocked,
            "pass20 advance screening returns authored preview dialogue without progression effects");
        Expect(dialogue.HistoryCount() == 1 && dialogue.Trust() == 0
                && !dialogue.Discussed(DialogueTopic::ShadowCrypt)
                && !dialogue.HasLoreEntry(LoreEntry::ShadowCryptRumor)
                && dialogue.Outcome() == DialogueOutcome::None,
            "pass20 advance screening does not mark locked story content completed");
        const DialogueBeat stillLocked = dialogue.Choose(
            DialogueTopic::ShadowCrypt, DialogueChoice::AskDirectly, {0, false, false});
        Expect(stillLocked.response == DialogueResponse::Invalid
                && dialogue.HistoryCount() == 1,
            "pass20 preview does not bypass the later ordinary prerequisite gate");
    }

    {
        LandmarkDialogue dialogue;
        DialogueRecommendation recommendation = dialogue.RecommendedTopic({0, false, false});
        Expect(recommendation.available && recommendation.topic == DialogueTopic::RiftTheory
                && recommendation.availability == DialogueAvailability::Available,
            "pass20 recommendation begins with an available unresolved core topic");
        dialogue.Choose(DialogueTopic::RiftTheory, DialogueChoice::AskDirectly);
        recommendation = dialogue.RecommendedTopic({0, false, false});
        Expect(recommendation.available && recommendation.topic == DialogueTopic::CivilianSafety,
            "pass20 recommendation skips already discussed topics deterministically");
        dialogue.Choose(DialogueTopic::CivilianSafety, DialogueChoice::AskDirectly);
        dialogue.Choose(DialogueTopic::LandmarkHistory, DialogueChoice::AskDirectly);
        recommendation = dialogue.RecommendedTopic({0, false, false});
        Expect(!recommendation.available,
            "pass20 recommendation does not highlight story content whose prerequisites are missing");
        recommendation = dialogue.RecommendedTopic({0, false, true});
        Expect(recommendation.available && recommendation.topic == DialogueTopic::ShadowCrypt
                && recommendation.availability == DialogueAvailability::AdvancePreview,
            "pass20 recommendation falls back to explicit advance-screening content only when enabled");
        recommendation = dialogue.RecommendedTopic({0, true, false});
        Expect(recommendation.available && recommendation.topic == DialogueTopic::ShadowCrypt
                && recommendation.availability == DialogueAvailability::Available,
            "pass20 normal unlocked story content outranks preview guidance");
    }

    {
        LandmarkDialogue dialogue;
        const DialogueBeat civilian = dialogue.Choose(
            DialogueTopic::CivilianSafety, DialogueChoice::Reassure);
        dialogue.Choose(
            DialogueTopic::RiftTheory, DialogueChoice::ShareEvidence, {2, false, false});
        const DialogueOutcome committed = dialogue.CommitOutcome();
        const DialogueBeat preview = dialogue.Choose(
            DialogueTopic::ShadowCrypt, DialogueChoice::Challenge, {0, false, true});
        const DialogueSynopsis synopsis = dialogue.Synopsis();
        Expect(committed == DialogueOutcome::AidCivilians
                && synopsis.topicsDiscussed == 2 && synopsis.cluesUnlocked == 1
                && synopsis.loreUnlocked == 1 && synopsis.retainedPreviewBeats == 1
                && synopsis.trust == 2 && synopsis.outcome == DialogueOutcome::AidCivilians
                && synopsis.latestResponse == preview.response,
            "pass20 synopsis reflects authoritative bounded story state and newest retained response");
        Expect(!LandmarkDialogue::QuickAdvanceSafe(civilian)
                && !LandmarkDialogue::QuickAdvanceSafe(preview),
            "pass20 quick advance does not skip first-time state-changing or substantive preview beats");
    }

    {
        LandmarkDialogue dialogue;
        Expect(dialogue.ShouldPromptForChoice(DialogueTopic::RiftTheory, {0, false, false}),
            "pass20 first-time differentiated responses retain a meaningful player choice prompt");
        const DialogueBeat first = dialogue.Choose(
            DialogueTopic::RiftTheory, DialogueChoice::AskDirectly, {0, false, false});
        Expect(!LandmarkDialogue::QuickAdvanceSafe(first),
            "pass20 first-time authored response is not marked safe for automatic fast-forward");
        Expect(!dialogue.ShouldPromptForChoice(DialogueTopic::RiftTheory, {0, false, false}),
            "pass20 repeated exhausted topic suppresses a fake prompt when all choices collapse identically");
        const DialogueBeat redundant = dialogue.Choose(
            DialogueTopic::RiftTheory, DialogueChoice::Challenge, {0, false, false});
        Expect(redundant.response == DialogueResponse::AlreadyDiscussed
                && redundant.autoAdvanceRecommended
                && LandmarkDialogue::QuickAdvanceSafe(redundant),
            "pass20 redundant repeated dialogue is explicitly safe to fast-forward");
        Expect(dialogue.ShouldPromptForChoice(DialogueTopic::RiftTheory, {2, false, false}),
            "pass20 a newly eligible evidence branch restores a meaningful choice prompt");
        const DialogueBeat evidence = dialogue.Choose(
            DialogueTopic::RiftTheory, DialogueChoice::ShareEvidence, {2, false, false});
        Expect(evidence.response == DialogueResponse::RiftEvidenceAcknowledged
                && evidence.clueUnlocked && !evidence.autoAdvanceRecommended
                && !LandmarkDialogue::QuickAdvanceSafe(evidence),
            "pass20 meaningful repeated evidence cannot be auto-advanced as a fake choice");
    }

    {
        LandmarkInteraction interaction;
        const DialogueRecommendation initial = interaction.RecommendedDialogueTopic();
        Expect(initial.available && initial.topic == DialogueTopic::RiftTheory
                && interaction.ShouldPromptForDialogueChoice(DialogueTopic::RiftTheory),
            "pass20 live LandmarkInteraction owner exposes recommendation and prompt policy");
        const DialogueBeat preview = interaction.TryDialogueChoice(
            DialogueTopic::ShadowCrypt, DialogueChoice::AskDirectly, true);
        const DialogueSynopsis summary = interaction.DialogueSummary();
        Expect(preview.previewed && summary.retainedPreviewBeats == 1
                && !interaction.Dialogue().Discussed(DialogueTopic::ShadowCrypt)
                && !LandmarkInteraction::DialogueQuickAdvanceSafe(preview),
            "pass20 live LandmarkInteraction owner exposes screening and synopsis without false completion");
    }

    {
        LandmarkDialogue dialogue;
        const DialogueTopic invalidTopic = static_cast<DialogueTopic>(255);
        const DialogueChoice invalidChoice = static_cast<DialogueChoice>(255);
        const DialogueRecommendation normalRecommendation =
            dialogue.RecommendedTopic({0, false, false});
        const DialogueBeat invalidTopicBeat = dialogue.Choose(
            invalidTopic, DialogueChoice::AskDirectly, {0, false, false});
        const DialogueBeat invalidChoiceBeat = dialogue.Choose(
            DialogueTopic::RiftTheory, invalidChoice, {0, false, false});
        Expect(dialogue.TopicAvailability(invalidTopic) == DialogueAvailability::Locked
                && normalRecommendation.available
                && invalidTopicBeat.response == DialogueResponse::Invalid
                && invalidChoiceBeat.response == DialogueResponse::Invalid
                && dialogue.HistoryCount() == 0,
            "pass20 invalid topic and choice values fail closed without history mutation");
    }
}
