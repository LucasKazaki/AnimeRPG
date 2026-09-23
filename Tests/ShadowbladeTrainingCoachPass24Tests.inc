void TestShadowbladeTrainingCoachPass24() {
    using namespace Astral::Scene;

    // GAME-117: focused drills select bounded canonical threat sequences while
    // continuing to use the authoritative DefensePracticeSession combat path.
    ShadowbladeTrainingDrillPlan quickPlan{};
    Expect(ShadowbladeTrainingCoach::PlanForFocus(
               ShadowbladeTrainingFocus::QuickCut,
               DefensePracticePace::Learning, quickPlan)
            && quickPlan.sequence.count == 1
            && quickPlan.sequence.patterns[0] == EnemyAttackPattern::QuickCut
            && quickPlan.pace == DefensePracticePace::Learning
            && quickPlan.goal == DefenseTrainingGoal::GuardDiscipline
            && quickPlan.goalTarget == DefensePracticeSession::DefaultGoalTarget,
        "pass24 QuickCut focus maps to the canonical guard drill");

    ShadowbladeTrainingDrillPlan bossPlan{};
    Expect(ShadowbladeTrainingCoach::PlanForFocus(
               ShadowbladeTrainingFocus::BossCycle,
               DefensePracticePace::Expert, bossPlan)
            && bossPlan.sequence.count == 3
            && bossPlan.sequence.patterns[0] == EnemyAttackPattern::QuickCut
            && bossPlan.sequence.patterns[1] == EnemyAttackPattern::GuardBreaker
            && bossPlan.sequence.patterns[2] == EnemyAttackPattern::RiftBurst
            && bossPlan.pace == DefensePracticePace::Expert
            && bossPlan.goal == DefenseTrainingGoal::FreePractice,
        "pass24 boss focus maps to all three authoritative threat types");

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
               focusedSession, ShadowbladeTrainingFocus::RiftBurst,
               DefensePracticePace::Learning)
            && focusedSession.PracticeSequenceLength() == 1
            && focusedSession.Pace() == DefensePracticePace::Learning
            && focusedSession.Goal() == DefenseTrainingGoal::DodgeDiscipline
            && focusedSession.QueueNextAttack(focusedCombat, focusedActions)
            && focusedCombat.PendingEnemyAttack().pattern == EnemyAttackPattern::RiftBurst,
        "pass24 focused drill config reaches the existing live threat path");
    const DefenseTrainingStats focusedBefore = focusedSession.Stats();
    Expect(!ShadowbladeTrainingCoach::ApplyDrill(
               focusedSession, ShadowbladeTrainingFocus::QuickCut,
               DefensePracticePace::Standard)
            && focusedActions.HasIncomingAttack()
            && focusedSession.Stats().attacksQueued == focusedBefore.attacksQueued
            && focusedSession.PracticeSequenceLength() == 1
            && focusedSession.Goal() == DefenseTrainingGoal::DodgeDiscipline,
        "pass24 drill reconfiguration is atomic while a threat is active");
    const EnemyAttackPlan focusedThreat = focusedCombat.PendingEnemyAttack();
    focusedSession.AdvanceTime(focusedCombat, focusedActions,
        focusedThreat.windupSeconds - 0.10f);
    Expect(focusedSession.TryDefend(focusedCombat, focusedActions,
               DefenseInput::Dodge).result == DefenseResult::PerfectDodge,
        "pass24 focused RiftBurst drill resolves through authoritative defense timing");
    focusedSession.AdvanceTime(
        focusedCombat, focusedActions, focusedThreat.recoverySeconds);

    // GAME-118 and GAME-119: run a bounded five-attempt practice sample so the
    // debrief and optional challenges are derived from real session metrics.
    DefensePracticeSession masterySession;
    ShadowbladeActions masteryActions;
    CombatSandbox masteryCombat;
    Expect(ShadowbladeTrainingCoach::ApplyDrill(
               masterySession, ShadowbladeTrainingFocus::BossCycle,
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
    Expect(masteryDebrief.resolvedAttempts == 5
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
               missSession, ShadowbladeTrainingFocus::BossCycle,
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
               ordinarySession, ShadowbladeTrainingFocus::QuickCut,
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
    // contract, and it refuses to erase an active threat.
    DefensePracticeSession retrySession;
    ShadowbladeActions retryActions;
    CombatSandbox retryCombat;
    Expect(ShadowbladeTrainingCoach::ApplyDrill(
               retrySession, ShadowbladeTrainingFocus::MixedDefense,
               DefensePracticePace::Expert)
            && retrySession.QueueNextAttack(retryCombat, retryActions)
            && retrySession.TryDefend(retryCombat, retryActions,
                DefenseInput::Guard).result == DefenseResult::Guarded,
        "pass24 retry sample records an ordinary result before reset");
    EnemyAttackPlan retryPlan = retryCombat.PendingEnemyAttack();
    retrySession.AdvanceTime(retryCombat, retryActions, retryPlan.recoverySeconds);
    Expect(ShadowbladeTrainingCoach::RetryDrill(retrySession)
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
    Expect(!ShadowbladeTrainingCoach::RetryDrill(retrySession)
            && retryActions.HasIncomingAttack()
            && retrySession.Stats().attacksQueued == activeRetryBefore.attacksQueued
            && retrySession.PracticeSequenceLength() == 2,
        "pass24 quick retry fails closed while an owned threat is active");

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
