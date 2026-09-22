void TestDefensePracticePass12() {
    using namespace Astral::Scene;

    // GAME-057: a selected practice attack uses the canonical plan without
    // consuming the normal enemy rotation and respects authoritative gates.
    CombatSandbox selectedCombat;
    ShadowbladeActions selectedActions;
    CombatDefenseTraining selectedDrill;
    const std::uint64_t generationBeforeSelected = selectedCombat.EnemyAttackGeneration();
    Expect(selectedDrill.QueueAttackPattern(
            selectedCombat, selectedActions, EnemyAttackPattern::GuardBreaker),
        "selected practice can queue GuardBreaker directly");
    const EnemyAttackPlan selectedGuardBreaker = selectedCombat.PendingEnemyAttack();
    Expect(selectedGuardBreaker.pattern == EnemyAttackPattern::GuardBreaker
            && Near(selectedGuardBreaker.windupSeconds, 0.90f)
            && selectedGuardBreaker.damage == 28
            && selectedGuardBreaker.guardDamage == 55
            && selectedGuardBreaker.blockable
            && Near(selectedGuardBreaker.recoverySeconds, 1.0f)
            && selectedCombat.EnemyAttackGeneration() != generationBeforeSelected,
        "selected GuardBreaker preserves the canonical attack definition and receives a generation");
    const std::uint64_t selectedGeneration = selectedCombat.EnemyAttackGeneration();
    Expect(!selectedDrill.QueueAttackPattern(
            selectedCombat, selectedActions, EnemyAttackPattern::QuickCut)
            && selectedCombat.EnemyAttackGeneration() == selectedGeneration,
        "selected practice rejects a duplicate pending threat without consuming a generation");
    const DefenseReport selectedGuard =
        selectedDrill.TryDefend(selectedCombat, selectedActions, DefenseInput::Guard);
    Expect(selectedGuard.result == DefenseResult::Guarded,
        "selected GuardBreaker resolves through the existing defense bridge");
    const std::uint64_t recoveryGeneration = selectedCombat.EnemyAttackGeneration();
    Expect(!selectedDrill.QueueAttackPattern(
            selectedCombat, selectedActions, EnemyAttackPattern::QuickCut)
            && selectedCombat.EnemyAttackGeneration() == recoveryGeneration,
        "selected practice respects post-attack recovery");
    selectedDrill.AdvanceTime(
        selectedCombat, selectedActions, selectedGuardBreaker.recoverySeconds);
    Expect(selectedDrill.QueueNextAttack(selectedCombat, selectedActions)
            && selectedCombat.PendingEnemyAttack().pattern == EnemyAttackPattern::QuickCut,
        "selected practice does not advance the ordinary Standard enemy rotation");
    const EnemyAttackPlan naturalQuickCut = selectedCombat.PendingEnemyAttack();
    Expect(Near(naturalQuickCut.windupSeconds, 0.55f)
            && naturalQuickCut.damage == 18
            && naturalQuickCut.guardDamage == 20
            && naturalQuickCut.blockable
            && Near(naturalQuickCut.recoverySeconds, 0.85f),
        "ordinary rotation remains on the canonical QuickCut definition after selected practice");
    Expect(selectedDrill.TryDefend(
            selectedCombat, selectedActions, DefenseInput::Guard).result == DefenseResult::Guarded,
        "ordinary attack can resolve after selected practice");
    selectedDrill.AdvanceTime(selectedCombat, selectedActions, naturalQuickCut.recoverySeconds);
    const std::uint64_t generationBeforeInvalid = selectedCombat.EnemyAttackGeneration();
    Expect(!selectedDrill.QueueAttackPattern(selectedCombat, selectedActions,
            static_cast<EnemyAttackPattern>(99))
            && selectedCombat.EnemyAttackGeneration() == generationBeforeInvalid
            && !selectedCombat.HasPendingEnemyAttack(),
        "invalid selected pattern fails closed without mutating planner identity");

    CombatSandbox selectedRiftCombat;
    ShadowbladeActions selectedRiftActions;
    CombatDefenseTraining selectedRiftDrill;
    Expect(selectedRiftDrill.QueueAttackPattern(
            selectedRiftCombat, selectedRiftActions, EnemyAttackPattern::RiftBurst),
        "selected practice can queue RiftBurst directly");
    const EnemyAttackPlan selectedRift = selectedRiftCombat.PendingEnemyAttack();
    Expect(selectedRift.pattern == EnemyAttackPattern::RiftBurst
            && Near(selectedRift.windupSeconds, 0.70f)
            && selectedRift.damage == 34
            && selectedRift.guardDamage == 0
            && !selectedRift.blockable
            && Near(selectedRift.recoverySeconds, 1.10f),
        "selected RiftBurst preserves the canonical unblockable attack definition");

    CombatSandbox staggerGateCombat;
    ShadowbladeActions staggerGateActions;
    CombatDefenseTraining staggerGateDrill;
    staggerGateCombat.TryAttack(AttackType::Light, {});
    staggerGateCombat.AdvanceTime(0.4f);
    const AttackReport staggerGateHeavy = staggerGateCombat.TryAttack(AttackType::Heavy, {});
    Expect(staggerGateHeavy.staggerTriggered
            && !staggerGateDrill.QueueAttackPattern(staggerGateCombat,
                staggerGateActions, EnemyAttackPattern::QuickCut),
        "selected practice respects the existing stagger gate");

    CombatSandbox defeatedGateCombat;
    ShadowbladeActions defeatedGateActions;
    CombatDefenseTraining defeatedGateDrill;
    defeatedGateCombat.ApplyDamage(defeatedGateCombat.Dummy().health);
    Expect(defeatedGateCombat.Dummy().IsDefeated()
            && !defeatedGateDrill.QueueAttackPattern(defeatedGateCombat,
                defeatedGateActions, EnemyAttackPattern::QuickCut),
        "selected practice rejects a defeated training target");

    // GAME-058: bounded drill goals distinguish perfect, guard, and dodge skill.
    CombatSandbox perfectGoalCombat;
    ShadowbladeActions perfectGoalActions;
    CombatDefenseTraining perfectGoalDrill;
    Expect(perfectGoalDrill.SetGoal(DefenseTrainingGoal::PerfectDefense),
        "perfect-defense drill goal can be selected");
    for (int attempt = 0; attempt < CombatDefenseTraining::GoalTarget; ++attempt) {
        Expect(perfectGoalDrill.QueueAttackPattern(perfectGoalCombat,
                perfectGoalActions, EnemyAttackPattern::QuickCut),
            "perfect-defense goal queues a known QuickCut attempt");
        const EnemyAttackPlan plan = perfectGoalCombat.PendingEnemyAttack();
        perfectGoalDrill.AdvanceTime(perfectGoalCombat, perfectGoalActions, 0.45f);
        Expect(perfectGoalDrill.TryDefend(perfectGoalCombat, perfectGoalActions,
                DefenseInput::Dodge).result == DefenseResult::PerfectDodge,
            "perfect-defense goal accepts an existing perfect dodge");
        if (attempt + 1 < CombatDefenseTraining::GoalTarget) {
            perfectGoalDrill.AdvanceTime(
                perfectGoalCombat, perfectGoalActions, plan.recoverySeconds);
        }
    }
    const DefenseTrainingGoalStatus perfectStatus = perfectGoalDrill.GoalStatus();
    Expect(perfectStatus.goal == DefenseTrainingGoal::PerfectDefense
            && perfectStatus.current == CombatDefenseTraining::GoalTarget
            && perfectStatus.target == CombatDefenseTraining::GoalTarget
            && perfectStatus.complete,
        "perfect-defense goal completes only after its bounded target count");

    CombatSandbox disciplineCombat;
    ShadowbladeActions disciplineActions;
    CombatDefenseTraining disciplineDrill;
    Expect(disciplineDrill.SetGoal(DefenseTrainingGoal::GuardDiscipline)
            && disciplineDrill.QueueAttackPattern(disciplineCombat,
                disciplineActions, EnemyAttackPattern::QuickCut)
            && disciplineDrill.TryDefend(disciplineCombat,
                disciplineActions, DefenseInput::Guard).result == DefenseResult::Guarded,
        "guard-discipline goal records an ordinary guard");
    const DefenseTrainingGoalStatus guardStatus = disciplineDrill.GoalStatus();
    Expect(guardStatus.current == 1 && !guardStatus.complete
            && disciplineDrill.Stats().guardDefenses == 1
            && disciplineDrill.Stats().dodgeDefenses == 0,
        "guard-discipline progress is distinct from dodge progress");
    disciplineDrill.AdvanceTime(disciplineCombat, disciplineActions, 0.85f);
    Expect(disciplineDrill.SetGoal(DefenseTrainingGoal::DodgeDiscipline)
            && disciplineDrill.QueueAttackPattern(disciplineCombat,
                disciplineActions, EnemyAttackPattern::QuickCut),
        "dodge-discipline goal can follow guard practice");
    disciplineDrill.AdvanceTime(disciplineCombat, disciplineActions, 0.25f);
    Expect(disciplineDrill.TryDefend(disciplineCombat,
            disciplineActions, DefenseInput::Dodge).result == DefenseResult::Evaded,
        "dodge-discipline goal records an ordinary evade");
    const DefenseTrainingGoalStatus dodgeStatus = disciplineDrill.GoalStatus();
    Expect(dodgeStatus.current == 1 && !dodgeStatus.complete
            && disciplineDrill.Stats().guardDefenses == 1
            && disciplineDrill.Stats().dodgeDefenses == 1,
        "dodge-discipline progress is counted separately from guards");
    const DefenseTrainingGoal previousGoal = disciplineDrill.Goal();
    Expect(!disciplineDrill.SetGoal(static_cast<DefenseTrainingGoal>(99))
            && disciplineDrill.Goal() == previousGoal,
        "invalid goal selection fails closed without changing the active goal");

    // GAME-059 and QOL-013: reminders and non-color semantic symbols derive
    // only from the authoritative linked attack and existing timing windows.
    CombatSandbox reminderCombat;
    ShadowbladeActions reminderActions;
    CombatDefenseTraining reminderDrill;
    Expect(reminderDrill.QueueAttackPattern(reminderCombat,
            reminderActions, EnemyAttackPattern::QuickCut),
        "reminder setup queues a blockable attack");
    DefenseTrainingCue reminderCue = reminderDrill.Cue(reminderCombat, reminderActions);
    Expect(reminderCue.symbol == DefenseTrainingCueSymbol::Blockable
            && reminderDrill.ControlReminder(reminderCombat, reminderActions)
                == DefenseTrainingControlReminder::Guard,
        "blockable practice exposes a semantic symbol and guard reminder without relying on color");
    Expect(reminderDrill.SetGoal(DefenseTrainingGoal::DodgeDiscipline)
            && reminderDrill.ControlReminder(reminderCombat, reminderActions)
                == DefenseTrainingControlReminder::Dodge,
        "dodge-discipline goal can intentionally override a blockable guard reminder");
    reminderDrill.AdvanceTime(reminderCombat, reminderActions, 0.45f);
    reminderCue = reminderDrill.Cue(reminderCombat, reminderActions);
    Expect(reminderCue.phase == DefenseTrainingCuePhase::PerfectWindow
            && reminderCue.symbol == DefenseTrainingCueSymbol::PerfectTiming,
        "perfect timing exposes its own non-color semantic cue symbol");

    const DefenseTrainingCue emptyCue = CombatDefenseTraining{}.Cue(
        CombatSandbox{}, ShadowbladeActions{});
    Expect(emptyCue.phase == DefenseTrainingCuePhase::None
            && emptyCue.symbol == DefenseTrainingCueSymbol::None,
        "no owned threat exposes no misleading semantic cue");

    Expect(selectedRiftDrill.Cue(selectedRiftCombat, selectedRiftActions).symbol
            == DefenseTrainingCueSymbol::Unblockable
            && selectedRiftDrill.ControlReminder(selectedRiftCombat, selectedRiftActions)
                == DefenseTrainingControlReminder::Dodge,
        "unblockable practice exposes a distinct symbol and recommends dodge");

    // GAME-060: repeated mistakes produce bounded training hints, while success
    // clears the mistake streak and clean ordinary defenses suggest tighter timing.
    CombatSandbox hintCombat;
    ShadowbladeActions hintActions;
    CombatDefenseTraining hintDrill;
    for (int hit = 0; hit < 2; ++hit) {
        Expect(hintDrill.QueueAttackPattern(
                hintCombat, hintActions, EnemyAttackPattern::QuickCut),
            "tutorial-hint setup queues a QuickCut hit");
        const EnemyAttackPlan plan = hintCombat.PendingEnemyAttack();
        Expect(hintDrill.AdvanceTime(hintCombat, hintActions, plan.windupSeconds),
            "tutorial-hint setup resolves the queued hit");
        if (hit == 0) {
            Expect(hintDrill.TutorialHint(hintCombat, hintActions)
                    == DefenseTrainingTutorialHint::ReadTelegraph,
                "one consecutive hit suggests rereading the telegraph");
        } else {
            Expect(hintDrill.TutorialHint(hintCombat, hintActions)
                    == DefenseTrainingTutorialHint::GuardBlockable,
                "second missed blockable attack remains actionable during recovery");
        }
        hintDrill.AdvanceTime(hintCombat, hintActions, plan.recoverySeconds);
    }
    Expect(hintDrill.QueueAttackPattern(
            hintCombat, hintActions, EnemyAttackPattern::QuickCut)
            && hintDrill.TutorialHint(hintCombat, hintActions)
                == DefenseTrainingTutorialHint::GuardBlockable,
        "repeated-hit hint remains specialized when the next blockable threat begins");
    Expect(hintDrill.TryDefend(
            hintCombat, hintActions, DefenseInput::Guard).result == DefenseResult::Guarded
            && hintDrill.Stats().consecutiveHits == 0,
        "a successful defense clears the consecutive-hit tutorial trigger");

    CombatSandbox unblockableHintCombat;
    ShadowbladeActions unblockableHintActions;
    CombatDefenseTraining unblockableHintDrill;
    for (int hit = 0; hit < 2; ++hit) {
        Expect(unblockableHintDrill.QueueAttackPattern(unblockableHintCombat,
                unblockableHintActions, EnemyAttackPattern::RiftBurst),
            "unblockable-hint setup queues a known RiftBurst hit");
        const EnemyAttackPlan plan = unblockableHintCombat.PendingEnemyAttack();
        Expect(unblockableHintDrill.AdvanceTime(
                unblockableHintCombat, unblockableHintActions, plan.windupSeconds),
            "unblockable-hint setup resolves the known RiftBurst hit");
        if (hit == 1) {
            Expect(unblockableHintDrill.TutorialHint(
                    unblockableHintCombat, unblockableHintActions)
                    == DefenseTrainingTutorialHint::DodgeUnblockable,
                "second missed unblockable attack remains actionable during recovery");
        }
        if (hit == 0) {
            unblockableHintDrill.AdvanceTime(
                unblockableHintCombat, unblockableHintActions, plan.recoverySeconds);
        }
    }

    CombatSandbox timingHintCombat;
    ShadowbladeActions timingHintActions;
    CombatDefenseTraining timingHintDrill;
    Expect(timingHintDrill.QueueAttackPattern(
            timingHintCombat, timingHintActions, EnemyAttackPattern::QuickCut),
        "timing-hint setup queues one earlier perfect defense");
    timingHintDrill.AdvanceTime(timingHintCombat, timingHintActions, 0.45f);
    Expect(timingHintDrill.TryDefend(timingHintCombat,
            timingHintActions, DefenseInput::Dodge).result == DefenseResult::PerfectDodge,
        "timing-hint setup records an earlier lifetime perfect defense");
    timingHintDrill.AdvanceTime(timingHintCombat, timingHintActions, 0.85f);
    for (int defense = 0; defense < CombatDefenseTraining::GoalTarget; ++defense) {
        Expect(timingHintDrill.QueueAttackPattern(
                timingHintCombat, timingHintActions, EnemyAttackPattern::QuickCut)
                && timingHintDrill.TryDefend(timingHintCombat,
                    timingHintActions, DefenseInput::Guard).result == DefenseResult::Guarded,
            "timing-hint setup records a clean ordinary guard");
        if (defense + 1 < CombatDefenseTraining::GoalTarget) {
            timingHintDrill.AdvanceTime(timingHintCombat, timingHintActions, 0.85f);
        }
    }
    Expect(timingHintDrill.Stats().perfectDefenses == 1
            && timingHintDrill.Stats().consecutiveOrdinaryDefenses
                == CombatDefenseTraining::GoalTarget
            && timingHintDrill.TutorialHint(timingHintCombat, timingHintActions)
                == DefenseTrainingTutorialHint::AimPerfectTiming,
        "three consecutive ordinary defenses suggest tighter timing even after an earlier perfect");

    // GAME-061: pausing freezes the linked gameplay clocks and input mutation,
    // then resumes the exact same threat once.
    CombatSandbox pauseCombat;
    ShadowbladeActions pauseActions;
    CombatDefenseTraining pauseDrill;
    Expect(pauseActions.TryDash({}).result == ShadowActionResult::Activated
            && pauseDrill.QueueAttackPattern(
                pauseCombat, pauseActions, EnemyAttackPattern::QuickCut),
        "pause setup starts an unrelated cooldown and one linked QuickCut");
    const EnemyAttackPlan pausePlan = pauseCombat.PendingEnemyAttack();
    const float remainingBeforePause = pauseActions.IncomingAttackRemaining();
    const float dashCooldownBeforePause = pauseActions.DashCooldownRemaining();
    const float resourceBeforePause = pauseActions.Resource();
    const double combatTimeBeforePause = pauseCombat.ElapsedSecondsPrecise();
    const std::uint64_t plannerGenerationBeforePause = pauseCombat.EnemyAttackGeneration();
    const std::uint64_t actionGenerationBeforePause = pauseActions.IncomingAttackGeneration();
    const DefenseTrainingStats statsBeforePause = pauseDrill.Stats();
    Expect(pauseDrill.SetPaused(true) && pauseDrill.Paused(),
        "defense practice can enter a paused state");
    Expect(!pauseDrill.QueueAttackPattern(
            pauseCombat, pauseActions, EnemyAttackPattern::RiftBurst),
        "paused practice does not queue replacement threats");
    const DefenseReport pausedDefense =
        pauseDrill.TryDefend(pauseCombat, pauseActions, DefenseInput::Guard);
    Expect(pausedDefense.result == DefenseResult::Paused,
        "paused practice rejects defense input without resolving the threat");
    CombatSandbox unrelatedPauseCombat;
    ShadowbladeActions unrelatedPauseActions;
    Expect(pauseDrill.TryDefend(unrelatedPauseCombat,
            unrelatedPauseActions, DefenseInput::Guard).result == DefenseResult::NoThreat,
        "paused coordinator preserves linked-object ownership for unrelated callers");
    Expect(!pauseDrill.AdvanceTime(pauseCombat, pauseActions, 100.0f),
        "paused practice consumes no simulation time even for a very large delta");
    Expect(pauseCombat.HasPendingEnemyAttack() && pauseActions.HasIncomingAttack()
            && Near(pauseActions.IncomingAttackRemaining(), remainingBeforePause)
            && Near(pauseActions.DashCooldownRemaining(), dashCooldownBeforePause)
            && Near(pauseActions.Resource(), resourceBeforePause)
            && pauseCombat.ElapsedSecondsPrecise() == combatTimeBeforePause
            && pauseCombat.EnemyAttackGeneration() == plannerGenerationBeforePause
            && pauseActions.IncomingAttackGeneration() == actionGenerationBeforePause
            && pauseActions.PlayerHealth() == ShadowbladeActions::MaximumPlayerHealth
            && pauseDrill.Stats().defenseInputs == statsBeforePause.defenseInputs
            && pauseDrill.Stats().attacksQueued == statsBeforePause.attacksQueued,
        "pause preserves threat identity timing cooldown resource health and telemetry exactly");
    Expect(pauseDrill.Cue(pauseCombat, pauseActions).symbol
            == DefenseTrainingCueSymbol::Blockable,
        "paused practice preserves the current readable cue for inspection");
    Expect(pauseDrill.SetPaused(false) && !pauseDrill.Paused(),
        "defense practice can resume from pause");
    Expect(pauseDrill.AdvanceTime(
            pauseCombat, pauseActions, pausePlan.windupSeconds)
            && !pauseCombat.HasPendingEnemyAttack()
            && !pauseActions.HasIncomingAttack()
            && pauseActions.PlayerHealth() == 82
            && pauseDrill.Stats().hitsTaken == 1,
        "resumed practice continues the preserved threat and resolves it exactly once");
}
