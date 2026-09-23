namespace {

bool ClearManaReactorMissionSafely(Astral::Scene::ManaReactorMission& mission) {
    using namespace Astral::Scene;
    const ManaReactorControlResult expected[8]{
        ManaReactorControlResult::Applied,
        ManaReactorControlResult::StageCleared,
        ManaReactorControlResult::Applied,
        ManaReactorControlResult::Applied,
        ManaReactorControlResult::StageCleared,
        ManaReactorControlResult::Applied,
        ManaReactorControlResult::Applied,
        ManaReactorControlResult::RunCompleted,
    };
    for (ManaReactorControlResult result : expected) {
        if (mission.ApplyControl(ManaReactorControl::Stabilize) != result) return false;
    }
    return true;
}

bool ClearLandmarkManaReactorSafely(Astral::Scene::LandmarkInteraction& interaction) {
    using namespace Astral::Scene;
    const ManaReactorControlResult expected[8]{
        ManaReactorControlResult::Applied,
        ManaReactorControlResult::StageCleared,
        ManaReactorControlResult::Applied,
        ManaReactorControlResult::Applied,
        ManaReactorControlResult::StageCleared,
        ManaReactorControlResult::Applied,
        ManaReactorControlResult::Applied,
        ManaReactorControlResult::RunCompleted,
    };
    for (ManaReactorControlResult result : expected) {
        if (interaction.ApplyManaReactorControl(ManaReactorControl::Stabilize) != result) return false;
    }
    return true;
}

void TestManaReactorMissionPass26() {
    using namespace Astral::Scene;

    // GAME-127: the production LandmarkInteraction owner launches the reactor only
    // after its real Rift Investigation evidence path completes.
    WorldBlockout world;
    LandmarkInteraction live;
    ShadowbladeActions actions;
    CharacterProgression progression;
    live.SetCharacterProgression(&progression);
    Expect(!live.BeginManaReactor(),
        "live Mana Reactor mission remains locked before Rift Investigation evidence");

    const Astral::Math::Vec3 positions[3]{{-8.0f, 18.0f, 0.0f}, {4.0f, 39.0f, 0.0f},
        {5.0f, 68.0f, 0.0f}};
    for (const Astral::Math::Vec3& position : positions) {
        live.TryInteract(position, world, actions);
    }
    const DialogueBeat riftEvidence = live.TryDialogueChoice(
        DialogueTopic::RiftTheory, DialogueChoice::ShareEvidence);
    const DialogueBeat coolingEvidence = live.TryDialogueChoice(
        DialogueTopic::LandmarkHistory, DialogueChoice::ShareEvidence);
    Expect(riftEvidence.clueUnlocked && coolingEvidence.clueUnlocked
            && live.FieldGuide().OperationProgress(FieldOperation::RiftInvestigation).Complete(),
        "existing landmark and dialogue evidence completes the authoritative Rift Investigation gate");
    Expect(live.BeginManaReactor(ManaReactorMode::Expedition,
               ManaReactorDifficulty::Standard, ManaReactorProtocol::Baseline)
            && live.ManaReactor().Expedition().Active(),
        "LandmarkInteraction now owns a live expedition launch using the existing reactor domain");

    // GAME-128: each stage exposes non-mutating three-control forecasts and one
    // deterministic recommendation derived from authoritative projected state.
    const ManaReactorSnapshot beforeBriefing = live.ManaReactor().Expedition().Snapshot();
    const ManaReactorMissionBriefing briefing = live.ManaReactorBriefing();
    const ManaReactorSnapshot afterBriefing = live.ManaReactor().Expedition().Snapshot();
    Expect(briefing.available && briefing.recommendationAvailable
            && briefing.snapshot.stage == ManaReactorStage::IntakeBay
            && briefing.controlPreviews[0].valid
            && briefing.controlPreviews[1].valid
            && briefing.controlPreviews[2].valid,
        "live reactor briefing compares all three valid control choices");
    Expect(briefing.recommendedControl == ManaReactorControl::Overdrive,
        "Intake briefing recommends the fastest safe stage-clearing control");
    Expect(beforeBriefing.heat == afterBriefing.heat
            && beforeBriefing.stability == afterBriefing.stability
            && beforeBriefing.objectiveProgress == afterBriefing.objectiveProgress,
        "reading reactor forecasts never mutates the authoritative run");

    // GAME-129: failure produces an explicit current-stage recovery action and
    // retry preserves cleared-stage progress.
    ManaReactorMission recovery;
    const ExplorationFieldGuide readyGuide = ManaReactorReadyGuide();
    Expect(recovery.Begin(readyGuide), "recovery fixture starts");
    Expect(recovery.ApplyControl(ManaReactorControl::Overdrive)
            == ManaReactorControlResult::StageCleared,
        "recovery fixture clears Intake quickly");
    Expect(recovery.ApplyControl(ManaReactorControl::Stabilize)
            == ManaReactorControlResult::Applied
            && recovery.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::Applied
            && recovery.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::StageCleared,
        "recovery fixture reaches Core Chamber");
    Expect(recovery.ApplyControl(ManaReactorControl::Overdrive)
            == ManaReactorControlResult::Applied
            && recovery.ApplyControl(ManaReactorControl::Overdrive)
                == ManaReactorControlResult::Failed,
        "recovery fixture reaches an authoritative Core failure");
    const ManaReactorMissionBriefing failedBriefing = recovery.Briefing();
    Expect(failedBriefing.recovery == ManaReactorMissionRecoveryAction::RetryStage
            && !failedBriefing.emergencyVentSuggested,
        "failed reactor guidance offers only an actually usable recovery action");
    Expect(recovery.RetryCurrentStage()
            && recovery.Expedition().CurrentStage() == ManaReactorStage::CoreChamber
            && recovery.Expedition().StagesCleared() == 2
            && !recovery.Expedition().Failed(),
        "failure guidance retries only the failed stage and retains cleared work");

    // GAME-130: best score/grade is retained per difficulty+protocol bucket and
    // consequence-free Calibration cannot overwrite Expedition records.
    ManaReactorMission records;
    Expect(records.Begin(readyGuide, ManaReactorMode::Expedition,
               ManaReactorDifficulty::Standard, ManaReactorProtocol::Baseline)
            && ClearManaReactorMissionSafely(records),
        "personal-best fixture completes a clean standard baseline run");
    const ManaReactorMissionRecord best = records.BestRecord(
        ManaReactorDifficulty::Standard, ManaReactorProtocol::Baseline);
    const ManaReactorMissionRecord separateBucket = records.BestRecord(
        ManaReactorDifficulty::Critical, ManaReactorProtocol::Baseline);
    Expect(best.valid && best.grade == ManaReactorGrade::Gold && best.score == 1310,
        "clean reactor clear records the authoritative score and grade");
    Expect(!separateBucket.valid,
        "personal-best records stay isolated across difficulty/protocol configurations");
    ManaReactorMission calibrationRecords;
    Expect(calibrationRecords.Begin(readyGuide, ManaReactorMode::Calibration,
               ManaReactorDifficulty::Standard, ManaReactorProtocol::Baseline)
            && ClearManaReactorMissionSafely(calibrationRecords)
            && !calibrationRecords.BestRecord(ManaReactorDifficulty::Standard,
                ManaReactorProtocol::Baseline).valid,
        "Calibration completion cannot replace or fabricate an Expedition personal best");

    // GAME-131: the live mission claims the existing one-time protagonist reward
    // without adding a second economy path.
    Expect(ClearLandmarkManaReactorSafely(live) && live.ManaReactor().Expedition().Complete(),
        "live LandmarkInteraction reactor reaches completion through production wrappers");
    const ManaReactorRewardReport reward = live.ClaimManaReactorFirstClearReward();
    const int rewardLevel = progression.Level();
    const int rewardExperience = progression.ExperienceIntoLevel();
    const int rewardMastery = progression.AvailableMasteryPoints();
    const int rewardMaterials = progression.EnhancementMaterials();
    Expect(reward.granted && progression.ManaReactorFirstClearClaimed()
            && reward.progression.experienceApplied == ManaReactorExpedition::FirstClearExperience
            && reward.progression.masteryPointsApplied == ManaReactorExpedition::FirstClearMastery
            && reward.progression.enhancementMaterialsApplied
                == ManaReactorExpedition::FirstClearEnhancementMaterials,
        "live first clear routes through the existing one-time CharacterProgression entitlement");
    const ManaReactorRewardReport duplicate = live.ClaimManaReactorFirstClearReward();
    Expect(!duplicate.granted && progression.Level() == rewardLevel
            && progression.ExperienceIntoLevel() == rewardExperience
            && progression.AvailableMasteryPoints() == rewardMastery
            && progression.EnhancementMaterials() == rewardMaterials,
        "repeated live reward claim cannot duplicate progression");

    // QOL-027: completed runs can replay immediately with exact prior settings,
    // without rebuilding configuration or destroying the saved best record.
    const ManaReactorMissionRecord liveBest = live.ManaReactorBestRecord(
        ManaReactorDifficulty::Standard, ManaReactorProtocol::Baseline);
    const ManaReactorMissionBriefing completeBriefing = live.ManaReactorBriefing();
    Expect(completeBriefing.recovery == ManaReactorMissionRecoveryAction::ReplayRun,
        "completed reactor briefing offers a direct replay action");
    Expect(live.ReplayCompletedManaReactor(),
        "completed live reactor can restart the same configuration in one action");
    const ManaReactorSnapshot replayed = live.ManaReactor().Expedition().Snapshot();
    const ManaReactorMissionRecord preservedBest = live.ManaReactorBestRecord(
        ManaReactorDifficulty::Standard, ManaReactorProtocol::Baseline);
    Expect(replayed.active && replayed.stage == ManaReactorStage::IntakeBay
            && replayed.difficulty == ManaReactorDifficulty::Standard
            && replayed.protocol == ManaReactorProtocol::Baseline
            && replayed.progressPercent == 0,
        "quick replay preserves exact configuration while resetting only run-local progress");
    Expect(liveBest.valid && preservedBest.valid
            && preservedBest.score == liveBest.score && preservedBest.grade == liveBest.grade,
        "quick replay preserves the prior personal-best record");
    Expect(!live.ReplayCompletedManaReactor(),
        "quick replay fails closed while the replayed run is active");

    ManaReactorMission invalid;
    const ManaReactorMissionRecord invalidRecord = invalid.BestRecord(
        static_cast<ManaReactorDifficulty>(255), ManaReactorProtocol::Baseline);
    Expect(!invalidRecord.valid && !invalid.ReplayCompletedRun(),
        "invalid record keys and never-started replay fail closed");
}

} // namespace
