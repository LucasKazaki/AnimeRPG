void TestShadowbladeTrainingHubPass25ReviewRepairs() {
    using namespace Astral::Scene;

    const auto TrainingDiscovery = [](ShadowbladeActions& owner) {
        LandmarkInteractionReport report{};
        report.result = LandmarkInteractionResult::Discovered;
        report.landmark = LandmarkEncounter::EncounterLandmark;
        report.actionOwner = &owner;
        return report;
    };

    // Independent-review regression: encounter completion and the training unlock
    // must stay bound to the same combat/action pair that established activation.
    // An already-defeated unrelated combat owner or a mismatched action owner
    // cannot complete the story gate, receive its completion transition, or unlock
    // the practice hub.
    LandmarkEncounter ownerEncounter;
    CombatSandbox ownerCombat;
    ShadowbladeActions ownerActions;
    CombatSandbox unrelatedOwnerCombat;
    ShadowbladeActions unrelatedOwnerActions;
    Expect(ownerEncounter.TryActivate(TrainingDiscovery(ownerActions), ownerCombat).result
            == LandmarkEncounterResult::Activated,
        "pass25 review owner fixture activates with an explicit combat/action pair");
    unrelatedOwnerCombat.ApplyDamage(1000);
    Expect(!ownerEncounter.Update(unrelatedOwnerCombat, unrelatedOwnerActions)
            && ownerEncounter.State() == LandmarkEncounterState::Active
            && ownerEncounter.TrainingHub().State() == ShadowbladeTrainingHubState::Locked,
        "pass25 review unrelated defeated combat owner cannot satisfy the story gate");
    ownerCombat.ApplyDamage(1000);
    Expect(!ownerEncounter.Update(ownerCombat, unrelatedOwnerActions)
            && ownerEncounter.State() == LandmarkEncounterState::Active
            && ownerEncounter.TrainingHub().State() == ShadowbladeTrainingHubState::Locked,
        "pass25 review mismatched action owner cannot complete or unlock the encounter");
    Expect(ownerEncounter.Update(ownerCombat, ownerActions)
            && ownerEncounter.State() == LandmarkEncounterState::Completed
            && ownerEncounter.TrainingHub().State() == ShadowbladeTrainingHubState::Ready,
        "pass25 review exact activation owners complete and unlock training");

    // Independent-review regression: active timing guidance may only describe the
    // threat owned by the current DefensePracticeSession. If the combat planner is
    // externally reset/replaced while the original Shadowblade threat still exists,
    // the hub must fail closed instead of presenting guidance for the replacement.
    LandmarkEncounter guideEncounter;
    CombatSandbox guideCombat;
    ShadowbladeActions guideActions;
    Expect(guideEncounter.TryActivate(TrainingDiscovery(guideActions), guideCombat).result
            == LandmarkEncounterResult::Activated,
        "pass25 review timing fixture activates encounter");
    guideCombat.ApplyDamage(1000);
    Expect(guideEncounter.Update(guideCombat, guideActions)
            && guideEncounter.ConfigureTraining(
                ShadowbladeTrainingFocus::BossCycle,
                DefensePracticePace::Standard, 2)
            && guideEncounter.StartTraining(guideCombat, guideActions),
        "pass25 review timing fixture starts a bound multi-pattern run");
    const ShadowbladeTrainingHubFeedback ownedGuide =
        guideEncounter.TrainingFeedback(guideCombat);
    Expect(ownedGuide.timingGuideValid
            && ownedGuide.timingGuide.pattern == EnemyAttackPattern::QuickCut,
        "pass25 review timing fixture begins with the owned QuickCut guide");

    guideCombat.ResetTrainingSession();
    Expect(guideActions.HasIncomingAttack()
            && guideCombat.QueueEnemyAttackPattern(EnemyAttackPattern::GuardBreaker)
            && guideCombat.HasPendingEnemyAttack()
            && guideCombat.PendingEnemyAttack().pattern == EnemyAttackPattern::GuardBreaker,
        "pass25 review fixture replaces only the combat-side pending threat");
    const ShadowbladeTrainingHubFeedback replacementGuide =
        guideEncounter.TrainingFeedback(guideCombat);
    Expect(!replacementGuide.timingGuideValid,
        "pass25 timing guidance rejects a replacement threat not owned by the practice session");

    // Independent-review regression: once a run enters Debrief, rolling damage
    // telemetry belongs to that completed run and must stay frozen even if the
    // bound combat owner is later reused by encounter flow or mutated directly.
    LandmarkEncounter telemetryEncounter;
    CombatSandbox telemetryCombat;
    ShadowbladeActions telemetryActions;
    Expect(telemetryEncounter.TryActivate(
               TrainingDiscovery(telemetryActions), telemetryCombat).result
            == LandmarkEncounterResult::Activated,
        "pass25 review telemetry fixture activates encounter");
    telemetryCombat.ApplyDamage(1000);
    Expect(telemetryEncounter.Update(telemetryCombat, telemetryActions)
            && telemetryEncounter.ConfigureTraining(
                ShadowbladeTrainingFocus::QuickCut,
                DefensePracticePace::Learning, 1)
            && telemetryEncounter.StartTraining(telemetryCombat, telemetryActions),
        "pass25 review telemetry fixture starts one-attempt practice");
    Expect(telemetryCombat.ApplyDamage(30) == 30,
        "pass25 review telemetry fixture records active-run damage");
    telemetryEncounter.Update(telemetryCombat, telemetryActions);

    const EnemyAttackPlan telemetryPlan = telemetryCombat.PendingEnemyAttack();
    telemetryEncounter.AdvanceTraining(
        telemetryCombat, telemetryActions, telemetryPlan.windupSeconds - 0.10f);
    const DefenseReport telemetryDefense = telemetryEncounter.TryTrainingDefense(
        telemetryCombat, telemetryActions,
        telemetryPlan.blockable ? DefenseInput::Guard : DefenseInput::Dodge);
    Expect((telemetryDefense.result == DefenseResult::PerfectGuard
            || telemetryDefense.result == DefenseResult::PerfectDodge)
            && telemetryEncounter.TrainingHub().State()
                == ShadowbladeTrainingHubState::Debrief,
        "pass25 review telemetry fixture reaches debrief on its exact attempt cap");

    const ShadowbladeTrainingDamageWindow frozenBefore =
        telemetryEncounter.TrainingFeedback(telemetryCombat).recentDamage;
    Expect(frozenBefore.valid && frozenBefore.damage == 30 && frozenBefore.hits == 1,
        "pass25 review telemetry fixture captures final active-run damage window");

    telemetryCombat.AdvanceTime(1.0f);
    Expect(telemetryCombat.ApplyDamage(25) == 25,
        "pass25 review telemetry fixture mutates the reused combat owner after debrief");
    telemetryEncounter.Update(telemetryCombat, telemetryActions);
    const ShadowbladeTrainingDamageWindow frozenAfter =
        telemetryEncounter.TrainingFeedback(telemetryCombat).recentDamage;
    Expect(frozenAfter.valid
            && frozenAfter.damage == frozenBefore.damage
            && frozenAfter.hits == frozenBefore.hits
            && frozenAfter.windowSeconds == frozenBefore.windowSeconds,
        "pass25 debrief telemetry remains frozen after post-run combat mutations");

    // Independent-review regression: a damage event that occurred before the
    // exact 20-second cutoff must not remain in the reported rolling window just
    // because the nearest stored sample before the cutoff is older.
    LandmarkEncounter cutoffEncounter;
    CombatSandbox cutoffCombat;
    ShadowbladeActions cutoffActions;
    Expect(cutoffEncounter.TryActivate(
               TrainingDiscovery(cutoffActions), cutoffCombat).result
            == LandmarkEncounterResult::Activated,
        "pass25 review cutoff fixture activates encounter");
    cutoffCombat.ApplyDamage(1000);
    Expect(cutoffEncounter.Update(cutoffCombat, cutoffActions)
            && cutoffEncounter.ConfigureTraining(
                ShadowbladeTrainingFocus::QuickCut,
                DefensePracticePace::Learning, 2)
            && cutoffEncounter.StartTraining(cutoffCombat, cutoffActions),
        "pass25 review cutoff fixture starts active practice");

    cutoffCombat.AdvanceTime(0.05f);
    Expect(cutoffCombat.ApplyDamage(10) == 10,
        "pass25 review cutoff fixture records pre-window damage between samples");
    cutoffEncounter.Update(cutoffCombat, cutoffActions);
    cutoffCombat.AdvanceTime(0.15f);
    cutoffEncounter.Update(cutoffCombat, cutoffActions);
    const ShadowbladeTrainingDamageWindow beforeExpiry =
        cutoffEncounter.TrainingFeedback(cutoffCombat).recentDamage;
    Expect(beforeExpiry.valid && beforeExpiry.damage == 10 && beforeExpiry.hits == 1,
        "pass25 review cutoff fixture initially includes the recent damage");

    cutoffCombat.AdvanceTime(19.90f);
    cutoffEncounter.Update(cutoffCombat, cutoffActions);
    const ShadowbladeTrainingDamageWindow afterExpiry =
        cutoffEncounter.TrainingFeedback(cutoffCombat).recentDamage;
    Expect(afterExpiry.valid && afterExpiry.damage == 0 && afterExpiry.hits == 0
            && afterExpiry.windowSeconds <= ShadowbladeTrainingHub::DamageWindowSeconds,
        "pass25 rolling telemetry expires damage before the exact twenty-second cutoff");

    // Independent-review regression: a stale unflushed burst cannot absorb a
    // fresh delta after a long observation gap. The old activity must age out
    // while the new damage remains visible in the current 20-second window.
    LandmarkEncounter sparseEncounter;
    CombatSandbox sparseCombat;
    ShadowbladeActions sparseActions;
    Expect(sparseEncounter.TryActivate(
               TrainingDiscovery(sparseActions), sparseCombat).result
            == LandmarkEncounterResult::Activated,
        "pass25 review sparse telemetry fixture activates encounter");
    sparseCombat.ApplyDamage(1000);
    Expect(sparseEncounter.Update(sparseCombat, sparseActions)
            && sparseEncounter.ConfigureTraining(
                ShadowbladeTrainingFocus::QuickCut,
                DefensePracticePace::Learning, 2)
            && sparseEncounter.StartTraining(sparseCombat, sparseActions),
        "pass25 review sparse telemetry fixture starts active practice");
    sparseCombat.AdvanceTime(0.05f);
    Expect(sparseCombat.ApplyDamage(10) == 10,
        "pass25 review sparse fixture creates an unflushed early damage bucket");
    sparseEncounter.Update(sparseCombat, sparseActions);
    sparseCombat.AdvanceTime(21.0f);
    Expect(sparseCombat.ApplyDamage(15) == 15,
        "pass25 review sparse fixture records fresh damage after the old bucket expires");
    sparseEncounter.Update(sparseCombat, sparseActions);
    const ShadowbladeTrainingDamageWindow sparseWindow =
        sparseEncounter.TrainingFeedback(sparseCombat).recentDamage;
    Expect(sparseWindow.valid && sparseWindow.damage == 15 && sparseWindow.hits == 1
            && sparseWindow.windowSeconds <= ShadowbladeTrainingHub::DamageWindowSeconds,
        "pass25 sparse telemetry excludes stale pending damage but preserves the fresh delta");

    // Independent-review regression: once the story gate has completed with an
    // explicit owner pair, neither training start nor encounter retry may rebind
    // that unlock to unrelated owners before the hub has established its own pair.
    LandmarkEncounter startOwnerEncounter;
    CombatSandbox startOwnerCombat;
    ShadowbladeActions startOwnerActions;
    CombatSandbox foreignStartCombat;
    ShadowbladeActions foreignStartActions;
    Expect(startOwnerEncounter.TryActivate(
               TrainingDiscovery(startOwnerActions), startOwnerCombat).result
            == LandmarkEncounterResult::Activated,
        "pass25 review start-owner fixture activates with explicit owners");
    startOwnerCombat.ApplyDamage(1000);
    Expect(startOwnerEncounter.Update(startOwnerCombat, startOwnerActions)
            && startOwnerEncounter.ConfigureTraining(
                ShadowbladeTrainingFocus::QuickCut,
                DefensePracticePace::Learning, 2),
        "pass25 review start-owner fixture completes and configures training");
    const int foreignHealthBeforeRetry = foreignStartCombat.Dummy().health;
    const float foreignResourceBeforeRetry = foreignStartActions.Resource();
    Expect(startOwnerEncounter.Retry(foreignStartCombat, foreignStartActions).result
            == LandmarkEncounterResult::RetryUnavailable
            && startOwnerEncounter.State() == LandmarkEncounterState::Completed
            && foreignStartCombat.Dummy().health == foreignHealthBeforeRetry
            && foreignStartActions.Resource() == foreignResourceBeforeRetry,
        "pass25 review completed story gate cannot retry through unrelated owners");
    Expect(!startOwnerEncounter.StartTraining(foreignStartCombat, foreignStartActions)
            && startOwnerEncounter.TrainingHub().State() == ShadowbladeTrainingHubState::Ready
            && foreignStartCombat.TargetMode() == TrainingTargetMode::Standard
            && !foreignStartCombat.HasPendingEnemyAttack()
            && !foreignStartActions.HasIncomingAttack(),
        "pass25 review training start rejects owners unrelated to encounter completion");
    Expect(startOwnerEncounter.StartTraining(startOwnerCombat, startOwnerActions)
            && startOwnerEncounter.TrainingHub().State() == ShadowbladeTrainingHubState::Active,
        "pass25 review exact completed encounter owners can start training");

    // Independent-review regression: an out-of-range DefenseInput value must fail
    // closed before the practice session can treat it as Guard or consume a threat.
    const ShadowbladeTrainingHubFeedback beforeInvalidDefense =
        startOwnerEncounter.TrainingFeedback(startOwnerCombat);
    const std::uint64_t pendingGenerationBeforeInvalid =
        startOwnerCombat.EnemyAttackGeneration();
    const int playerHealthBeforeInvalid = startOwnerActions.PlayerHealth();
    const DefenseReport invalidDefense = startOwnerEncounter.TryTrainingDefense(
        startOwnerCombat, startOwnerActions, static_cast<DefenseInput>(255));
    const ShadowbladeTrainingHubFeedback afterInvalidDefense =
        startOwnerEncounter.TrainingFeedback(startOwnerCombat);
    Expect(invalidDefense.result == DefenseResult::NoThreat
            && startOwnerCombat.HasPendingEnemyAttack()
            && startOwnerActions.HasIncomingAttack()
            && startOwnerCombat.EnemyAttackGeneration() == pendingGenerationBeforeInvalid
            && startOwnerActions.PlayerHealth() == playerHealthBeforeInvalid
            && afterInvalidDefense.resolvedAttempts == beforeInvalidDefense.resolvedAttempts,
        "pass25 review invalid defense input cannot consume or resolve a training threat");

    // Independent-review regression: distinct damage observations inside one
    // 200 ms storage bucket retain their own activity timestamps. At a later
    // cutoff that falls between those observations, only the still-fresh delta
    // remains even though both activities were persisted in the same 5 Hz sample.
    LandmarkEncounter bucketEncounter;
    CombatSandbox bucketCombat;
    ShadowbladeActions bucketActions;
    Expect(bucketEncounter.TryActivate(
               TrainingDiscovery(bucketActions), bucketCombat).result
            == LandmarkEncounterResult::Activated,
        "pass25 review bucket telemetry fixture activates encounter");
    bucketCombat.ApplyDamage(1000);
    Expect(bucketEncounter.Update(bucketCombat, bucketActions)
            && bucketEncounter.ConfigureTraining(
                ShadowbladeTrainingFocus::QuickCut,
                DefensePracticePace::Learning, 2)
            && bucketEncounter.StartTraining(bucketCombat, bucketActions),
        "pass25 review bucket telemetry fixture starts active practice");
    bucketCombat.AdvanceTime(0.10f);
    Expect(bucketCombat.ApplyDamage(10) == 10,
        "pass25 review bucket fixture records the first in-bucket delta");
    bucketEncounter.Update(bucketCombat, bucketActions);
    bucketCombat.AdvanceTime(0.11f);
    Expect(bucketCombat.ApplyDamage(15) == 15,
        "pass25 review bucket fixture records the second in-bucket delta");
    bucketEncounter.Update(bucketCombat, bucketActions);
    bucketCombat.AdvanceTime(19.94f);
    bucketEncounter.Update(bucketCombat, bucketActions);
    const ShadowbladeTrainingDamageWindow bucketWindow =
        bucketEncounter.TrainingFeedback(bucketCombat).recentDamage;
    Expect(bucketWindow.valid && bucketWindow.damage == 15 && bucketWindow.hits == 1
            && bucketWindow.windowSeconds <= ShadowbladeTrainingHub::DamageWindowSeconds,
        "pass25 review exact cutoff separates old and fresh deltas within one stored sample");
}