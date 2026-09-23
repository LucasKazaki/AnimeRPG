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
    const ShadowbladeWeapon weaponBefore = actions.Loadout().EquippedWeapon();
    Expect(encounter.StartTraining(combat, actions)
            && encounter.TrainingHub().State() == ShadowbladeTrainingHubState::Active
            && combat.TargetMode() == TrainingTargetMode::Endless
            && !combat.Dummy().IsDefeated()
            && combat.HasPendingEnemyAttack()
            && actions.HasIncomingAttack()
            && combat.PendingEnemyAttack().pattern == EnemyAttackPattern::QuickCut
            && encounter.TrainingHub().Session().PracticeSequenceLength() == 3
            && encounter.TrainingHub().Session().Pace() == DefensePracticePace::Expert
            && actions.Loadout().EquippedWeapon() == weaponBefore,
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
        const float beforeWindup = actions.IncomingAttackRemaining();
        encounter.AdvanceTraining(combat, actions, plan.windupSeconds - 0.10f);
        Expect(actions.IncomingAttackRemaining() < beforeWindup,
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

    // Review regression: leaving a completed training run for encounter retry
    // must restore the finite target. Endless practice mode otherwise makes the
    // retried landmark target impossible to defeat and strands the encounter.
    Expect(encounter.Retry(combat, actions).result == LandmarkEncounterResult::Retried
            && combat.TargetMode() == TrainingTargetMode::Standard
            && !combat.Dummy().IsDefeated(),
        "pass25 encounter retry restores the standard finite training target");
    Expect(combat.ApplyDamage(1000) > 0 && combat.Dummy().IsDefeated(),
        "pass25 retried landmark target remains defeatable after leaving training");
    Expect(encounter.Update(combat, actions)
            && encounter.State() == LandmarkEncounterState::Completed,
        "pass25 retried landmark encounter can complete after training debrief");

    // Review regression: a run that defeats the player before the requested
    // attempt cap must end in Debrief rather than becoming permanently Active
    // with no queueable threat. The same owner pair can then be configured and
    // restarted, which restores transient player state through the existing API.
    LandmarkEncounter defeatEncounter;
    CombatSandbox defeatCombat;
    ShadowbladeActions defeatActions;
    Expect(defeatEncounter.TryActivate(discovery, defeatCombat).result
            == LandmarkEncounterResult::Activated,
        "pass25 defeat fixture activates encounter");
    defeatCombat.ApplyDamage(1000);
    Expect(defeatEncounter.Update(defeatCombat, defeatActions)
            && defeatEncounter.ConfigureTraining(
                ShadowbladeTrainingFocus::GuardBreaker,
                DefensePracticePace::Expert, ShadowbladeTrainingHub::MaximumAttempts)
            && defeatEncounter.StartTraining(defeatCombat, defeatActions),
        "pass25 defeat fixture starts a ten-attempt GuardBreaker run");
    while (defeatEncounter.TrainingHub().Active() && defeatActions.PlayerHealth() > 0) {
        const EnemyAttackPlan plan = defeatCombat.PendingEnemyAttack();
        Expect(defeatEncounter.AdvanceTraining(
                   defeatCombat, defeatActions, plan.windupSeconds + 0.01f),
            "pass25 missed practice threat resolves through authoritative time advancement");
        if (defeatEncounter.TrainingHub().Active()
            && !defeatCombat.HasPendingEnemyAttack()
            && !defeatActions.HasIncomingAttack()) {
            defeatEncounter.AdvanceTraining(
                defeatCombat, defeatActions, plan.recoverySeconds + 0.01f);
        }
    }
    const ShadowbladeTrainingHubFeedback defeatDebrief =
        defeatEncounter.TrainingFeedback(defeatCombat);
    Expect(defeatActions.PlayerHealth() == 0
            && defeatEncounter.TrainingHub().State() == ShadowbladeTrainingHubState::Debrief
            && defeatDebrief.resolvedAttempts > 0
            && defeatDebrief.resolvedAttempts < ShadowbladeTrainingHub::MaximumAttempts
            && defeatDebrief.debrief.valid
            && defeatDebrief.debrief.hitsTaken == defeatDebrief.resolvedAttempts
            && !defeatCombat.HasPendingEnemyAttack()
            && !defeatActions.HasIncomingAttack(),
        "pass25 player defeat closes a bounded run instead of trapping it active");
    Expect(defeatEncounter.ConfigureTraining(
               ShadowbladeTrainingFocus::QuickCut,
               DefensePracticePace::Learning, 1)
            && defeatEncounter.StartTraining(defeatCombat, defeatActions)
            && defeatActions.PlayerHealth() == ShadowbladeActions::MaximumPlayerHealth,
        "pass25 a defeated run can be reconfigured and restarted on the same owners");

    // Review regression: timing guidance must mirror the timing preset on the
    // exact bound ShadowbladeActions owner, including supported post-start assist
    // changes rather than a preset snapshot captured when the run began.
    LandmarkEncounter timingEncounter;
    CombatSandbox timingCombat;
    ShadowbladeActions timingActions;
    timingActions.SetDefenseTimingPreset(DefenseTimingPreset::Forgiving);
    Expect(timingEncounter.TryActivate(discovery, timingCombat).result
            == LandmarkEncounterResult::Activated,
        "pass25 timing-guide fixture activates encounter");
    timingCombat.ApplyDamage(1000);
    Expect(timingEncounter.Update(timingCombat, timingActions)
            && timingEncounter.ConfigureTraining(
                ShadowbladeTrainingFocus::QuickCut,
                DefensePracticePace::Standard, 1)
            && timingEncounter.StartTraining(timingCombat, timingActions),
        "pass25 timing-guide fixture starts with forgiving defense timing preserved");
    const ShadowbladeTrainingHubFeedback timingFeedback =
        timingEncounter.TrainingFeedback(timingCombat);
    Expect(timingFeedback.timingGuideValid
            && Near(timingFeedback.timingGuide.selectedPerfectWindowSeconds,
                ShadowbladeActions::ForgivingPerfectDefenseWindowSeconds)
            && Near(timingActions.PerfectDefenseWindowSeconds(),
                ShadowbladeActions::ForgivingPerfectDefenseWindowSeconds),
        "pass25 timing guide matches the exact bound action owner's forgiving window");
    Expect(timingActions.SetDefenseTimingPreset(DefenseTimingPreset::Standard),
        "pass25 timing fixture supports a post-start timing assist change");
    const ShadowbladeTrainingHubFeedback changedTimingFeedback =
        timingEncounter.TrainingFeedback(timingCombat);
    Expect(changedTimingFeedback.timingGuideValid
            && Near(changedTimingFeedback.timingGuide.selectedPerfectWindowSeconds,
                ShadowbladeActions::StandardPerfectDefenseWindowSeconds)
            && Near(timingActions.PerfectDefenseWindowSeconds(),
                ShadowbladeActions::StandardPerfectDefenseWindowSeconds),
        "pass25 timing guide reads the current bound action owner after start");

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
    Expect(pauseEncounter.SetTrainingPaused(false),
        "pass25 pause fixture resumes the same training session");
    pauseEncounter.AdvanceTraining(pauseCombat, pauseActions, 0.10f);
    Expect(pauseCombat.ElapsedSecondsPrecise() > pausedCombatSeconds
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
