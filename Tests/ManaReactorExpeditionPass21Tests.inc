namespace {

Astral::Scene::ExplorationFieldGuide ManaReactorReadyGuide() {
    using namespace Astral::Scene;
    ExplorationFieldGuide guide;
    FieldNarrativeSnapshot evidence{};
    evidence.hasRiftResidue = true;
    evidence.hasCoolingAnomaly = true;
    guide.SyncNarrativeEvidence(evidence);
    return guide;
}

bool ClearManaReactorSafely(Astral::Scene::ManaReactorExpedition& run) {
    using namespace Astral::Scene;
    for (int step = 0; step < 2; ++step) {
        const auto result = run.ApplyControl(ManaReactorControl::Stabilize);
        if (step == 0 && result != ManaReactorControlResult::Applied) return false;
        if (step == 1 && result != ManaReactorControlResult::StageCleared) return false;
    }
    for (int step = 0; step < 3; ++step) {
        const auto result = run.ApplyControl(ManaReactorControl::Stabilize);
        if (step < 2 && result != ManaReactorControlResult::Applied) return false;
        if (step == 2 && result != ManaReactorControlResult::StageCleared) return false;
    }
    for (int step = 0; step < 3; ++step) {
        const auto result = run.ApplyControl(ManaReactorControl::Stabilize);
        if (step < 2 && result != ManaReactorControlResult::Applied) return false;
        if (step == 2 && result != ManaReactorControlResult::RunCompleted) return false;
    }
    return true;
}

void TestManaReactorExpeditionPass21() {
    using namespace Astral::Scene;

    // GAME-102: entry is tied to the existing two-clue Rift Investigation and reports bounded progress.
    ExplorationFieldGuide lockedGuide;
    ManaReactorExpedition locked;
    Expect(!locked.TryBegin(lockedGuide),
        "Mana Reactor entry remains locked before the Rift Investigation completes");

    ExplorationFieldGuide readyGuide = ManaReactorReadyGuide();
    Expect(readyGuide.OperationProgress(FieldOperation::RiftInvestigation).Complete(),
        "Mana Reactor fixture satisfies the authoritative Rift Investigation prerequisite");
    Expect(!locked.TryBegin(readyGuide, static_cast<ManaReactorMode>(255)),
        "invalid Mana Reactor mode fails closed before mutating the run");
    Expect(!locked.Active() && locked.ProgressPercent() == 0,
        "invalid Mana Reactor mode leaves the run inactive at zero progress");
    Expect(locked.TryBegin(readyGuide, ManaReactorMode::Expedition),
        "completed Rift Investigation admits an Expedition run");
    auto start = locked.Snapshot();
    Expect(start.active && start.stage == ManaReactorStage::IntakeBay
            && start.hazard == ManaReactorHazard::CondenserLeak
            && start.progressPercent == 0 && start.totalStages == 3,
        "Mana Reactor starts at the Intake Bay with explicit stage and hazard state");
    Expect(locked.ApplyControl(ManaReactorControl::Stabilize)
            == ManaReactorControlResult::Applied
            && locked.ProgressPercent() == 12,
        "partial Intake progress produces deterministic bounded expedition percentage");
    Expect(locked.ApplyControl(ManaReactorControl::Stabilize)
            == ManaReactorControlResult::StageCleared
            && locked.CurrentStage() == ManaReactorStage::CoolingLattice
            && locked.ProgressPercent() == 25,
        "clearing Intake advances exactly one stage and reports 25 percent total progress");

    // GAME-103: reactor controls trade objective speed against bounded thermal/stability pressure.
    ManaReactorExpedition controlRun;
    Expect(controlRun.TryBegin(readyGuide), "control-loop fixture starts");
    const auto beforeInvalid = controlRun.Snapshot();
    Expect(controlRun.ApplyControl(static_cast<ManaReactorControl>(255))
            == ManaReactorControlResult::Rejected,
        "invalid reactor control is rejected");
    const auto afterInvalid = controlRun.Snapshot();
    Expect(afterInvalid.heat == beforeInvalid.heat
            && afterInvalid.stability == beforeInvalid.stability
            && afterInvalid.objectiveProgress == beforeInvalid.objectiveProgress,
        "invalid reactor control is non-mutating");
    Expect(controlRun.ApplyControl(ManaReactorControl::Overdrive)
            == ManaReactorControlResult::StageCleared,
        "Overdrive can trade higher pressure for faster Intake completion");
    Expect(controlRun.Heat() >= 0 && controlRun.Heat() <= ManaReactorExpedition::MaximumHeat
            && controlRun.Stability() >= 0
            && controlRun.Stability() <= ManaReactorExpedition::MaximumStability,
        "heat and stability remain bounded after stage transition");
    Expect(controlRun.ApplyControl(ManaReactorControl::Stabilize)
            == ManaReactorControlResult::Applied
            && controlRun.ApplyControl(ManaReactorControl::Stabilize)
            == ManaReactorControlResult::Applied
            && controlRun.ApplyControl(ManaReactorControl::Stabilize)
            == ManaReactorControlResult::StageCleared,
        "safe Cooling Lattice controls reach the Core Chamber deterministically");
    Expect(controlRun.CurrentStage() == ManaReactorStage::CoreChamber
            && controlRun.ApplyControl(ManaReactorControl::Overdrive)
                == ManaReactorControlResult::Applied
            && controlRun.ApplyControl(ManaReactorControl::Overdrive)
                == ManaReactorControlResult::Failed,
        "repeated Core Overdrive reaches bounded thermal failure before stage completion");
    Expect(controlRun.Failed() && controlRun.Active()
            && controlRun.CurrentStage() == ManaReactorStage::CoreChamber
            && controlRun.StagesCleared() == 2 && !controlRun.Complete(),
        "reactor failure never silently advances or completes the current stage");
    Expect(controlRun.Heat() == ManaReactorExpedition::MaximumHeat,
        "thermal failure clamps heat at the declared maximum");
    const int failedProgress = controlRun.ObjectiveProgress();
    Expect(controlRun.ApplyControl(ManaReactorControl::Stabilize)
            == ManaReactorControlResult::Rejected
            && controlRun.ObjectiveProgress() == failedProgress,
        "failed stage rejects further controls until an explicit retry");

    // GAME-104: stage challenge targets are optional, bounded, and independent of main progress.
    ManaReactorExpedition challengeRun;
    Expect(challengeRun.TryBegin(readyGuide), "optional-target fixture starts");
    Expect(challengeRun.ApplyControl(ManaReactorControl::Stabilize)
            == ManaReactorControlResult::Applied
            && challengeRun.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::StageCleared
            && challengeRun.OptionalTargetsComplete() == 1,
        "safe Intake clear earns exactly one optional thermal/stability target");
    Expect(challengeRun.ApplyControl(ManaReactorControl::Overdrive)
            == ManaReactorControlResult::Applied
            && challengeRun.ApplyControl(ManaReactorControl::Balance)
                == ManaReactorControlResult::StageCleared
            && challengeRun.OptionalTargetsComplete() == 1,
        "risky Cooling clear advances main progress without falsely awarding its optional target");
    Expect(challengeRun.StagesCleared() == 2,
        "missing an optional target never blocks main stage progression");

    // QOL-022: retry current stage retains cleared work, while Start Over resets the run.
    ManaReactorExpedition retryRun;
    Expect(retryRun.TryBegin(readyGuide, ManaReactorMode::Calibration),
        "retry fixture starts in Calibration mode");
    Expect(retryRun.ApplyControl(ManaReactorControl::Stabilize)
            == ManaReactorControlResult::Applied
            && retryRun.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::StageCleared,
        "retry fixture clears Intake safely");
    const int clearedOptional = retryRun.OptionalTargetsComplete();
    Expect(retryRun.ApplyControl(ManaReactorControl::Overdrive)
            == ManaReactorControlResult::Applied,
        "retry fixture makes partial Cooling progress");
    Expect(retryRun.RetryCurrentStage(), "current-stage retry is explicitly available");
    const auto retried = retryRun.Snapshot();
    Expect(retried.stage == ManaReactorStage::CoolingLattice
            && retried.stagesCleared == 1
            && retried.optionalTargetsComplete == clearedOptional
            && retried.objectiveProgress == 0 && retried.heat == 45
            && retried.stability == 85 && retried.retries == 1
            && retried.mode == ManaReactorMode::Calibration,
        "retry restores only the current stage while retaining cleared progress and selected mode");
    Expect(retryRun.StartOver(), "full Start Over is explicitly available after a retry");
    const auto restarted = retryRun.Snapshot();
    Expect(restarted.stage == ManaReactorStage::IntakeBay
            && restarted.stagesCleared == 0 && restarted.optionalTargetsComplete == 0
            && restarted.progressPercent == 0 && restarted.retries == 0
            && restarted.mode == ManaReactorMode::Calibration,
        "Start Over resets run-local progress while retaining Calibration mode");
    ManaReactorExpedition neverStarted;
    Expect(!neverStarted.RetryCurrentStage() && !neverStarted.StartOver(),
        "retry and Start Over fail closed when no Mana Reactor run exists");

    // GAME-105: Calibration runs the real hazard loop without touching progression entitlements.
    ManaReactorExpedition calibration;
    CharacterProgression calibrationProgression;
    Expect(calibration.TryBegin(readyGuide, ManaReactorMode::Calibration)
            && ClearManaReactorSafely(calibration) && calibration.Complete(),
        "Calibration can complete the same bounded reactor stages");
    const auto calibrationSummary = calibration.CompletionSummary();
    Expect(calibrationSummary.grade == ManaReactorGrade::Gold
            && calibrationSummary.stagesCleared == ManaReactorExpedition::StageCount
            && calibrationSummary.optionalTargetsComplete == ManaReactorExpedition::StageCount
            && calibrationSummary.score > 0,
        "Calibration still exposes deterministic performance feedback");
    const auto calibrationReward = calibration.ClaimFirstClearReward(calibrationProgression);
    Expect(!calibrationReward.granted && !calibrationProgression.ManaReactorFirstClearClaimed()
            && calibrationProgression.Level() == 1
            && calibrationProgression.AvailableMasteryPoints() == 0
            && calibrationProgression.EnhancementMaterials() == 0,
        "Calibration cannot consume entitlement or grant progression rewards");

    // GAME-106: Expedition grading, highest score, replay, and first-clear rewards are idempotent.
    ManaReactorExpedition expedition;
    CharacterProgression progression;
    Expect(expedition.TryBegin(readyGuide, ManaReactorMode::Expedition)
            && ClearManaReactorSafely(expedition),
        "Expedition fixture completes all three stages safely");
    const auto gold = expedition.CompletionSummary();
    Expect(gold.grade == ManaReactorGrade::Gold && gold.score == 1310
            && expedition.BestExpeditionScore() == gold.score,
        "clean all-target Expedition produces deterministic Gold score and records the best score");
    const auto firstReward = expedition.ClaimFirstClearReward(progression);
    Expect(firstReward.granted && progression.ManaReactorFirstClearClaimed()
            && firstReward.progression.experienceApplied == ManaReactorExpedition::FirstClearExperience
            && firstReward.progression.masteryPointsApplied == ManaReactorExpedition::FirstClearMastery
            && firstReward.progression.enhancementMaterialsApplied
                == ManaReactorExpedition::FirstClearEnhancementMaterials,
        "first Expedition clear grants the bounded Mana Reactor progression package once");
    const int rewardedLevel = progression.Level();
    const int rewardedExperience = progression.ExperienceIntoLevel();
    const int rewardedMastery = progression.AvailableMasteryPoints();
    const int rewardedMaterials = progression.EnhancementMaterials();
    const auto duplicateReward = expedition.ClaimFirstClearReward(progression);
    Expect(!duplicateReward.granted && progression.Level() == rewardedLevel
            && progression.ExperienceIntoLevel() == rewardedExperience
            && progression.AvailableMasteryPoints() == rewardedMastery
            && progression.EnhancementMaterials() == rewardedMaterials,
        "repeat claim cannot duplicate Mana Reactor progression rewards");

    ManaReactorExpedition replay;
    Expect(replay.TryBegin(readyGuide) && ClearManaReactorSafely(replay),
        "completed Mana Reactor expedition remains replayable in a fresh run object");
    const auto staleObjectClaim = replay.ClaimFirstClearReward(progression);
    Expect(!staleObjectClaim.granted && progression.Level() == rewardedLevel
            && progression.ExperienceIntoLevel() == rewardedExperience
            && progression.AvailableMasteryPoints() == rewardedMastery
            && progression.EnhancementMaterials() == rewardedMaterials,
        "progression-owned first-clear entitlement blocks duplicate reward across run objects");
    Expect(!replay.RetryCurrentStage(),
        "completed expedition cannot be misinterpreted as an active current-stage retry");
}

} // namespace
