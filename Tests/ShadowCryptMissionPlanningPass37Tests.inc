namespace {

Astral::Scene::ExplorationFieldGuide UnlockedShadowCryptGuidePass37() {
    using namespace Astral::Scene;
    ExplorationFieldGuide guide;
    guide.SyncNarrativeEvidence({false, false, true, true});
    return guide;
}

void AdvanceShadowCryptMissionPass37ToRiftNave(Astral::Scene::ShadowCryptMission& mission) {
    using namespace Astral::Scene;
    Expect(mission.RecordObjectiveStep() && mission.RecordObjectiveStep()
            && mission.Briefing().room == ShadowCryptRoom::ArchiveGallery,
        "pass37 mission clears the two Entry Seal anchors");
    Expect(mission.RecordObjectiveStep() && mission.RecordObjectiveStep()
            && mission.RecordObjectiveStep()
            && mission.Briefing().room == ShadowCryptRoom::RiftNave,
        "pass37 mission clears the three Archive Gallery records");
}

void FinishShadowCryptMissionPass37FromRiftNave(Astral::Scene::ShadowCryptMission& mission) {
    using namespace Astral::Scene;
    Expect(mission.RecordObjectiveStep() && mission.RecordObjectiveStep()
            && mission.Briefing().room == ShadowCryptRoom::WardenSanctum,
        "pass37 mission stabilizes the two Rift Nave nodes");
    Expect(mission.RecordObjectiveStep() && mission.Briefing().complete,
        "pass37 mission completes only after the Rift Warden objective");
}

void CompleteShadowCryptMissionPass37(Astral::Scene::ShadowCryptMission& mission,
    int damageTaken, bool reportDefeat, bool clearCache) {
    using namespace Astral::Scene;
    AdvanceShadowCryptMissionPass37ToRiftNave(mission);
    if (clearCache) {
        Expect(mission.DiscoverCoolingCache() && mission.ClearCoolingCache(),
            "pass37 optional cache remains available when requested");
    }
    if (damageTaken > 0) {
        Expect(mission.RecordDamageTaken(damageTaken) == damageTaken,
            "pass37 completion fixture records requested bounded damage");
    }
    if (reportDefeat) {
        Expect(mission.ReportDefeat(),
            "pass37 completion fixture records the requested local defeat");
    }
    FinishShadowCryptMissionPass37FromRiftNave(mission);
}

void TestShadowCryptMissionPlanningPass37() {
    using namespace Astral::Scene;

    // GAME-183 + GAME-184: pre-entry gating and the next-step guide come from
    // authoritative field evidence without changing the caller's tracked operation.
    ExplorationFieldGuide lockedGuide;
    Expect(lockedGuide.TrackOperation(FieldOperation::MallSurvey),
        "pass37 setup explicitly tracks the unrelated Mall Survey operation");
    ShadowCryptMission lockedMission;
    const ShadowCryptEntryReport lockedEntry = lockedMission.EntryReport(lockedGuide);
    Expect(!lockedEntry.readyToBegin
            && lockedEntry.blocker == ShadowCryptEntryBlocker::ShadowCryptLeadIncomplete
            && lockedEntry.leadProgress.current == 0 && lockedEntry.leadProgress.required == 2,
        "pass37 entry report exposes the exact incomplete Shadow Crypt lead prerequisite");
    Expect(lockedMission.EntryGuidance(lockedGuide) == ShadowCryptEntryGuidance::FindCryptSigil
            && lockedGuide.TrackedOperation() == FieldOperation::MallSurvey,
        "pass37 prerequisite guide points at the missing sigil without mutating tracked field guidance");

    lockedGuide.SyncNarrativeEvidence({false, false, true, false});
    Expect(lockedMission.EntryGuidance(lockedGuide)
            == ShadowCryptEntryGuidance::AskAboutShadowCrypt,
        "pass37 prerequisite guide advances to the missing lore witness after the sigil is known");

    ExplorationFieldGuide guide = UnlockedShadowCryptGuidePass37();
    const ShadowCryptEntryReport readyEntry = lockedMission.EntryReport(guide);
    Expect(readyEntry.readyToBegin && readyEntry.blocker == ShadowCryptEntryBlocker::None
            && lockedMission.EntryGuidance(guide) == ShadowCryptEntryGuidance::EnterShadowCrypt,
        "pass37 complete lead exposes a deterministic ready-to-enter state");

    // QOL-038: encounter intel is available before commitment and does not create
    // a run or invent enemy statistics.
    const ShadowCryptMissionPreview preview = lockedMission.Preview();
    Expect(preview.available && preview.mainRoomCount == 4
            && preview.objectiveRequirements[0] == 2
            && preview.objectiveRequirements[1] == 3
            && preview.objectiveRequirements[2] == 2
            && preview.objectiveRequirements[3] == 1
            && preview.optionalCoolingCache && preview.optionalCacheUnlockAfterRooms == 2
            && preview.riftWardenFinalEncounter
            && preview.finalRoom == ShadowCryptRoom::WardenSanctum
            && preview.finalObjective == ShadowCryptObjective::DefeatRiftWarden
            && !lockedMission.Briefing().available,
        "pass37 read-only preview exposes the authored room plan and Warden finale before starting");

    ShadowCryptMission mission;
    Expect(mission.Begin(guide), "pass37 unlocked mission begins exactly once");
    Expect(mission.EntryReport(guide).blocker == ShadowCryptEntryBlocker::ActiveRun
            && mission.EntryGuidance(guide) == ShadowCryptEntryGuidance::ContinueRun,
        "pass37 active mission reports continuation instead of suggesting a duplicate entry");

    AdvanceShadowCryptMissionPass37ToRiftNave(mission);
    Expect(mission.Briefing().guidance == ShadowCryptGuidanceAction::DiscoverCoolingCache,
        "pass37 Standard guidance preserves the existing optional Cooling Cache prompt");

    // GAME-182: Focused guidance prioritizes the main room objective but the
    // side objective stays manually available and no expedition state is deleted.
    Expect(mission.SetFocusMode(ShadowCryptMissionFocusMode::Focused)
            && mission.Briefing().focusMode == ShadowCryptMissionFocusMode::Focused
            && mission.Briefing().guidance == ShadowCryptGuidanceAction::StabilizeRiftNode,
        "pass37 Focused mode suppresses only optional-cache guidance");
    Expect(mission.DiscoverCoolingCache(),
        "pass37 Focused mode does not disable the optional Cooling Cache itself");
    Expect(mission.SetFocusMode(ShadowCryptMissionFocusMode::Standard)
            && mission.Briefing().guidance == ShadowCryptGuidanceAction::ClearCoolingCache
            && mission.ClearCoolingCache(),
        "pass37 returning to Standard surfaces the still-authoritative optional cache state");
    Expect(!mission.SetFocusMode(ShadowCryptMissionFocusMode::Standard)
            && !mission.SetFocusMode(static_cast<ShadowCryptMissionFocusMode>(255))
            && mission.FocusMode() == ShadowCryptMissionFocusMode::Standard,
        "pass37 identical or invalid focus requests are non-mutating");

    Expect(mission.RecordDamageTaken(10) == 10,
        "pass37 first clear records bounded damage for result comparison");
    FinishShadowCryptMissionPass37FromRiftNave(mission);

    // GAME-185 + GAME-186: accepted clears expose latest-vs-best comparison and
    // a bounded newest-first archive while preserving the monotonic best record.
    ShadowCryptMissionResult result = mission.LatestResult();
    Expect(result.available && result.newPersonalBest && result.scoreBehindBest == 0
            && result.latest.valid && result.latest.grade == ShadowCryptGrade::Gold
            && result.latest.score == 3240
            && result.personalBest.score == result.latest.score
            && mission.CompletionRecordCount() == 1,
        "pass37 first completion records a Gold latest result and personal best exactly once");
    Expect(mission.EntryReport(guide).blocker
                == ShadowCryptEntryBlocker::CompletedRunRequiresReplay
            && mission.EntryGuidance(guide) == ShadowCryptEntryGuidance::ReplayCompletedRun,
        "pass37 completed mission reports replay rather than fresh-entry guidance");

    Expect(mission.ReplayCompletedRun(guide) && mission.SuspendAtSafeBoundary(),
        "pass37 replay can be suspended at its validated initial room boundary");
    Expect(mission.EntryReport(guide).blocker == ShadowCryptEntryBlocker::CheckpointPending
            && mission.EntryGuidance(guide) == ShadowCryptEntryGuidance::ResumeCheckpoint,
        "pass37 suspended mission reports the checkpoint as the authoritative next action");
    Expect(mission.ResumeSuspendedRun()
            && mission.EntryReport(guide).blocker == ShadowCryptEntryBlocker::ActiveRun,
        "pass37 resuming consumes the checkpoint and restores the active-run entry blocker");
    CompleteShadowCryptMissionPass37(mission, 500, true, false);
    result = mission.LatestResult();
    Expect(result.available && !result.newPersonalBest
            && result.latest.grade == ShadowCryptGrade::Bronze
            && result.latest.score == 450
            && result.personalBest.score == 3240 && result.scoreBehindBest == 2790
            && mission.CompletionRecordCount() == 2
            && mission.CompletionRecordFromNewest(0).score == 450
            && mission.CompletionRecordFromNewest(1).score == 3240,
        "pass37 worse replay keeps the Gold best while retaining newest-first clear history");

    Expect(mission.ReplayCompletedRun(guide), "pass37 third clear replay starts");
    CompleteShadowCryptMissionPass37(mission, 100, false, false);
    Expect(mission.ReplayCompletedRun(guide), "pass37 fourth clear replay starts");
    CompleteShadowCryptMissionPass37(mission, 120, false, false);
    Expect(mission.ReplayCompletedRun(guide), "pass37 fifth clear replay starts");
    CompleteShadowCryptMissionPass37(mission, 250, false, false);

    Expect(mission.CompletionRecordCount() == ShadowCryptMission::CompletionHistoryCapacity
            && mission.CompletionRecordFromNewest(0).score == 750
            && mission.CompletionRecordFromNewest(1).score == 1880
            && mission.CompletionRecordFromNewest(2).score == 1900
            && mission.CompletionRecordFromNewest(3).score == 450
            && !mission.CompletionRecordFromNewest(4).valid
            && mission.BestRecord().score == 3240,
        "pass37 All Records archive stays bounded to four newest clears while the dropped Gold clear remains the personal best");
}

} // namespace
