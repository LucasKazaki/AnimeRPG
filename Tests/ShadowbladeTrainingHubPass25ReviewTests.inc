void TestShadowbladeTrainingHubPass25ReviewRepairs() {
    using namespace Astral::Scene;

    LandmarkInteractionReport discovery{};
    discovery.result = LandmarkInteractionResult::Discovered;
    discovery.landmark = LandmarkEncounter::EncounterLandmark;

    // Independent-review regression: active timing guidance may only describe the
    // threat owned by the current DefensePracticeSession. If the combat planner is
    // externally reset/replaced while the original Shadowblade threat still exists,
    // the hub must fail closed instead of presenting guidance for the replacement.
    LandmarkEncounter guideEncounter;
    CombatSandbox guideCombat;
    ShadowbladeActions guideActions;
    Expect(guideEncounter.TryActivate(discovery, guideCombat).result
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
    Expect(telemetryEncounter.TryActivate(discovery, telemetryCombat).result
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
    // because the nearest stored sample before the cutoff is older. The hub may
    // conservatively omit up to one sample interval, but it cannot report stale
    // out-of-window damage while claiming a 20-second maximum window.
    LandmarkEncounter cutoffEncounter;
    CombatSandbox cutoffCombat;
    ShadowbladeActions cutoffActions;
    Expect(cutoffEncounter.TryActivate(discovery, cutoffCombat).result
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
}
