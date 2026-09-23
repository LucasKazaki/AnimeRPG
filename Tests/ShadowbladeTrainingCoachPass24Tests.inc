void TestShadowbladeTrainingCoachPass24() {
    using namespace Astral::Scene;

    // GAME-117: every advertised focus has an exact bounded mapping to the
    // existing canonical threat definitions, goals, and selected pace.
    ShadowbladeTrainingDrillPlan quickPlan{};
    Expect(ShadowbladeTrainingCoach::PlanForFocus(
               ShadowbladeTrainingFocus::QuickCut,
               DefensePracticePace::Learning, quickPlan)
            && quickPlan.sequence.count == 1
            && quickPlan.sequence.patterns[0] == EnemyAttackPattern::QuickCut
            && quickPlan.pace == DefensePracticePace::Learning
            && quickPlan.goal == DefenseTrainingGoal::GuardDiscipline
            && quickPlan.goalTarget == DefensePracticeSession::DefaultGoalTarget,
        "pass24 QuickCut focus maps exactly to the canonical guard drill");

    ShadowbladeTrainingDrillPlan breakerPlan{};
    Expect(ShadowbladeTrainingCoach::PlanForFocus(
               ShadowbladeTrainingFocus::GuardBreaker,
               DefensePracticePace::Standard, breakerPlan)
            && breakerPlan.sequence.count == 1
            && breakerPlan.sequence.patterns[0] == EnemyAttackPattern::GuardBreaker
            && breakerPlan.pace == DefensePracticePace::Standard
            && breakerPlan.goal == DefenseTrainingGoal::GuardDiscipline
            && breakerPlan.goalTarget == DefensePracticeSession::DefaultGoalTarget,
        "pass24 GuardBreaker focus maps exactly to the canonical guard drill");

    ShadowbladeTrainingDrillPlan riftPlan{};
    Expect(ShadowbladeTrainingCoach::PlanForFocus(
               ShadowbladeTrainingFocus::RiftBurst,
               DefensePracticePace::Expert, riftPlan)
            && riftPlan.sequence.count == 1
            && riftPlan.sequence.patterns[0] == EnemyAttackPattern::RiftBurst
            && riftPlan.pace == DefensePracticePace::Expert
            && riftPlan.goal == DefenseTrainingGoal::DodgeDiscipline
            && riftPlan.goalTarget == DefensePracticeSession::DefaultGoalTarget,
        "pass24 RiftBurst focus maps exactly to the canonical dodge drill");

    ShadowbladeTrainingDrillPlan mixedPlan{};
    Expect(ShadowbladeTrainingCoach::PlanForFocus(
               ShadowbladeTrainingFocus::MixedDefense,
               DefensePracticePace::Standard, mixedPlan)
            && mixedPlan.sequence.count == 2
            && mixedPlan.sequence.patterns[0] == EnemyAttackPattern::QuickCut
            && mixedPlan.sequence.patterns[1] == EnemyAttackPattern::RiftBurst
            && mixedPlan.pace == DefensePracticePace::Standard
            && mixedPlan.goal == DefenseTrainingGoal::PerfectDefense
            && mixedPlan.goalTarget == DefensePracticeSession::DefaultGoalTarget,
        "pass24 MixedDefense focus maps exactly to QuickCut then RiftBurst");

    ShadowbladeTrainingDrillPlan bossPlan{};
    Expect(ShadowbladeTrainingCoach::PlanForFocus(
               ShadowbladeTrainingFocus::BossCycle,
               DefensePracticePace::Expert, bossPlan)
            && bossPlan.sequence.count == 3
            && bossPlan.sequence.patterns[0] == EnemyAttackPattern::QuickCut
            && bossPlan.sequence.patterns[1] == EnemyAttackPattern::GuardBreaker
            && bossPlan.sequence.patterns[2] == EnemyAttackPattern::RiftBurst
            && bossPlan.pace == DefensePracticePace::Expert
            && bossPlan.goal == DefenseTrainingGoal::FreePractice
            && bossPlan.goalTarget == DefensePracticeSession::DefaultGoalTarget,
        "pass24 BossCycle focus maps exactly to all three canonical threat types");

    const ShadowbladeTrainingDrillPlan unchangedPlan = bossPlan;
    Expect(!ShadowbladeTrainingCoach::PlanForFocus(
               static_cast<ShadowbladeTrainingFocus>(255),
               DefensePracticePace::Standard, bossPlan)
            && bossPlan.sequence.count == unchangedPlan.sequence.count
            && bossPlan.focus == unchangedPlan.focus,
        "pass24 invalid focus fails closed without overwriting the prior plan");
    Expect(!ShadowbladeTrainingCoach::PlanForFocus(
               ShadowbladeTrainingFocus::MixedDefense,
               static_cast<DefensePracticePace>(255), bossPlan)
            && bossPlan.sequence.count == unchangedPlan.sequence.count,
        "pass24 invalid pace fails closed without overwriting the prior plan");

    DefensePracticeSession focusedSession;
    ShadowbladeActions focusedActions;
    CombatSandbox focusedCombat;
    Expect(ShadowbladeTrainingCoach::ApplyDrill(
               focusedSession, focusedCombat, focusedActions,
               ShadowbladeTrainingFocus::RiftBurst,
               DefensePracticePace::Learning)
            && focusedSession.PracticeSequenceLength() == 1
            && focusedSession.Pace() == DefensePracticePace::Learning
            && focusedSession.Goal() == DefenseTrainingGoal::DodgeDiscipline
            && focusedSession.QueueNextAttack(focusedCombat, focusedActions)
            && focusedCombat.PendingEnemyAttack().pattern == EnemyAttackPattern::RiftBurst,
        "pass24 focused drill config reaches the existing live threat path");
    const DefenseTrainingStats focusedBefore = focusedSession.Stats();
    Expect(!ShadowbladeTrainingCoach::ApplyDrill(
               focusedSession, focusedCombat, focusedActions,
               ShadowbladeTrainingFocus::QuickCut,
               DefensePracticePace::Standard)
            && focusedActions.HasIncomingAttack()
            && focusedSession.Stats().attacksQueued == focusedBefore.attacksQueued
            && focusedSession.PracticeSequenceLength() == 1
            && focusedSession.Goal() == DefenseTrainingGoal::DodgeDiscipline,
        "pass24 drill reconfiguration is atomic while a session threat is active");
    const EnemyAttackPlan focusedThreat = focusedCombat.PendingEnemyAttack();
    focusedSession.AdvanceTime(focusedCombat, focusedActions,
        focusedThreat.windupSeconds - 0.10f);
    Expect(focusedSession.TryDefend(focusedCombat, focusedActions,
               DefenseInput::Dodge).result == DefenseResult::PerfectDodge,
        "pass24 focused RiftBurst drill resolves through authoritative defense timing");
    focusedSession.AdvanceTime(
        focusedCombat, focusedActions, focusedThreat.recoverySeconds);

    // Review regression: threats queued directly on either authoritative owner
    // must block reconfiguration even though the session itself has no link.
    DefensePracticeSession detachedActionSession;
    ShadowbladeActions detachedActions;
    CombatSandbox detachedActionCombat;
    Expect(detachedActions.BeginIncomingAttack({0.8f, 20, 10, true}),
        "pass24 detached-action regression creates a live authoritative threat");
    Expect(!ShadowbladeTrainingCoach::ApplyDrill(
               detachedActionSession, detachedActionCombat, detachedActions,
               ShadowbladeTrainingFocus::QuickCut, DefensePracticePace::Standard)
            && detachedActions.HasIncomingAttack()
            && detachedActionSession.PracticeSequenceLength() == 0
            && detachedActionSession.Stats().attacksQueued == 0,
        "pass24 detached action threat blocks drill mutation");

    DefensePracticeSession detachedCombatSession;
    ShadowbladeActions detachedCombatActions;
    CombatSandbox detachedCombat;
    Expect(detachedCombat.QueueEnemyAttackPattern(EnemyAttackPattern::GuardBreaker),
        "pass24 detached-combat regression creates a live planner threat");
    Expect(!ShadowbladeTrainingCoach::ApplyDrill(
               detachedCombatSession, detachedCombat, detachedCombatActions,
               ShadowbladeTrainingFocus::RiftBurst, DefensePracticePace::Learning)
            && detachedCombat.HasPendingEnemyAttack()
            && detachedCombatSession.PracticeSequenceLength() == 0,
        "pass24 detached combat threat blocks drill mutation");

    // GAME-118 and GAME-119: run a bounded five-attempt practice sample so the
    // debrief and optional challenges are derived from real session metrics.
    DefensePracticeSession masterySession;
    ShadowbladeActions masteryActions;
    CombatSandbox masteryCombat;
    Expect(ShadowbladeTrainingCoach::ApplyDrill(
               masterySession, masteryCombat, masteryActions,
               ShadowbladeTrainingFocus::BossCycle,
               DefensePracticePace::Standard),
        "pass24 mastery sample configures the three-pattern boss cycle");
    for (int attempt = 0; attempt < 5; ++attempt) {
        Expect(masterySession.QueueNextAttack(masteryCombat, masteryActions),
            "pass24 mastery sample queues the next canonical threat");
        const EnemyAttackPlan plan = masteryCombat.PendingEnemyAttack();
        masterySession.AdvanceTime(
            masteryCombat, masteryActions, plan.windupSeconds - 0.10f);
        const DefenseInput input = plan.blockable ? DefenseInput::Guard : DefenseInput::Dodge;
        const DefenseResult result = masterySession.TryDefend(
            masteryCombat, masteryActions, input).result;
        Expect(result == DefenseResult::PerfectGuard
                || result == DefenseResult::PerfectDodge,
            "pass24 mastery sample records an authoritative perfect defense");
        masterySession.AdvanceTime(
            masteryCombat, masteryActions, plan.recoverySeconds);
    }

    const ShadowbladeTrainingDebrief masteryDebrief =
        ShadowbladeTrainingCoach::Debrief(masterySession, masteryCombat);
    Expect(masteryDebrief.valid
            && masteryDebrief.resolvedAttempts == 5
            && masteryDebrief.successfulDefenses == 5
            && masteryDebrief.perfectDefenses == 5
            && masteryDebrief.ordinaryDefenses == 0
            && masteryDebrief.hitsTaken == 0
            && masteryDebrief.damageTaken == 0
            && masteryDebrief.accuracyPercent == 100
            && masteryDebrief.perfectPercent == 100
            && masteryDebrief.bestPerfectStreak == 5
            && masteryDebrief.baseScore > 0
            && masteryDebrief.finalScore > 0
            && masteryDebrief.timeCoefficientPercent > 0
            && masteryDebrief.grade == DefenseTrainingGrade::Gold,
        "pass24 scored debrief exposes bounded authoritative session results");

    // Review regression: score ownership is the combat/session provenance
    // witness. A mismatched combat object fails the complete debrief closed.
    CombatSandbox unrelatedCombat;
    const ShadowbladeTrainingDebrief mismatchedDebrief =
        ShadowbladeTrainingCoach::Debrief(masterySession, unrelatedCombat);
    Expect(!mismatchedDebrief.valid
            && mismatchedDebrief.resolvedAttempts == 0
            && mismatchedDebrief.successfulDefenses == 0
            && mismatchedDebrief.baseScore == 0
            && mismatchedDebrief.finalScore == 0
            && mismatchedDebrief.grade == DefenseTrainingGrade::None,
        "pass24 mismatched combat owner cannot produce a mixed-provenance debrief");

    const ShadowbladeTrainingChallengeStatus streak =
        ShadowbladeTrainingCoach::ChallengeStatus(
            masterySession, ShadowbladeTrainingChallenge::PerfectStreak);
    const ShadowbladeTrainingChallengeStatus noHit =
        ShadowbladeTrainingCoach::ChallengeStatus(
            masterySession, ShadowbladeTrainingChallenge::NoHitSequence);
    const ShadowbladeTrainingChallengeStatus patterns =
        ShadowbladeTrainingCoach::ChallengeStatus(
            masterySession, ShadowbladeTrainingChallenge::PatternMastery);
    Expect(streak.complete && streak.current == 3 && streak.target == 3
            && noHit.complete && noHit.current == 5 && noHit.target == 5
            && patterns.complete && patterns.current == 3 && patterns.target == 3,
        "pass24 optional streak no-hit and pattern-mastery challenges complete independently");
    const ShadowbladeTrainingChallengeStatus invalidChallenge =
        ShadowbladeTrainingCoach::ChallengeStatus(
            masterySession, static_cast<ShadowbladeTrainingChallenge>(255));
    Expect(!invalidChallenge.complete && invalidChallenge.current == 0
            && invalidChallenge.target == 0,
        "pass24 invalid challenge fails closed without inventing completion");

    // GAME-120: coaching recommends the worst measured threat first, then timing
    // precision when the player is safe but consistently defending early.
    DefensePracticeSession missSession;
    ShadowbladeActions missActions;
    CombatSandbox missCombat;
    Expect(ShadowbladeTrainingCoach::ApplyDrill(
               missSession, missCombat, missActions,
               ShadowbladeTrainingFocus::BossCycle,
               DefensePracticePace::Standard)
            && missSession.QueueNextAttack(missCombat, missActions),
        "pass24 recommendation sample starts with canonical QuickCut");
    const EnemyAttackPlan missedQuickCut = missCombat.PendingEnemyAttack();
    missSession.AdvanceTime(
        missCombat, missActions, missedQuickCut.windupSeconds + 0.01f);
    missSession.AdvanceTime(
        missCombat, missActions, missedQuickCut.recoverySeconds);
    Expect(ShadowbladeTrainingCoach::Recommendation(missSession)
            == ShadowbladeTrainingRecommendation::PracticeQuickCut,
        "pass24 measured QuickCut failure recommends focused QuickCut practice");

    DefensePracticeSession ordinarySession;
    ShadowbladeActions ordinaryActions;
    CombatSandbox ordinaryCombat;
    Expect(ShadowbladeTrainingCoach::ApplyDrill(
               ordinarySession, ordinaryCombat, ordinaryActions,
               ShadowbladeTrainingFocus::QuickCut,
               DefensePracticePace::Standard)
            && ordinarySession.QueueNextAttack(ordinaryCombat, ordinaryActions)
            && ordinarySession.TryDefend(ordinaryCombat, ordinaryActions,
                DefenseInput::Guard).result == DefenseResult::Guarded,
        "pass24 recommendation sample records one clean ordinary guard");
    const EnemyAttackPlan ordinaryPlan = ordinaryCombat.PendingEnemyAttack();
    ordinarySession.AdvanceTime(
        ordinaryCombat, ordinaryActions, ordinaryPlan.recoverySeconds);
    Expect(ShadowbladeTrainingCoach::Recommendation(ordinarySession)
            == ShadowbladeTrainingRecommendation::ImprovePerfectTiming,
        "pass24 clean ordinary defenses recommend improving perfect timing");
    Expect(ShadowbladeTrainingCoach::Recommendation(masterySession)
            == ShadowbladeTrainingRecommendation::MaintainForm,
        "pass24 all-perfect measured practice recommends maintaining form");

    // GAME-121: retry clears results/cursor while preserving the selected drill
    // contract, and refuses threats visible on either authoritative live owner.
    DefensePracticeSession retrySession;
    ShadowbladeActions retryActions;
    CombatSandbox retryCombat;
    Expect(ShadowbladeTrainingCoach::ApplyDrill(
               retrySession, retryCombat, retryActions,
               ShadowbladeTrainingFocus::MixedDefense,
               DefensePracticePace::Expert)
            && retrySession.QueueNextAttack(retryCombat, retryActions)
            && retrySession.TryDefend(retryCombat, retryActions,
                DefenseInput::Guard).result == DefenseResult::Guarded,
        "pass24 retry sample records an ordinary result before reset");
    EnemyAttackPlan retryPlan = retryCombat.PendingEnemyAttack();
    retrySession.AdvanceTime(retryCombat, retryActions, retryPlan.recoverySeconds);
    Expect(ShadowbladeTrainingCoach::RetryDrill(
               retrySession, retryCombat, retryActions)
            && retrySession.Stats().attacksQueued == 0
            && retrySession.Stats().ordinaryDefenses == 0
            && retrySession.PracticeSequenceLength() == 2
            && retrySession.PracticeSequenceCursor() == 0
            && retrySession.Pace() == DefensePracticePace::Expert
            && retrySession.Goal() == DefenseTrainingGoal::PerfectDefense
            && retrySession.GoalTarget() == DefensePracticeSession::DefaultGoalTarget,
        "pass24 quick retry preserves drill configuration while clearing session results");
    Expect(retrySession.QueueNextAttack(retryCombat, retryActions),
        "pass24 retried drill restarts its canonical sequence");
    const DefenseTrainingStats activeRetryBefore = retrySession.Stats();
    Expect(!ShadowbladeTrainingCoach::RetryDrill(
               retrySession, retryCombat, retryActions)
            && retryActions.HasIncomingAttack()
            && retrySession.Stats().attacksQueued == activeRetryBefore.attacksQueued
            && retrySession.PracticeSequenceLength() == 2,
        "pass24 quick retry fails closed while an owned threat is active");

    // Review regression: after the session-owned threat is fully resolved, a
    // replacement queued directly on the actions owner still blocks reset.
    retryPlan = retryCombat.PendingEnemyAttack();
    retrySession.AdvanceTime(retryCombat, retryActions,
        retryPlan.windupSeconds - 0.10f);
    const DefenseInput retryInput = retryPlan.blockable
        ? DefenseInput::Guard
        : DefenseInput::Dodge;
    retrySession.TryDefend(retryCombat, retryActions, retryInput);
    retrySession.AdvanceTime(retryCombat, retryActions, retryPlan.recoverySeconds);
    const DefenseTrainingStats replacementBefore = retrySession.Stats();
    Expect(retryActions.BeginIncomingAttack({0.6f, 19, 12, true}),
        "pass24 replacement regression creates a direct action threat after session resolution");
    Expect(!ShadowbladeTrainingCoach::RetryDrill(
               retrySession, retryCombat, retryActions)
            && retryActions.HasIncomingAttack()
            && retrySession.Stats().attacksQueued == replacementBefore.attacksQueued
            && retrySession.Stats().perfectDefenses == replacementBefore.perfectDefenses
            && retrySession.Stats().ordinaryDefenses == replacementBefore.ordinaryDefenses,
        "pass24 direct replacement threat prevents result reset");

    // Review regression: the owner checks themselves must be bound to the
    // session's recorded pair. An unrelated idle pair cannot hide a live
    // replacement on the real pair from either reconfiguration or retry.
    CombatSandbox unrelatedIdleCombat;
    ShadowbladeActions unrelatedIdleActions;
    Expect(!retrySession.AcceptsPracticeObjects(
               unrelatedIdleCombat, unrelatedIdleActions),
        "pass24 bound practice session rejects an unrelated idle owner pair");
    Expect(!ShadowbladeTrainingCoach::ApplyDrill(
               retrySession, unrelatedIdleCombat, unrelatedIdleActions,
               ShadowbladeTrainingFocus::QuickCut, DefensePracticePace::Standard)
            && retryActions.HasIncomingAttack()
            && retrySession.Stats().attacksQueued == replacementBefore.attacksQueued
            && retrySession.PracticeSequenceLength() == 2
            && retrySession.Pace() == DefensePracticePace::Expert,
        "pass24 unrelated idle owners cannot bypass live replacement protection during reconfiguration");
    Expect(!ShadowbladeTrainingCoach::RetryDrill(
               retrySession, unrelatedIdleCombat, unrelatedIdleActions)
            && retryActions.HasIncomingAttack()
            && retrySession.Stats().attacksQueued == replacementBefore.attacksQueued
            && retrySession.Stats().perfectDefenses == replacementBefore.perfectDefenses
            && retrySession.Stats().ordinaryDefenses == replacementBefore.ordinaryDefenses,
        "pass24 unrelated idle owners cannot bypass live replacement protection during retry");

    // QOL-025: the requested timing guide is a read-only view over the same
    // production threat definitions and existing defense-window constants.
    ShadowbladeTrainingTimingGuide quickGuide{};
    Expect(ShadowbladeTrainingCoach::TimingGuide(
               EnemyAttackPattern::QuickCut, DefenseTimingPreset::Standard, quickGuide)
            && quickGuide.pattern == EnemyAttackPattern::QuickCut
            && Near(quickGuide.windupSeconds, 0.55f)
            && quickGuide.damage == 18
            && quickGuide.guardDamage == 20
            && quickGuide.blockable
            && quickGuide.recommendedInput == DefenseInput::Guard
            && Near(quickGuide.selectedPerfectWindowSeconds,
                ShadowbladeActions::StandardPerfectDefenseWindowSeconds)
            && Near(quickGuide.forgivingPerfectWindowSeconds,
                ShadowbladeActions::ForgivingPerfectDefenseWindowSeconds)
            && Near(quickGuide.dodgeWindowSeconds,
                ShadowbladeActions::DodgeWindowSeconds),
        "pass24 timing guide exposes authoritative QuickCut telegraph and defense windows");

    ShadowbladeTrainingTimingGuide riftGuide{};
    Expect(ShadowbladeTrainingCoach::TimingGuide(
               EnemyAttackPattern::RiftBurst, DefenseTimingPreset::Forgiving, riftGuide)
            && Near(riftGuide.windupSeconds, 0.70f)
            && riftGuide.damage == 34
            && riftGuide.guardDamage == 0
            && !riftGuide.blockable
            && riftGuide.recommendedInput == DefenseInput::Dodge
            && Near(riftGuide.selectedPerfectWindowSeconds,
                ShadowbladeActions::ForgivingPerfectDefenseWindowSeconds),
        "pass24 timing guide marks unblockable RiftBurst for dodge practice");

    const ShadowbladeTrainingTimingGuide priorGuide = quickGuide;
    Expect(!ShadowbladeTrainingCoach::TimingGuide(
               static_cast<EnemyAttackPattern>(255),
               DefenseTimingPreset::Standard, quickGuide)
            && quickGuide.pattern == priorGuide.pattern
            && Near(quickGuide.windupSeconds, priorGuide.windupSeconds),
        "pass24 invalid threat cannot overwrite an existing timing guide");
    Expect(!ShadowbladeTrainingCoach::TimingGuide(
               EnemyAttackPattern::QuickCut,
               static_cast<DefenseTimingPreset>(255), quickGuide)
            && quickGuide.pattern == priorGuide.pattern,
        "pass24 invalid timing preset fails closed without guide mutation");
}
