namespace {

void AdvanceShadowCryptPass27ToRiftNave(Astral::Scene::LandmarkInteraction& owner) {
    using namespace Astral::Scene;
    Expect(owner.AdvanceShadowCryptObjective() && owner.AdvanceShadowCryptObjective()
            && owner.ShadowCryptBriefing().room == ShadowCryptRoom::ArchiveGallery,
        "pass27 live Shadow Crypt clears the two-anchor Entry Seal through the production owner");
    Expect(owner.AdvanceShadowCryptObjective() && owner.AdvanceShadowCryptObjective()
            && owner.AdvanceShadowCryptObjective()
            && owner.ShadowCryptBriefing().room == ShadowCryptRoom::RiftNave,
        "pass27 live Shadow Crypt clears the three-record Archive Gallery through the production owner");
}

void FinishShadowCryptPass27FromRiftNave(Astral::Scene::LandmarkInteraction& owner) {
    using namespace Astral::Scene;
    Expect(owner.AdvanceShadowCryptObjective() && owner.AdvanceShadowCryptObjective()
            && owner.ShadowCryptBriefing().room == ShadowCryptRoom::WardenSanctum,
        "pass27 live Shadow Crypt stabilizes two Rift Nave nodes before the Warden");
    Expect(owner.AdvanceShadowCryptObjective() && owner.ShadowCryptBriefing().complete,
        "pass27 live Shadow Crypt completes only after the Rift Warden objective");
}

void TestShadowCryptMissionPass27() {
    using namespace Astral::Scene;

    // GAME-132: production entry consumes the authoritative field-guide lead and
    // cannot be started from an unrelated fresh landmark owner.
    LandmarkInteraction lockedOwner;
    Expect(!lockedOwner.BeginShadowCrypt() && !lockedOwner.ShadowCryptBriefing().available,
        "pass27 Shadow Crypt production entry remains locked without authoritative narrative evidence");

    WorldBlockout world;
    ShadowbladeActions actions;
    CharacterProgression progression;
    LandmarkInteraction owner;
    owner.SetCharacterProgression(&progression);

    const Astral::Math::Vec3 positions[3]{
        {-8.0f, 18.0f, 0.0f},
        {4.0f, 39.0f, 0.0f},
        {5.0f, 68.0f, 0.0f},
    };
    for (const Astral::Math::Vec3& position : positions) {
        Expect(owner.TryInteract(position, world, actions).result
                == LandmarkInteractionResult::Discovered,
            "pass27 setup discovers each existing National Mall landmark through the real interaction path");
    }
    Expect(owner.ObjectiveComplete(),
        "pass27 setup completes the existing National Mall objective before advanced dialogue");

    const DialogueBeat cryptLore = owner.TryDialogueChoice(
        DialogueTopic::ShadowCrypt, DialogueChoice::Reassure);
    const DialogueBeat cryptSigil = owner.TryDialogueChoice(
        DialogueTopic::ShadowCrypt, DialogueChoice::ShareEvidence);
    Expect(cryptLore.loreUnlocked && cryptSigil.clueUnlocked
            && owner.FieldGuide().OperationProgress(FieldOperation::ShadowCryptLead).Complete(),
        "pass27 production dialogue produces both Shadow Crypt lead witnesses before entry");
    Expect(owner.BeginShadowCrypt() && !owner.BeginShadowCrypt(),
        "pass27 production owner starts exactly one active Shadow Crypt expedition");

    // GAME-133 + QOL-028: expose authoritative room progress and a compact next
    // action without duplicating dungeon state in UI-facing code.
    ShadowCryptMissionBriefing briefing = owner.ShadowCryptBriefing();
    Expect(briefing.available && briefing.active && !briefing.complete
            && briefing.room == ShadowCryptRoom::EntrySeal
            && briefing.objective == ShadowCryptObjective::BreakSealAnchors
            && briefing.objectiveProgress == 0 && briefing.objectiveRequired == 2
            && briefing.safeToSuspend
            && briefing.guidance == ShadowCryptGuidanceAction::BreakSealAnchor,
        "pass27 briefing exposes the first live room objective, bounded progress, safe boundary, and next action");

    Expect(owner.AdvanceShadowCryptObjective(),
        "pass27 production objective path advances one Entry Seal anchor");
    briefing = owner.ShadowCryptBriefing();
    Expect(briefing.objectiveProgress == 1 && !briefing.safeToSuspend
            && !owner.SuspendShadowCrypt(),
        "pass27 mid-objective state cannot be mislabeled or suspended as a safe checkpoint");
    Expect(owner.AdvanceShadowCryptObjective()
            && owner.ShadowCryptBriefing().room == ShadowCryptRoom::ArchiveGallery,
        "pass27 second Entry Seal anchor advances to the Archive Gallery exactly once");
    Expect(owner.AdvanceShadowCryptObjective() && owner.AdvanceShadowCryptObjective()
            && owner.AdvanceShadowCryptObjective(),
        "pass27 Archive Gallery requires all three authoritative record steps");

    briefing = owner.ShadowCryptBriefing();
    Expect(briefing.room == ShadowCryptRoom::RiftNave
            && briefing.guidance == ShadowCryptGuidanceAction::DiscoverCoolingCache,
        "pass27 objective tracker surfaces the newly eligible optional Cooling Cache before hiding it");
    Expect(owner.DiscoverShadowCryptCoolingCache()
            && owner.ShadowCryptBriefing().guidance == ShadowCryptGuidanceAction::ClearCoolingCache
            && owner.ClearShadowCryptCoolingCache()
            && owner.ShadowCryptBriefing().guidance == ShadowCryptGuidanceAction::StabilizeRiftNode,
        "pass27 guidance advances from cache discovery to cache clear and back to the main Rift objective");

    // GAME-134: room-boundary suspension moves the one authoritative live run
    // into a validated one-shot resume point and rejects stale double-resume.
    Expect(owner.ShadowCryptBriefing().safeToSuspend && owner.SuspendShadowCrypt(),
        "pass27 production owner suspends the live dungeon only at a validated room boundary");
    briefing = owner.ShadowCryptBriefing();
    Expect(briefing.suspended && !briefing.active && briefing.checkpointAvailable
            && briefing.room == ShadowCryptRoom::RiftNave
            && briefing.roomsCleared == 2 && briefing.coolingCacheCleared
            && briefing.guidance == ShadowCryptGuidanceAction::ResumeCheckpoint
            && !owner.BeginShadowCrypt(),
        "pass27 suspended briefing preserves bounded room/cache state and blocks accidental fresh-run overwrite");
    Expect(owner.ResumeShadowCrypt() && !owner.ResumeShadowCrypt()
            && !owner.ShadowCryptBriefing().checkpointAvailable
            && owner.ShadowCryptBriefing().active,
        "pass27 resume consumes the checkpoint once and restores one authoritative live timeline");

    Expect(owner.RecordShadowCryptDamageTaken(-10) == 0
            && owner.RecordShadowCryptDamageTaken(40) == 40,
        "pass27 production damage route rejects invalid input and preserves bounded valid damage");
    FinishShadowCryptPass27FromRiftNave(owner);
    briefing = owner.ShadowCryptBriefing();
    Expect(briefing.complete && !briefing.active
            && briefing.guidance == ShadowCryptGuidanceAction::ReplayRun,
        "pass27 completed briefing transitions the objective tracker to the replay action");

    // GAME-135: the production mission keeps a monotonic best clear summary that
    // survives replay while a worse later clear cannot replace it.
    const ShadowCryptMissionRecord firstRecord = owner.ShadowCryptBestRecord();
    Expect(firstRecord.valid && firstRecord.grade == ShadowCryptGrade::Gold
            && firstRecord.optionalCacheCleared && firstRecord.damageTaken == 40
            && firstRecord.defeats == 0 && firstRecord.score == 3210,
        "pass27 clean cache clear records the deterministic Gold personal best");

    // GAME-136: first-clear progression remains protagonist-owned and idempotent,
    // while the full dungeon can replay immediately without erasing the best.
    const ShadowCryptRewardReport reward = owner.ClaimShadowCryptFirstClearReward();
    Expect(reward.granted
            && reward.progression.experienceApplied == ShadowCryptExpedition::FirstClearExperience
            && reward.progression.masteryPointsApplied == ShadowCryptExpedition::FirstClearMastery
            && reward.progression.enhancementMaterialsApplied
                == ShadowCryptExpedition::FirstClearEnhancementMaterials
            && !owner.ClaimShadowCryptFirstClearReward().granted,
        "pass27 live first clear routes the existing bounded reward exactly once through persistent progression");
    Expect(owner.ReplayCompletedShadowCrypt()
            && owner.ShadowCryptBriefing().room == ShadowCryptRoom::EntrySeal
            && owner.ShadowCryptBriefing().active
            && !owner.ReplayCompletedShadowCrypt(),
        "pass27 one-action replay restarts the full expedition and rejects mid-run replay resets");
    const ShadowCryptMissionRecord retained = owner.ShadowCryptBestRecord();
    Expect(retained.valid && retained.score == firstRecord.score
            && retained.grade == firstRecord.grade,
        "pass27 replay preserves the already-earned personal best before the next clear");

    Expect(owner.RecordShadowCryptDamageTaken(600) == 600
            && owner.ReportShadowCryptDefeat(),
        "pass27 worse replay setup records bounded damage and one local defeat through production APIs");
    AdvanceShadowCryptPass27ToRiftNave(owner);
    FinishShadowCryptPass27FromRiftNave(owner);
    const ShadowCryptMissionRecord afterWorseReplay = owner.ShadowCryptBestRecord();
    Expect(afterWorseReplay.valid && afterWorseReplay.score == firstRecord.score
            && afterWorseReplay.grade == ShadowCryptGrade::Gold
            && !owner.ClaimShadowCryptFirstClearReward().granted,
        "pass27 worse replay cannot roll back the best record or regrant the persistent first-clear reward");

    LandmarkInteraction noProgressionOwner;
    Expect(!noProgressionOwner.ClaimShadowCryptFirstClearReward().granted,
        "pass27 missing progression owner fails closed without inventing a reward route");
}

} // namespace
