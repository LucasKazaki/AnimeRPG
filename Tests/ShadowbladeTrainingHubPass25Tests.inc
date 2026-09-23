void TestShadowbladeTrainingHubPass25() {
    using namespace Astral::Scene;

    // GAME-122: the live Lincoln encounter completion is the only production
    // unlock point. Configuration/start attempts before completion fail closed.
    LandmarkEncounter encounter;
    CombatSandbox combat;
    ShadowbladeActions actions;
    Expect(encounter.TrainingHub().State() == ShadowbladeTrainingHubState::Locked,
        "pass25 training hub starts locked");
    Expect(!encounter.ConfigureTraining(
               ShadowbladeTrainingFocus::BossCycle,
               DefensePracticePace::Expert, 4)
            && !encounter.StartTraining(combat, actions),
        "pass25 training cannot be configured or started before encounter completion");

    LandmarkInteractionReport discovery{};
    discovery.result = LandmarkInteractionResult::Discovered;
    discovery.landmark = LandmarkEncounter::EncounterLandmark;
    Expect(encounter.TryActivate(discovery, combat).result
            == LandmarkEncounterResult::Activated,
        "pass25 fixture activates the live Lincoln encounter");
    combat.ApplyDamage(1000);
    Expect(encounter.Update(combat, actions)
            && encounter.State() == LandmarkEncounterState::Completed
            && encounter.TrainingHub().State() == ShadowbladeTrainingHubState::Ready
            && encounter.TrainingHub().Unlocked(),
        "pass25 completed live encounter unlocks the production training hub");
    Expect(!encounter.Update(combat, actions)
            && encounter.TrainingHub().State() == ShadowbladeTrainingHubState::Ready,
        "pass25 repeated completion updates do not reset the unlocked hub");

    // GAME-123: focus, pace, and bounded attempt target are configurable as one
    // atomic training contract; invalid values preserve the prior selection.
    Expect(encounter.ConfigureTraining(
               ShadowbladeTrainingFocus::BossCycle,
               DefensePracticePace::Expert, 4)
            && encounter.TrainingHub().Focus() == ShadowbladeTrainingFocus::BossCycle
            && encounter.TrainingHub().Pace() == DefensePracticePace::Expert
            && encounter.TrainingHub().TargetAttempts() == 4,
        "pass25 configures a selected coach drill pace and bounded attempt target");
    Expect(!encounter.ConfigureTraining(
               static_cast<ShadowbladeTrainingFocus>(255),
               DefensePracticePace::Standard, 3)
            && !encounter.ConfigureTraining(
                ShadowbladeTrainingFocus::QuickCut,
                static_cast<DefensePracticePace>(255), 3)
            && !encounter.ConfigureTraining(
                ShadowbladeTrainingFocus::QuickCut,
                DefensePracticePace::Standard, 0)
            && !encounter.ConfigureTraining(
                ShadowbladeTrainingFocus::QuickCut,
                DefensePracticePace::Standard,
                ShadowbladeTrainingHub::MaximumAttempts + 1)
            && encounter.TrainingHub().Focus() == ShadowbladeTrainingFocus::BossCycle
            && encounter.TrainingHub().Pace() == DefensePracticePace::Expert
            && encounter.TrainingHub().TargetAttempts() == 4,
        "pass25 invalid training configuration fails closed without partial mutation");

    // GAME-124: starting training switches the existing combat owner to endless
    // practice, preserves persistent Shadowblade loadout state, and queues the
    // selected coach drill through the existing defense-practice bridge.
    const ShadowbladeLoadoutSnapshot loadoutBefore = actions.Loadout().Snapshot();
    Expect(encounter.StartTraining(combat, actions)
            && encounter.TrainingHub().State() == ShadowbladeTrainingHubState::Active
            && combat.TargetMode() == TrainingTargetMode::Endless
            && !combat.Dummy().IsDefeated()
            && combat.HasPendingEnemyAttack()
            && actions.HasIncomingAttack()
            && combat.PendingEnemyAttack().pattern == EnemyAttackPattern::QuickCut
            && encounter.TrainingHub().Session().PracticeSequenceLength() == 3
            && encounter.TrainingHub().Session().Pace() == DefensePracticePace::Expert
            && actions.Loadout().Snapshot().equippedWeapon == loadoutBefore.equippedWeapon,
        "pass25 start routes the selected coach drill through live combat and Shadowblade owners");
    Expect(encounter.Retry(combat, actions).result
            == LandmarkEncounterResult::RetryUnavailable
            && encounter.TrainingHub().Active()
            && actions.HasIncomingAttack(),
        "pass25 encounter retry cannot reset an active training owner pair");

    CombatSandbox unrelatedCombat;
    ShadowbladeActions unrelatedActions;
    const DefenseTrainingStats ownerStatsBefore = encounter.TrainingHub().Session().Stats();
    Expect(encounter.TryTrainingDefense(
               unrelatedCombat, unrelatedActions, DefenseInput::Guard).result
            == DefenseResult::NoThreat
            && !encounter.AdvanceTraining(unrelatedCombat, unrelatedActions, 1.0f)
            && encounter.TrainingHub().Session().Stats().attacksQueued
                == ownerStatsBefore.attacksQueued
            && actions.HasIncomingAttack(),
        "pass25 unrelated combat and action owners cannot drive the live training session");

    // GAME-125: a bounded four-attempt boss-cycle run auto-queues the selected
    // patterns after recovery and transitions into a coach debrief at the exact
    // configured target instead of running forever.
    for (int attempt = 0; attempt < 4; ++attempt) {
        const EnemyAttackPlan plan = combat.PendingEnemyAttack();
        Expect(encounter.AdvanceTraining(
                   combat, actions, plan.windupSeconds - 0.10f),
            "pass25 active training advances the authoritative practice clocks");
        const DefenseInput input = plan.blockable ? DefenseInput::Guard : DefenseInput::Dodge;
        const DefenseReport defense = encounter.TryTrainingDefense(combat, actions, input);
        Expect(defense.result == DefenseResult::PerfectGuard
                || defense.result == DefenseResult::PerfectDodge,
            "pass25 selected live training threat accepts the authoritative perfect defense");

        if (attempt < 3) {
            Expect(encounter.AdvanceTraining(combat, actions, plan.recoverySeconds)
                    && combat.HasPendingEnemyAttack()
                    && actions.HasIncomingAttack(),
                "pass25 live training auto-queues the next drill threat after recovery");
        }
    }
    const ShadowbladeTrainingHubFeedback debrief = encounter.TrainingFeedback(combat);
    Expect(encounter.TrainingHub().State() == ShadowbladeTrainingHubState::Debrief
            && debrief.state == ShadowbladeTrainingHubState::Debrief
            && debrief.resolvedAttempts == 4
            && debrief.targetAttempts == 4
            && debrief.debrief.valid
            && debrief.debrief.resolvedAttempts == 4
            && debrief.debrief.successfulDefenses == 4
            && debrief.debrief.hitsTaken == 0
            && debrief.debrief.accuracyPercent == 100
            && debrief.timingGuideValid,
        "pass25 exact attempt cap produces an authoritative coach debrief and timing guide");
    Expect(!combat.HasPendingEnemyAttack() && !actions.HasIncomingAttack(),
        "pass25 debrief target stops instead of queueing a fifth threat");

    const ShadowbladeTrainingHubFeedback wrongOwnerFeedback =
        encounter.TrainingFeedback(unrelatedCombat);
    Expect(wrongOwnerFeedback.resolvedAttempts == 0
            && !wrongOwnerFeedback.debrief.valid
            && !wrongOwnerFeedback.recentDamage.valid,
        "pass25 mismatched combat owner cannot compose mixed-provenance feedback");

    // GAME-126: pause/resume freezes the practice-owned combat and Shadowblade
    // clocks. It does not consume a threat, take damage, or fabricate progress.
    LandmarkEncounter pauseEncounter;
    CombatSandbox pauseCombat;
    ShadowbladeActions pauseActions;
    Expect(pauseEncounter.TryActivate(discovery, pauseCombat).result
            == LandmarkEncounterResult::Activated,
        "pass25 pause fixture activates encounter");
    pauseCombat.ApplyDamage(1000);
    Expect(pauseEncounter.Update(pauseCombat, pauseActions)
            && pauseEncounter.ConfigureTraining(
                ShadowbladeTrainingFocus::QuickCut,
                DefensePracticePace::Standard, 2)
            && pauseEncounter.StartTraining(pauseCombat, pauseActions),
        "pass25 pause fixture starts QuickCut training");
    const double pausedCombatSeconds = pauseCombat.ElapsedSecondsPrecise();
    const float pausedThreatSeconds = pauseActions.IncomingAttackRemaining();
    const int pausedHealth = pauseActions.PlayerHealth();
    Expect(pauseEncounter.SetTrainingPaused(true)
            && !pauseEncounter.AdvanceTraining(pauseCombat, pauseActions, 15.0f)
            && pauseCombat.ElapsedSecondsPrecise() == pausedCombatSeconds
            && Near(pauseActions.IncomingAttackRemaining(), pausedThreatSeconds)
            && pauseActions.PlayerHealth() == pausedHealth
            && pauseEncounter.TrainingFeedback(pauseCombat).paused,
        "pass25 paused training freezes combat threat and player state");
    Expect(pauseEncounter.SetTrainingPaused(false)
            && pauseEncounter.AdvanceTraining(pauseCombat, pauseActions, 0.10f)
            && pauseCombat.ElapsedSecondsPrecise() > pausedCombatSeconds
            && pauseActions.IncomingAttackRemaining() < pausedThreatSeconds,
        "pass25 resuming training advances the same authoritative threat");

    // QOL-026: a bounded twenty-second rolling damage window gives offensive
    // practice feedback without letting the endless target die. The live
    // LandmarkEncounter update path performs read-only sampling.
    LandmarkEncounter damageEncounter;
    CombatSandbox damageCombat;
    ShadowbladeActions damageActions;
    Expect(damageEncounter.TryActivate(discovery, damageCombat).result
            == LandmarkEncounterResult::Activated,
        "pass25 damage-window fixture activates encounter");
    damageCombat.ApplyDamage(1000);
    Expect(damageEncounter.Update(damageCombat, damageActions)
            && damageEncounter.ConfigureTraining(
                ShadowbladeTrainingFocus::QuickCut,
                DefensePracticePace::Learning, 2)
            && damageEncounter.StartTraining(damageCombat, damageActions),
        "pass25 damage-window fixture starts endless target practice");
    Expect(damageCombat.ApplyDamage(30) == 30,
        "pass25 rolling feedback records first endless-target damage sample");
    damageEncounter.Update(damageCombat, damageActions);
    damageCombat.AdvanceTime(1.0f);
    Expect(damageCombat.ApplyDamage(20) == 20,
        "pass25 rolling feedback records second endless-target damage sample");
    damageEncounter.Update(damageCombat, damageActions);
    const ShadowbladeTrainingDamageWindow earlyWindow =
        damageEncounter.TrainingFeedback(damageCombat).recentDamage;
    Expect(earlyWindow.valid && earlyWindow.damage == 50 && earlyWindow.hits == 2
            && earlyWindow.windowSeconds >= 1.0,
        "pass25 recent-damage feedback totals the current bounded practice window");

    damageCombat.AdvanceTime(21.0f);
    Expect(damageCombat.ApplyDamage(10) == 10,
        "pass25 rolling feedback records post-window endless-target damage");
    damageEncounter.Update(damageCombat, damageActions);
    const ShadowbladeTrainingDamageWindow rolledWindow =
        damageEncounter.TrainingFeedback(damageCombat).recentDamage;
    Expect(rolledWindow.valid && rolledWindow.damage == 10 && rolledWindow.hits == 1
            && rolledWindow.windowSeconds <= ShadowbladeTrainingHub::DamageWindowSeconds,
        "pass25 twenty-second window drops stale damage while retaining recent feedback");
}
