void TestLandmarkDialogueContinuityPass30() {
    using namespace Astral::Scene;

    LandmarkInteraction owner;
    const DialogueBeat first = owner.TryDialogueChoice(
        DialogueTopic::RiftTheory, DialogueChoice::AskDirectly);
    Expect(first.response == DialogueResponse::RiftTheoryBrief
            && owner.DialogueRelationship() == DialogueRelationshipTier::Acquainted,
        "pass30 live owner records an ordinary dialogue beat and relationship tier");

    const DialogueCheckpoint checkpoint = owner.DialogueCheckpointFor(DialogueTopic::RiftTheory);
    Expect(checkpoint.available && checkpoint.sequence == 1
            && checkpoint.beat.response == DialogueResponse::RiftTheoryBrief,
        "pass30 live owner exposes the latest substantive checkpoint for a topic");
    const DialogueSynopsis beforeRevisit = owner.DialogueSummary();
    const DialogueRevisit revisit = owner.RevisitDialogueCheckpoint(DialogueTopic::RiftTheory);
    const DialogueSynopsis afterRevisit = owner.DialogueSummary();
    Expect(revisit.available && !revisit.mutatesStory && revisit.originalSequence == 1
            && revisit.beat.response == DialogueResponse::RiftTheoryBrief
            && beforeRevisit.topicsDiscussed == afterRevisit.topicsDiscussed
            && beforeRevisit.trust == afterRevisit.trust
            && owner.Dialogue().HistoryCount() == 1,
        "pass30 checkpoint revisit is read-only and cannot duplicate story progression");

    Expect(owner.CaptureDialogueInterruption(),
        "pass30 live owner can retain the latest accepted beat when dialogue is interrupted");
    const InterruptedDialogueBeat interrupted = owner.InterruptedDialogue();
    Expect(interrupted.pending && interrupted.sequence == 1
            && interrupted.beat.response == DialogueResponse::RiftTheoryBrief
            && !owner.CaptureDialogueInterruption(),
        "pass30 interruption capture is idempotent and preserves the original beat");
    Expect(owner.AcknowledgeInterruptedDialogue()
            && !owner.AcknowledgeInterruptedDialogue()
            && !owner.InterruptedDialogue().pending,
        "pass30 recovered dialogue is acknowledged exactly once");

    const DialogueBeat reassure = owner.TryDialogueChoice(
        DialogueTopic::CivilianSafety, DialogueChoice::Reassure);
    const DialogueBeat history = owner.TryDialogueChoice(
        DialogueTopic::LandmarkHistory, DialogueChoice::Reassure);
    Expect(reassure.response == DialogueResponse::CivilianRouteShared
            && history.response == DialogueResponse::LandmarkHistoryReassured
            && owner.DialogueRelationship() == DialogueRelationshipTier::Confidant
            && owner.DialogueStoryEpisodeUnlocked(DialogueStoryEpisode::MallWitness),
        "pass30 existing trust and lore state unlock an original confidant story episode");

    const DialogueOutcome outcome = owner.CommitDialogueOutcome();
    const DialogueEndingArchive endings = owner.DialogueEndings();
    Expect(outcome == DialogueOutcome::AidCivilians
            && endings.aidCivilians && !endings.pursueRift && endings.unlockedCount == 1,
        "pass30 committed ending memory follows the authoritative dialogue outcome");
    Expect(owner.CommitDialogueOutcome() == DialogueOutcome::AidCivilians
            && owner.DialogueEndings().unlockedCount == 1,
        "pass30 repeated outcome commits cannot duplicate ending memory");

    LandmarkInteraction previewOwner;
    const DialogueBeat preview = previewOwner.TryDialogueChoice(
        DialogueTopic::ShadowCrypt, DialogueChoice::AskDirectly, true);
    Expect(preview.previewed && preview.response == DialogueResponse::ShadowCryptBrief
            && !previewOwner.DialogueCheckpointFor(DialogueTopic::ShadowCrypt).available,
        "pass30 advance-screening previews do not become authoritative checkpoints");

    LandmarkInteraction repeatedOwner;
    repeatedOwner.TryDialogueChoice(DialogueTopic::RiftTheory, DialogueChoice::AskDirectly);
    const DialogueBeat repeated = repeatedOwner.TryDialogueChoice(
        DialogueTopic::RiftTheory, DialogueChoice::Challenge);
    Expect(repeated.repeatedTopic && repeated.response == DialogueResponse::AlreadyDiscussed
            && repeatedOwner.DialogueCheckpointFor(DialogueTopic::RiftTheory).sequence == 1,
        "pass30 exhausted repeated dialogue cannot overwrite its substantive checkpoint");

    LandmarkDialogue directDialogue;
    LandmarkDialogueContinuity directContinuity;
    const DialogueBeat challenged = directDialogue.Choose(
        DialogueTopic::RiftTheory, DialogueChoice::Challenge);
    Expect(directContinuity.RecordBeat(directDialogue, challenged)
            && directContinuity.RelationshipTier(directDialogue) == DialogueRelationshipTier::Wary,
        "pass30 negative trust maps deterministically to the wary relationship tier");
    Expect(!directContinuity.Checkpoint(static_cast<DialogueTopic>(999)).available
            && !directContinuity.Revisit(static_cast<DialogueTopic>(999)).available,
        "pass30 invalid checkpoint queries fail closed");

    LandmarkDialogue story;
    LandmarkDialogueContinuity continuity;
    const DialogueBeat civil = story.Choose(
        DialogueTopic::CivilianSafety, DialogueChoice::Reassure);
    continuity.RecordBeat(story, civil);
    const DialogueBeat rift = story.Choose(
        DialogueTopic::RiftTheory, DialogueChoice::ShareEvidence, {2, false});
    continuity.RecordBeat(story, rift);
    const DialogueBeat landmark = story.Choose(
        DialogueTopic::LandmarkHistory, DialogueChoice::ShareEvidence, {3, true});
    continuity.RecordBeat(story, landmark);
    const DialogueBeat crypt = story.Choose(
        DialogueTopic::ShadowCrypt, DialogueChoice::ShareEvidence, {3, true});
    continuity.RecordBeat(story, crypt);
    Expect(continuity.StoryEpisodeUnlocked(DialogueStoryEpisode::RiftConfidant)
            && continuity.StoryEpisodeUnlocked(DialogueStoryEpisode::CryptPartner),
        "pass30 clue, lore and trust evidence unlock deeper story episodes without a parallel quest state");

    Expect(!continuity.RecordBeat(story, {})
            && !continuity.StoryEpisodeUnlocked(static_cast<DialogueStoryEpisode>(999)),
        "pass30 invalid beats and episode ids fail closed");
}
