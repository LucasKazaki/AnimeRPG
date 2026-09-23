void TestExplorationFieldGuidePass14() {
    using namespace Astral::Scene;

    ExplorationFieldGuide guide;
    const FieldOperationProgress initialSurvey = guide.OperationProgress(FieldOperation::MallSurvey);
    Expect(initialSurvey.current == 0 && initialSurvey.required == 3 && !initialSurvey.Complete(),
        "field-operation board begins with a bounded three-site Mall survey");
    Expect(guide.TrackOperation(FieldOperation::RiftInvestigation)
            && guide.TrackedOperation() == FieldOperation::RiftInvestigation
            && !guide.TrackOperation(static_cast<FieldOperation>(999)),
        "tracked operations can change explicitly while invalid operation ids fail closed");

    Expect(guide.RecordLandmark(LandmarkKind::WashingtonMonument)
            && !guide.RecordLandmark(LandmarkKind::WashingtonMonument)
            && guide.DiscoveryCount() == 1
            && guide.DiscoveryFromOldest(0) == LandmarkKind::WashingtonMonument,
        "landmark survey chronology records each site once even when exploration is out of order");
    Expect(guide.HasJournalEntry(FieldJournalEntry::MonumentFieldNote)
            && guide.JournalEntryUnlockedCount() == 1,
        "first-time site surveys unlock their bounded field-journal entry");

    FieldNarrativeSnapshot evidence{};
    evidence.hasRiftResidue = true;
    evidence.hasCoolingAnomaly = true;
    guide.SyncNarrativeEvidence(evidence);
    const FieldOperationProgress rift = guide.OperationProgress(FieldOperation::RiftInvestigation);
    Expect(rift.current == 2 && rift.required == 2 && rift.Complete()
            && guide.HasJournalEntry(FieldJournalEntry::RiftEvidence),
        "two authoritative dialogue clues complete the rift investigation and unlock its evidence note");

    FieldNarrativeSnapshot staleEvidence{};
    staleEvidence.hasRiftResidue = true;
    guide.SyncNarrativeEvidence(staleEvidence);
    Expect(guide.OperationProgress(FieldOperation::RiftInvestigation).current == 2,
        "a stale lower evidence snapshot cannot erase already observed clue progress");

    FieldNarrativeSnapshot cryptEvidence{};
    cryptEvidence.hasCryptSigil = true;
    guide.SyncNarrativeEvidence(cryptEvidence);
    Expect(guide.OperationProgress(FieldOperation::ShadowCryptLead).current == 1
            && !guide.OperationProgress(FieldOperation::ShadowCryptLead).Complete(),
        "Shadow Crypt field operation remains incomplete with only the sigil clue");
    cryptEvidence.hasShadowCryptLore = true;
    guide.SyncNarrativeEvidence(cryptEvidence);
    Expect(guide.OperationProgress(FieldOperation::ShadowCryptLead).Complete()
            && guide.HasJournalEntry(FieldJournalEntry::ShadowCryptLead),
        "the crypt clue plus matching lore complete the Shadow Crypt lead without a duplicate reward path");

    ExplorationFieldGuide route;
    Expect(route.TrackOperation(FieldOperation::MallSurvey),
        "survey route can be selected as the tracked field operation");
    const FieldTarget recommended = route.CurrentTarget();
    Expect(recommended.available && recommended.source == FieldTargetSource::Recommended
            && recommended.landmark == LandmarkKind::LincolnMemorial,
        "survey guidance recommends the first unvisited National Mall site");
    Expect(route.RecordLandmark(LandmarkKind::LincolnMemorial)
            && route.CurrentTarget().landmark == LandmarkKind::ReflectingPool,
        "survey guidance advances past already observed landmarks");

    Expect(route.PinTarget(LandmarkKind::WashingtonMonument),
        "player can temporarily override route guidance with a custom unvisited field target");
    const FieldTarget pinned = route.CurrentTarget();
    Expect(pinned.available && pinned.source == FieldTargetSource::PlayerPin
            && pinned.landmark == LandmarkKind::WashingtonMonument,
        "manual field pin takes precedence over the automatic survey recommendation");
    Expect(route.RecordLandmark(LandmarkKind::WashingtonMonument)
            && !route.HasPinnedTarget()
            && route.CurrentTarget().source == FieldTargetSource::Recommended
            && route.CurrentTarget().landmark == LandmarkKind::ReflectingPool,
        "discovering the pinned site automatically reconciles the pin instead of leaving stale cleanup work");
    Expect(!route.PinTarget(LandmarkKind::LincolnMemorial),
        "already completed landmarks cannot be repinned as unfinished targets");

    Expect(route.RecordLandmark(LandmarkKind::ReflectingPool)
            && route.OperationProgress(FieldOperation::MallSurvey).Complete()
            && !route.CurrentTarget().available,
        "completing all three unique site surveys closes the bounded route with no phantom target");
    Expect(route.CompletedOperationCount() == 1,
        "completed operation count reflects only operations actually satisfied in this guide state");
}
