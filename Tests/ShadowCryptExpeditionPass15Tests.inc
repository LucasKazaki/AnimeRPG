namespace {

void CompleteShadowCryptEntryAndArchive(Astral::Scene::ShadowCryptExpedition& run) {
    using namespace Astral::Scene;
    Expect(run.RecordObjectiveStep() && run.RecordObjectiveStep()
            && run.CurrentRoom() == ShadowCryptRoom::ArchiveGallery,
        "Shadow Crypt entry seal requires exactly two objective steps before advancing");
    Expect(run.RecordObjectiveStep() && run.RecordObjectiveStep() && run.RecordObjectiveStep()
            && run.CurrentRoom() == ShadowCryptRoom::RiftNave,
        "Shadow Crypt archive gallery requires exactly three records before the Rift Nave");
}

void CompleteShadowCryptRun(Astral::Scene::ShadowCryptExpedition& run) {
    using namespace Astral::Scene;
    if (run.CurrentRoom() == ShadowCryptRoom::EntrySeal) {
        CompleteShadowCryptEntryAndArchive(run);
    }
    if (run.CurrentRoom() == ShadowCryptRoom::RiftNave) {
        Expect(run.RecordObjectiveStep() && run.RecordObjectiveStep(),
            "Shadow Crypt Rift Nave requires two stabilization nodes before the Warden");
    }
    Expect(run.CurrentRoom() == ShadowCryptRoom::WardenSanctum
            && run.CurrentObjective() == ShadowCryptObjective::DefeatRiftWarden
            && run.RecordObjectiveStep() && run.Complete()
            && !run.Active() && run.RoomsCleared() == ShadowCryptExpedition::MainRoomCount,
        "defeating the Rift Warden completes the bounded four-room expedition once");
}

void TestShadowCryptExpeditionPass15() {
    using namespace Astral::Scene;

    ExplorationFieldGuide guide;
    ShadowCryptExpedition locked;
    Expect(!locked.TryBegin(guide) && !locked.Active(),
        "Shadow Crypt expedition entry is locked until the existing field-guide lead is complete");

    FieldNarrativeSnapshot lead{};
    lead.hasCryptSigil = true;
    lead.hasShadowCryptLore = true;
    guide.SyncNarrativeEvidence(lead);
    Expect(guide.OperationProgress(FieldOperation::ShadowCryptLead).Complete()
            && locked.TryBegin(guide) && locked.Active()
            && !locked.TryBegin(guide),
        "completed Crypt Sigil and lore evidence unlock one active Shadow Crypt run without double-starting");
    Expect(locked.CurrentRoom() == ShadowCryptRoom::EntrySeal
            && locked.CurrentObjective() == ShadowCryptObjective::BreakSealAnchors
            && locked.ObjectiveProgress() == 0 && locked.ObjectiveRequired() == 2,
        "the expedition exposes an explicit room-specific first objective and bounded progress requirement");

    Expect(locked.SafeToSuspend() && locked.CreateResumePoint().valid,
        "fresh room boundaries expose a deterministic safe-to-suspend point");
    Expect(locked.RecordObjectiveStep() && !locked.SafeToSuspend()
            && !locked.CreateResumePoint().valid,
        "mid-objective progress is not advertised as a safe suspension boundary");
    Expect(locked.ReportDefeat() && locked.Defeats() == 1
            && locked.ObjectiveProgress() == 0
            && locked.CurrentRoom() == ShadowCryptRoom::EntrySeal,
        "defeat retries only the current room objective while preserving expedition identity");

    CompleteShadowCryptEntryAndArchive(locked);
    Expect(locked.RoomsCleared() == 2
            && locked.DiscoverCoolingCache() && !locked.DiscoverCoolingCache()
            && locked.ClearCoolingCache() && !locked.ClearCoolingCache(),
        "the optional Cooling Cache opens after two cleared rooms and can be discovered and cleared once");
    Expect(locked.SafeToSuspend(),
        "the Rift Nave boundary remains a safe suspension point after optional-cache completion");

    const ShadowCryptResumePoint checkpoint = locked.CreateResumePoint();
    Expect(checkpoint.valid && checkpoint.room == ShadowCryptRoom::RiftNave
            && checkpoint.coolingCacheDiscovered && checkpoint.coolingCacheCleared
            && checkpoint.defeats == 1,
        "resume metadata preserves completed-room, side-cache, and defeat state at the room boundary");
    Expect(locked.RecordObjectiveStep(),
        "the source run can advance after creating a non-mutating checkpoint snapshot");

    ShadowCryptExpedition activeReceiver;
    Expect(activeReceiver.TryBegin(guide) && !activeReceiver.ResumeFrom(checkpoint)
            && activeReceiver.CurrentRoom() == ShadowCryptRoom::EntrySeal,
        "a stale checkpoint cannot rewind an already-active expedition");

    ShadowCryptExpedition resumed;
    Expect(resumed.ResumeFrom(checkpoint)
            && resumed.CurrentRoom() == ShadowCryptRoom::RiftNave
            && resumed.ObjectiveProgress() == 0 && resumed.CoolingCacheCleared(),
        "a valid boundary checkpoint restores the exact next room with no fabricated partial objective progress");
    Expect(resumed.RecordDamageTaken(40) == 40
            && resumed.RecordDamageTaken(-1) == 0
            && resumed.RecordDamageTaken(std::numeric_limits<int>::max()) == 999960
            && resumed.DamageTaken() == 1000000,
        "Shadow Crypt damage accounting ignores invalid input and saturates at its explicit bound");
    CompleteShadowCryptRun(resumed);
    const ShadowCryptCompletion bronze = resumed.CompletionSummary();
    Expect(bronze.grade == ShadowCryptGrade::Bronze
            && bronze.roomsCleared == 4 && bronze.optionalCacheCleared,
        "large damage downgrades a completed run to Bronze even when the optional cache was cleared");

    CharacterProgression progression;
    const ShadowCryptRewardReport reward = resumed.ClaimFirstClearReward(progression);
    Expect(reward.granted
            && reward.progression.experienceApplied == ShadowCryptExpedition::FirstClearExperience
            && reward.progression.masteryPointsApplied == ShadowCryptExpedition::FirstClearMastery
            && reward.progression.enhancementMaterialsApplied
                == ShadowCryptExpedition::FirstClearEnhancementMaterials,
        "first Shadow Crypt completion grants the bounded progression package through CharacterProgression");
    Expect(!resumed.ClaimFirstClearReward(progression).granted,
        "repeated claims cannot duplicate the first-clear progression reward");

    Expect(resumed.TryBegin(guide),
        "completed Shadow Crypt can be replayed without resetting the persistent first-clear claim");
    const ShadowCryptResumePoint replayPoint = resumed.CreateResumePoint();
    Expect(replayPoint.valid && replayPoint.firstClearRewardClaimed,
        "replay checkpoints preserve consumed first-clear reward state");
    ShadowCryptExpedition replayed;
    Expect(replayed.ResumeFrom(replayPoint),
        "a valid replay boundary can resume in a fresh expedition object");
    CompleteShadowCryptRun(replayed);
    Expect(!replayed.ClaimFirstClearReward(progression).granted,
        "resume plus replay cannot regrant a previously consumed first-clear reward");

    ShadowCryptExpedition silver;
    Expect(silver.TryBegin(guide) && silver.ReportDefeat()
            && silver.RecordDamageTaken(150) == 150,
        "Silver-grade boundary inputs are accepted without hidden multipliers");
    CompleteShadowCryptRun(silver);
    Expect(silver.CompletionSummary().grade == ShadowCryptGrade::Silver,
        "one defeat with at most 150 damage resolves to Silver");

    ShadowCryptExpedition gold;
    Expect(gold.TryBegin(guide), "a clean comparison run can start from the completed field-guide lead");
    CompleteShadowCryptEntryAndArchive(gold);
    Expect(gold.DiscoverCoolingCache() && gold.ClearCoolingCache()
            && gold.RecordDamageTaken(50) == 50,
        "Gold-grade optional objective and damage boundary are reachable deterministically");
    CompleteShadowCryptRun(gold);
    Expect(gold.CompletionSummary().grade == ShadowCryptGrade::Gold,
        "flawless cache-clearing run at the damage boundary resolves to Gold");

    ShadowCryptResumePoint bad = checkpoint;
    bad.coolingCacheDiscovered = false;
    bad.coolingCacheCleared = true;
    ShadowCryptExpedition untouched;
    Expect(!untouched.ResumeFrom(bad) && !untouched.Active() && !untouched.Complete(),
        "inconsistent optional-cache checkpoint data fails closed without mutating the receiver");
    bad = checkpoint;
    bad.schemaVersion = ShadowCryptResumePoint::SchemaVersion + 1;
    Expect(!untouched.ResumeFrom(bad),
        "unknown checkpoint schema versions fail closed");
    bad = checkpoint;
    bad.room = static_cast<ShadowCryptRoom>(255);
    Expect(!untouched.ResumeFrom(bad),
        "invalid checkpoint room ids fail closed");
    bad = checkpoint;
    bad.defeats = -1;
    Expect(!untouched.ResumeFrom(bad),
        "negative checkpoint counters fail closed");

    ShadowCryptExpedition gatedCache;
    Expect(gatedCache.TryBegin(guide) && !gatedCache.DiscoverCoolingCache(),
        "the optional cache cannot be claimed before its two-room exploration gate");
}

} // namespace
