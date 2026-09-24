void TestExplorationFieldGuidePass29() {
    using namespace Astral::Scene;

    ExplorationFieldGuide guide;
    const auto initial = guide.Briefing();
    Expect(initial.trackedOperation == FieldOperation::MallSurvey
            && initial.operationProgress.current == 0 && initial.operationProgress.required == 3
            && initial.hint.kind == FieldHintKind::VisitLandmark
            && initial.hint.hasLandmark
            && initial.hint.landmark == LandmarkKind::LincolnMemorial
            && initial.target.available && initial.target.source == FieldTargetSource::Recommended,
        "pass29 initial briefing exposes the first survey action without mutation");

    Expect(guide.AddRouteTarget(LandmarkKind::WashingtonMonument, FieldPinCategory::Note),
        "pass29 route adds a note stop");
    Expect(guide.AddRouteTarget(LandmarkKind::ReflectingPool, FieldPinCategory::Resource),
        "pass29 route adds a resource stop");
    Expect(!guide.AddRouteTarget(LandmarkKind::WashingtonMonument, FieldPinCategory::Objective)
            && !guide.AddRouteTarget(static_cast<LandmarkKind>(999), FieldPinCategory::Objective)
            && !guide.AddRouteTarget(LandmarkKind::LincolnMemorial, FieldPinCategory::Count),
        "pass29 route rejects duplicate, invalid landmark and invalid category atomically");
    Expect(guide.RouteTargetCount() == 2
            && guide.RouteTarget(0).landmark == LandmarkKind::WashingtonMonument
            && guide.RouteTarget(0).category == FieldPinCategory::Note
            && guide.RouteTarget(1).landmark == LandmarkKind::ReflectingPool
            && guide.CurrentTarget().source == FieldTargetSource::RoutePin
            && guide.CurrentTarget().landmark == LandmarkKind::WashingtonMonument,
        "pass29 route preserves bounded insertion order ahead of recommendations");

    Expect(guide.SetRouteFilter(FieldPinFilter::Resource)
            && guide.CurrentTarget().source == FieldTargetSource::RoutePin
            && guide.CurrentTarget().landmark == LandmarkKind::ReflectingPool,
        "pass29 route visibility filter selects only the matching category");
    Expect(!guide.SetRouteFilter(FieldPinFilter::Count)
            && !guide.SetRouteFilter(static_cast<FieldPinFilter>(999))
            && guide.RouteFilter() == FieldPinFilter::Resource
            && guide.RouteTargetCount() == 2,
        "pass29 invalid route filters fail closed without deleting hidden stops");

    Expect(guide.PinTarget(LandmarkKind::LincolnMemorial)
            && guide.CurrentTarget().source == FieldTargetSource::PlayerPin
            && guide.CurrentTarget().landmark == LandmarkKind::LincolnMemorial,
        "pass29 legacy single pin retains precedence over the route");
    guide.ClearPinnedTarget();
    Expect(guide.CurrentTarget().source == FieldTargetSource::RoutePin
            && guide.CurrentTarget().landmark == LandmarkKind::ReflectingPool,
        "pass29 clearing legacy pin restores filtered route selection");

    Expect(guide.RecordLandmark(LandmarkKind::ReflectingPool)
            && guide.RouteTargetCount() == 1,
        "pass29 visiting a route stop prunes it exactly once");
    Expect(guide.CurrentTarget().source == FieldTargetSource::Recommended
            && guide.CurrentTarget().landmark == LandmarkKind::LincolnMemorial,
        "pass29 hidden remaining route falls back to the operation recommendation");

    Expect(guide.UnreadJournalCount() == 1
            && guide.MarkJournalRead(FieldJournalEntry::ReflectingPoolFieldNote)
            && guide.UnreadJournalCount() == 0,
        "pass29 newly unlocked journal notes begin unread and can be acknowledged");
    Expect(!guide.MarkJournalRead(FieldJournalEntry::ReflectingPoolFieldNote)
            && !guide.MarkJournalRead(FieldJournalEntry::ShadowCryptLead)
            && !guide.MarkJournalRead(static_cast<FieldJournalEntry>(999)),
        "pass29 read acknowledgement is idempotent and rejects locked or invalid entries");

    Expect(guide.TrackOperation(FieldOperation::RiftInvestigation)
            && guide.CurrentHint().kind == FieldHintKind::SeekRiftResidue,
        "pass29 Rift Investigation points at its first missing evidence");
    guide.SyncNarrativeEvidence({true, false, false, false});
    Expect(guide.CurrentHint().kind == FieldHintKind::InvestigateCoolingAnomaly
            && guide.UnreadJournalCount() == 1,
        "pass29 Rift hint advances after residue and raises one evidence notice");
    Expect(guide.MarkJournalRead(FieldJournalEntry::RiftEvidence)
            && guide.UnreadJournalCount() == 0,
        "pass29 Rift evidence notice can be acknowledged");
    guide.SyncNarrativeEvidence({true, false, false, false});
    Expect(guide.UnreadJournalCount() == 0,
        "pass29 duplicate evidence never resurrects an acknowledged journal notice");
    guide.SyncNarrativeEvidence({false, true, false, false});
    Expect(guide.CurrentHint().kind == FieldHintKind::OperationComplete
            && guide.OperationProgress(FieldOperation::RiftInvestigation).Complete(),
        "pass29 Rift hint reports completion after both evidence types");

    Expect(guide.TrackNextIncompleteOperation()
            && guide.TrackedOperation() == FieldOperation::ShadowCryptLead
            && guide.CurrentHint().kind == FieldHintKind::FindCryptSigil,
        "pass29 next-incomplete tracking advances cyclically to Shadow Crypt");
    guide.SyncNarrativeEvidence({false, false, true, false});
    Expect(guide.CurrentHint().kind == FieldHintKind::AskAboutShadowCrypt,
        "pass29 Shadow Crypt hint advances from sigil to lore");

    const auto briefing = guide.Briefing();
    Expect(briefing.trackedOperation == FieldOperation::ShadowCryptLead
            && briefing.operationProgress.current == 1 && briefing.operationProgress.required == 2
            && briefing.hint.kind == FieldHintKind::AskAboutShadowCrypt
            && briefing.routeStops == 1
            && briefing.journalUnlocked == guide.JournalEntryUnlockedCount()
            && briefing.journalUnread == guide.UnreadJournalCount()
            && briefing.completedOperations == guide.CompletedOperationCount(),
        "pass29 briefing consolidates exact authoritative field-guide state");
    Expect(guide.TrackedOperation() == FieldOperation::ShadowCryptLead
            && guide.RouteTargetCount() == 1,
        "pass29 briefing is non-mutating");

    Expect(guide.SetRouteFilter(FieldPinFilter::All)
            && guide.CurrentTarget().source == FieldTargetSource::RoutePin
            && guide.CurrentTarget().landmark == LandmarkKind::WashingtonMonument,
        "pass29 restoring All makes the hidden route stop visible again");
    Expect(guide.ClearRouteTargets() == 1 && guide.RouteTargetCount() == 0
            && guide.ClearRouteTargets() == 0,
        "pass29 route batch clear reports exact removals and is idempotent");

    guide.SyncNarrativeEvidence({false, false, false, true});
    Expect(guide.CurrentHint().kind == FieldHintKind::OperationComplete
            && guide.HasJournalEntry(FieldJournalEntry::ShadowCryptLead),
        "pass29 Shadow Crypt hint completes after matching lore");
    const std::size_t unreadBefore = guide.UnreadJournalCount();
    Expect(guide.MarkAllJournalRead() == unreadBefore
            && guide.UnreadJournalCount() == 0
            && guide.MarkAllJournalRead() == 0,
        "pass29 mark-all acknowledges exactly the unread journal entries once");

    ExplorationFieldGuide capacity;
    Expect(capacity.AddRouteTarget(LandmarkKind::LincolnMemorial, FieldPinCategory::Objective)
            && capacity.AddRouteTarget(LandmarkKind::ReflectingPool, FieldPinCategory::Resource)
            && capacity.AddRouteTarget(LandmarkKind::WashingtonMonument, FieldPinCategory::Note)
            && capacity.RouteTargetCount() == ExplorationFieldGuide::RouteCapacity,
        "pass29 route reaches its fixed three-stop capacity without allocation");
    Expect(!capacity.AddRouteTarget(LandmarkKind::LincolnMemorial, FieldPinCategory::Note)
            && capacity.RouteTargetCount() == ExplorationFieldGuide::RouteCapacity,
        "pass29 full route cannot grow or duplicate entries");
    Expect(capacity.RemoveRouteTarget(LandmarkKind::ReflectingPool)
            && !capacity.RemoveRouteTarget(LandmarkKind::ReflectingPool)
            && capacity.RouteTargetCount() == 2
            && capacity.RouteTarget(0).landmark == LandmarkKind::LincolnMemorial
            && capacity.RouteTarget(1).landmark == LandmarkKind::WashingtonMonument,
        "pass29 individual route removal compacts order and is idempotent");

    ExplorationFieldGuide wraparound;
    wraparound.SyncNarrativeEvidence({true, true, true, true});
    Expect(wraparound.TrackOperation(FieldOperation::ShadowCryptLead)
            && wraparound.OperationProgress(FieldOperation::RiftInvestigation).Complete()
            && wraparound.OperationProgress(FieldOperation::ShadowCryptLead).Complete()
            && !wraparound.OperationProgress(FieldOperation::MallSurvey).Complete(),
        "pass29 wraparound setup leaves only Mall Survey incomplete while tracking the final operation");
    Expect(wraparound.TrackNextIncompleteOperation()
            && wraparound.TrackedOperation() == FieldOperation::MallSurvey,
        "pass29 next-incomplete tracking wraps from Shadow Crypt Lead back to Mall Survey");

    ExplorationFieldGuide complete;
    Expect(complete.RecordLandmark(LandmarkKind::LincolnMemorial)
            && complete.RecordLandmark(LandmarkKind::ReflectingPool)
            && complete.RecordLandmark(LandmarkKind::WashingtonMonument),
        "pass29 complete-state setup records all Mall sites");
    complete.SyncNarrativeEvidence({true, true, true, true});
    Expect(complete.CompletedOperationCount() == ExplorationFieldGuide::OperationCount,
        "pass29 complete-state setup satisfies all three operations");
    Expect(complete.TrackOperation(FieldOperation::RiftInvestigation),
        "pass29 complete-state setup can select a finished operation");
    const FieldOperation before = complete.TrackedOperation();
    Expect(!complete.TrackNextIncompleteOperation()
            && complete.TrackedOperation() == before,
        "pass29 next-incomplete tracking is a no-op once every operation is complete");

    // Independent-review repair: exercise the new mutations through the real
    // LandmarkInteraction-owned production guide instead of only standalone
    // ExplorationFieldGuide fixtures.
    LandmarkInteraction owner;
    Expect(owner.AddFieldRouteTarget(LandmarkKind::WashingtonMonument, FieldPinCategory::Note)
            && owner.AddFieldRouteTarget(LandmarkKind::ReflectingPool, FieldPinCategory::Resource)
            && owner.SetFieldRouteFilter(FieldPinFilter::Resource),
        "pass29 production owner can create and filter the bounded field route");
    const FieldGuideBriefing ownerRouteBriefing = owner.FieldBriefing();
    Expect(ownerRouteBriefing.routeStops == 2
            && ownerRouteBriefing.target.available
            && ownerRouteBriefing.target.source == FieldTargetSource::RoutePin
            && ownerRouteBriefing.target.landmark == LandmarkKind::ReflectingPool,
        "pass29 production briefing reflects owner-routed route state");
    Expect(!owner.SetFieldRouteFilter(FieldPinFilter::Count)
            && owner.FieldGuide().RouteFilter() == FieldPinFilter::Resource,
        "pass29 production owner rejects an invalid route filter without mutation");
    Expect(owner.RemoveFieldRouteTarget(LandmarkKind::WashingtonMonument)
            && !owner.RemoveFieldRouteTarget(LandmarkKind::WashingtonMonument)
            && owner.FieldBriefing().routeStops == 1,
        "pass29 production owner removal is exact and idempotent");
    Expect(owner.ClearFieldRouteTargets() == 1
            && owner.FieldBriefing().routeStops == 0
            && owner.ClearFieldRouteTargets() == 0,
        "pass29 production owner clears a nonempty route exactly once");
    Expect(owner.AddFieldRouteTarget(LandmarkKind::ReflectingPool, FieldPinCategory::Resource),
        "pass29 production route can be reseeded for live visit-pruning coverage");
    Expect(owner.TrackFieldOperation(FieldOperation::RiftInvestigation)
            && owner.TrackNextIncompleteFieldOperation()
            && owner.FieldBriefing().trackedOperation == FieldOperation::ShadowCryptLead,
        "pass29 production owner can select the next incomplete operation");

    WorldBlockout world;
    ShadowbladeActions actions;
    Expect(owner.TryInteract({-8.0f, 18.0f, 0.0f}, world, actions).result
            == LandmarkInteractionResult::Discovered
            && owner.FieldBriefing().journalUnread == 1
            && owner.MarkFieldJournalRead(FieldJournalEntry::LincolnFieldNote)
            && owner.FieldBriefing().journalUnread == 0,
        "pass29 live landmark discovery creates a journal notice that owner-level acknowledgement clears");
    Expect(!owner.MarkFieldJournalRead(FieldJournalEntry::LincolnFieldNote)
            && !owner.MarkFieldJournalRead(FieldJournalEntry::ShadowCryptLead),
        "pass29 production journal acknowledgement remains idempotent and locked-entry safe");
    Expect(owner.TryInteract({4.0f, 39.0f, 0.0f}, world, actions).result
            == LandmarkInteractionResult::Discovered
            && owner.MarkAllFieldJournalRead() == 1
            && owner.FieldBriefing().journalUnread == 0
            && owner.MarkAllFieldJournalRead() == 0,
        "pass29 production owner read-all acknowledges exactly the newly discovered note once");
    Expect(owner.ClearFieldRouteTargets() == 0
            && owner.FieldBriefing().routeStops == 0,
        "pass29 live route auto-pruning leaves no route state for a later clear");
}
