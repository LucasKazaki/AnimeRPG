void TestDefensePracticePass13() {
    using namespace Astral::Scene;

    // GAME-062 and QOL-014: a bounded authored practice sequence advances only
    // after successful queues and records analytics against the exact pattern.
    DefensePracticeSequence sequence{};
    sequence.patterns[0] = EnemyAttackPattern::QuickCut;
    sequence.patterns[1] = EnemyAttackPattern::RiftBurst;
    sequence.patterns[2] = EnemyAttackPattern::GuardBreaker;
    sequence.count = 3;

    CombatSandbox sequenceCombat;
    ShadowbladeActions sequenceActions;
    DefensePracticeSession sequenceSession;
    Expect(sequenceSession.SetPracticeSequence(sequence)
            && sequenceSession.PracticeSequenceLength() == 3
            && sequenceSession.PracticeSequenceCursor() == 0,
        "pass13 practice sequence accepts three validated patterns");
    Expect(sequenceSession.QueueNextAttack(sequenceCombat, sequenceActions)
            && sequenceCombat.PendingEnemyAttack().pattern == EnemyAttackPattern::QuickCut
            && sequenceSession.PracticeSequenceCursor() == 1,
        "pass13 sequence queues QuickCut first and advances its cursor once");
    Expect(!sequenceSession.QueueNextAttack(sequenceCombat, sequenceActions)
            && sequenceSession.PracticeSequenceCursor() == 1,
        "pass13 rejected duplicate queue preserves sequence cursor");
    const EnemyAttackPlan firstSequencePlan = sequenceCombat.PendingEnemyAttack();
    Expect(sequenceSession.TryDefend(sequenceCombat, sequenceActions,
            DefenseInput::Guard).result == DefenseResult::Guarded,
        "pass13 first sequence attack resolves through authoritative guard logic");
    sequenceSession.AdvanceTime(
        sequenceCombat, sequenceActions, firstSequencePlan.recoverySeconds);

    Expect(sequenceSession.QueueNextAttack(sequenceCombat, sequenceActions)
            && sequenceCombat.PendingEnemyAttack().pattern == EnemyAttackPattern::RiftBurst
            && sequenceSession.PracticeSequenceCursor() == 2,
        "pass13 sequence queues its unblockable second pattern deterministically");
    const EnemyAttackPlan riftPlan = sequenceCombat.PendingEnemyAttack();
    Expect(sequenceSession.AdvanceTime(
            sequenceCombat, sequenceActions, riftPlan.windupSeconds)
            && sequenceActions.PlayerHealth() == 66,
        "pass13 sequence automatic RiftBurst hit uses canonical 34 damage");
    sequenceSession.AdvanceTime(sequenceCombat, sequenceActions, riftPlan.recoverySeconds);

    Expect(sequenceSession.QueueNextAttack(sequenceCombat, sequenceActions)
            && sequenceCombat.PendingEnemyAttack().pattern == EnemyAttackPattern::GuardBreaker
            && sequenceSession.PracticeSequenceCursor() == 0,
        "pass13 sequence queues its third pattern and loops cursor to zero");
    const EnemyAttackPlan breakerPlan = sequenceCombat.PendingEnemyAttack();
    Expect(sequenceSession.TryDefend(sequenceCombat, sequenceActions,
            DefenseInput::Guard).result == DefenseResult::Guarded,
        "pass13 GuardBreaker remains blockable inside the authored sequence");

    const DefensePracticePatternStats quickStats =
        sequenceSession.PatternStats(EnemyAttackPattern::QuickCut);
    const DefensePracticePatternStats riftStats =
        sequenceSession.PatternStats(EnemyAttackPattern::RiftBurst);
    const DefensePracticePatternStats breakerStats =
        sequenceSession.PatternStats(EnemyAttackPattern::GuardBreaker);
    Expect(quickStats.attempts == 1 && quickStats.ordinaryDefenses == 1
            && quickStats.hitsTaken == 0 && quickStats.damageTaken == 0,
        "pass13 analytics isolate QuickCut ordinary-defense results");
    Expect(riftStats.attempts == 1 && riftStats.hitsTaken == 1
            && riftStats.damageTaken == 34 && riftStats.ordinaryDefenses == 0,
        "pass13 analytics attribute RiftBurst damage only to RiftBurst");
    Expect(breakerStats.attempts == 1 && breakerStats.ordinaryDefenses == 1
            && breakerStats.damageTaken == 0,
        "pass13 analytics isolate GuardBreaker ordinary-defense results");

    sequenceSession.AdvanceTime(sequenceCombat, sequenceActions, breakerPlan.recoverySeconds);
    Expect(sequenceSession.QueueNextAttack(sequenceCombat, sequenceActions)
            && sequenceCombat.PendingEnemyAttack().pattern == EnemyAttackPattern::QuickCut,
        "pass13 practice sequence loops back to QuickCut after one full cycle");
    Expect(!sequenceSession.ResetMetrics(),
        "pass13 analytics reset refuses to erase an active owned attempt");
    Expect(sequenceSession.TryDefend(sequenceCombat, sequenceActions,
            DefenseInput::Guard).result == DefenseResult::Guarded,
        "pass13 active sequence attempt can finish before metrics reset");
    Expect(sequenceSession.ResetMetrics()
            && sequenceSession.PatternStats(EnemyAttackPattern::QuickCut).attempts == 0
            && sequenceSession.Stats().attacksQueued == 0
            && sequenceSession.PracticeSequenceLength() == 3
            && sequenceSession.PracticeSequenceCursor() == 0,
        "pass13 analytics reset clears telemetry while preserving sequence configuration");

    DefensePracticeSequence emptySequence{};
    Expect(!sequenceSession.SetPracticeSequence(emptySequence)
            && sequenceSession.PracticeSequenceLength() == 3,
        "pass13 zero-length sequence fails closed without replacing configuration");
    DefensePracticeSequence invalidSequence = sequence;
    invalidSequence.patterns[1] = static_cast<EnemyAttackPattern>(99);
    Expect(!sequenceSession.SetPracticeSequence(invalidSequence)
            && sequenceSession.PracticeSequenceLength() == 3,
        "pass13 invalid sequence pattern fails closed without partial mutation");
    DefensePracticeSequence oversizedSequence = sequence;
    oversizedSequence.count = DefensePracticeSequence::MaximumPatterns + 1;
    Expect(!sequenceSession.SetPracticeSequence(oversizedSequence)
            && sequenceSession.PracticeSequenceLength() == 3,
        "pass13 oversized sequence fails closed before reading out-of-range patterns");

    // GAME-063: practice pace maps only to the existing recovery cadence. Attack
    // windup, damage, guard damage, and blockability remain canonical.
    DefensePracticeSequence quickOnly{};
    quickOnly.patterns[0] = EnemyAttackPattern::QuickCut;
    quickOnly.count = 1;
    CombatSandbox paceCombat;
    ShadowbladeActions paceActions;
    DefensePracticeSession paceSession;
    Expect(paceSession.SetPracticeSequence(quickOnly)
            && paceSession.SetPace(DefensePracticePace::Learning),
        "pass13 learning pace can be selected before practice begins");
    Expect(paceSession.QueueNextAttack(paceCombat, paceActions),
        "pass13 learning pace queues its first canonical QuickCut");
    const EnemyAttackPlan learningPlan = paceCombat.PendingEnemyAttack();
    Expect(Near(learningPlan.windupSeconds, 0.55f)
            && learningPlan.damage == 18
            && learningPlan.guardDamage == 20
            && learningPlan.blockable
            && Near(learningPlan.recoverySeconds,
                CombatSandbox::RelaxedEnemyRecoverySeconds),
        "pass13 learning pace changes recovery only and preserves QuickCut threat values");
    Expect(paceSession.TryDefend(paceCombat, paceActions,
            DefenseInput::Guard).result == DefenseResult::Guarded,
        "pass13 learning pace defense resolves normally");
    paceSession.AdvanceTime(paceCombat, paceActions, learningPlan.recoverySeconds);
    Expect(paceSession.SetPace(DefensePracticePace::Expert)
            && paceSession.QueueNextAttack(paceCombat, paceActions),
        "pass13 expert pace can replace learning pace between attempts");
    const EnemyAttackPlan expertPlan = paceCombat.PendingEnemyAttack();
    Expect(Near(expertPlan.windupSeconds, learningPlan.windupSeconds)
            && expertPlan.damage == learningPlan.damage
            && expertPlan.guardDamage == learningPlan.guardDamage
            && expertPlan.blockable == learningPlan.blockable
            && Near(expertPlan.recoverySeconds,
                CombatSandbox::AggressiveEnemyRecoverySeconds),
        "pass13 expert pace shortens recovery without inflating health or attack power");
    Expect(paceSession.TryDefend(paceCombat, paceActions,
            DefenseInput::Guard).result == DefenseResult::Guarded,
        "pass13 expert pace still delegates defense to the same production bridge");
    paceSession.AdvanceTime(paceCombat, paceActions, expertPlan.recoverySeconds);
    const DefensePracticePace paceBeforeInvalid = paceSession.Pace();
    Expect(!paceSession.SetPace(static_cast<DefensePracticePace>(99))
            && paceSession.Pace() == paceBeforeInvalid,
        "pass13 invalid practice pace fails closed");

    // GAME-064: alternating Guard/Dodge execution builds a bounded chain. A
    // repeated defense or a miss resets the live chain but not the best chain.
    CombatSandbox chainCombat;
    ShadowbladeActions chainActions;
    DefensePracticeSession chainSession;
    Expect(chainSession.SetPracticeSequence(quickOnly)
            && chainSession.QueueNextAttack(chainCombat, chainActions)
            && chainSession.TryDefend(chainCombat, chainActions,
                DefenseInput::Guard).result == DefenseResult::Guarded
            && chainSession.CurrentAlternatingChain() == 1,
        "pass13 alternating chain starts at one successful guard");
    chainSession.AdvanceTime(chainCombat, chainActions, 0.85f);
    Expect(chainSession.QueueNextAttack(chainCombat, chainActions),
        "pass13 alternating chain queues its dodge attempt");
    chainSession.AdvanceTime(chainCombat, chainActions, 0.25f);
    Expect(chainSession.TryDefend(chainCombat, chainActions,
            DefenseInput::Dodge).result == DefenseResult::Evaded
            && chainSession.CurrentAlternatingChain() == 2,
        "pass13 Guard then Dodge extends the alternating chain inside its window");
    chainSession.AdvanceTime(chainCombat, chainActions, 0.85f);
    Expect(chainSession.QueueNextAttack(chainCombat, chainActions)
            && chainSession.TryDefend(chainCombat, chainActions,
                DefenseInput::Guard).result == DefenseResult::Guarded
            && chainSession.CurrentAlternatingChain() == 3
            && chainSession.BestAlternatingChain() == 3,
        "pass13 Guard Dodge Guard reaches a three-technique best chain");
    chainSession.AdvanceTime(chainCombat, chainActions, 0.85f);
    Expect(chainSession.QueueNextAttack(chainCombat, chainActions)
            && chainSession.TryDefend(chainCombat, chainActions,
                DefenseInput::Guard).result == DefenseResult::Guarded
            && chainSession.CurrentAlternatingChain() == 1
            && chainSession.BestAlternatingChain() == 3,
        "pass13 repeated Guard resets live variety without erasing the best chain");
    chainSession.AdvanceTime(chainCombat, chainActions, 0.85f);
    Expect(chainSession.QueueNextAttack(chainCombat, chainActions),
        "pass13 chain reset setup queues a missed QuickCut");
    const EnemyAttackPlan chainMiss = chainCombat.PendingEnemyAttack();
    Expect(chainSession.AdvanceTime(chainCombat, chainActions, chainMiss.windupSeconds)
            && chainSession.CurrentAlternatingChain() == 0
            && chainSession.BestAlternatingChain() == 3,
        "pass13 taking a hit resets live alternating chain while preserving best");

    // Invalid DefenseInput values must not fabricate an alternating technique.
    CombatSandbox invalidInputCombat;
    ShadowbladeActions invalidInputActions;
    DefensePracticeSession invalidInputSession;
    Expect(invalidInputSession.SetPracticeSequence(quickOnly)
            && invalidInputSession.QueueNextAttack(invalidInputCombat, invalidInputActions),
        "pass13 invalid-input chain regression queues first QuickCut");
    const DefenseReport invalidInputReport = invalidInputSession.TryDefend(
        invalidInputCombat, invalidInputActions, static_cast<DefenseInput>(99));
    Expect(invalidInputReport.result == DefenseResult::Guarded
            && invalidInputSession.CurrentAlternatingChain() == 1
            && invalidInputSession.BestAlternatingChain() == 1,
        "pass13 chain derives technique from authoritative guard result");
    invalidInputSession.AdvanceTime(invalidInputCombat, invalidInputActions, 0.85f);
    Expect(invalidInputSession.QueueNextAttack(invalidInputCombat, invalidInputActions)
            && invalidInputSession.TryDefend(invalidInputCombat, invalidInputActions,
                DefenseInput::Guard).result == DefenseResult::Guarded
            && invalidInputSession.CurrentAlternatingChain() == 1
            && invalidInputSession.BestAlternatingChain() == 1,
        "pass13 malformed defense input cannot fabricate a Dodge-to-Guard chain");

    // The chain clock is session-owned and monotonic. Resetting the combat
    // sandbox clock after the chain window elapsed cannot revive an old link.
    CombatSandbox resetChainCombat;
    ShadowbladeActions resetChainActions;
    DefensePracticeSession resetChainSession;
    Expect(resetChainSession.SetPracticeSequence(quickOnly)
            && resetChainSession.QueueNextAttack(resetChainCombat, resetChainActions)
            && resetChainSession.TryDefend(resetChainCombat, resetChainActions,
                DefenseInput::Guard).result == DefenseResult::Guarded
            && resetChainSession.CurrentAlternatingChain() == 1,
        "pass13 reset-chain regression records its first guard");
    resetChainSession.AdvanceTime(resetChainCombat, resetChainActions, 3.0f);
    resetChainCombat.ResetTrainingSession();
    Expect(resetChainSession.QueueNextAttack(resetChainCombat, resetChainActions),
        "pass13 reset-chain regression can queue after combat clock reset");
    resetChainSession.AdvanceTime(resetChainCombat, resetChainActions, 0.25f);
    Expect(resetChainSession.TryDefend(resetChainCombat, resetChainActions,
            DefenseInput::Dodge).result == DefenseResult::Evaded
            && resetChainSession.CurrentAlternatingChain() == 1
            && resetChainSession.BestAlternatingChain() == 1,
        "pass13 combat clock reset cannot extend an expired alternating chain");

    // A successful defense resolved directly by ShadowbladeActions must produce
    // the same chain result when the session reconciles it on the next tick,
    // even when that coordinator frame itself is longer than the chain window.
    CombatSandbox externalDefenseCombat;
    ShadowbladeActions externalDefenseActions;
    DefensePracticeSession externalDefenseSession;
    Expect(externalDefenseSession.SetPracticeSequence(quickOnly)
            && externalDefenseSession.QueueNextAttack(
                externalDefenseCombat, externalDefenseActions)
            && externalDefenseSession.TryDefend(
                externalDefenseCombat, externalDefenseActions,
                DefenseInput::Guard).result == DefenseResult::Guarded
            && externalDefenseSession.CurrentAlternatingChain() == 1,
        "pass13 external-defense regression records its first guard");
    externalDefenseSession.AdvanceTime(
        externalDefenseCombat, externalDefenseActions, 0.85f);
    Expect(externalDefenseSession.QueueNextAttack(
            externalDefenseCombat, externalDefenseActions),
        "pass13 external-defense regression queues the alternating attempt");
    externalDefenseSession.AdvanceTime(
        externalDefenseCombat, externalDefenseActions, 0.25f);
    const DefenseReport externalDodge =
        externalDefenseActions.TryDefend(DefenseInput::Dodge);
    Expect(externalDodge.result == DefenseResult::Evaded
            && externalDefenseSession.AdvanceTime(
                externalDefenseCombat, externalDefenseActions, 3.0f)
            && externalDefenseSession.CurrentAlternatingChain() == 2
            && externalDefenseSession.BestAlternatingChain() == 2,
        "pass13 reconciliation timestamps external Dodge before a long coordinator frame");
    const DefensePracticePatternStats externalDefenseQuick =
        externalDefenseSession.PatternStats(EnemyAttackPattern::QuickCut);
    Expect(externalDefenseQuick.attempts == 2
            && externalDefenseQuick.ordinaryDefenses == 2
            && externalDefenseQuick.hitsTaken == 0,
        "pass13 externally reconciled defense updates per-pattern analytics exactly once");

    // GAME-065: score separates original execution points from a bounded time
    // coefficient and accumulates only valid, unpaused session time.
    CombatSandbox scoreCombat;
    ShadowbladeActions scoreActions;
    DefensePracticeSession scoreSession;
    Expect(scoreSession.SetPracticeSequence(quickOnly),
        "pass13 score setup selects a deterministic QuickCut sequence");
    const DefensePracticeScoreReport emptyScore = scoreSession.Score(scoreCombat);
    Expect(emptyScore.score == 0 && emptyScore.baseScore == 0
            && emptyScore.resolvedAttempts == 0,
        "pass13 score remains zero before any resolved practice attempt");
    Expect(scoreSession.QueueNextAttack(scoreCombat, scoreActions)
            && scoreSession.TryDefend(scoreCombat, scoreActions,
                DefenseInput::Guard).result == DefenseResult::Guarded,
        "pass13 score setup records one ordinary guard");
    DefensePracticeScoreReport fastScore = scoreSession.Score(scoreCombat);
    Expect(fastScore.baseScore == 200 && fastScore.timeCoefficientPercent == 125
            && fastScore.score == 250 && fastScore.resolvedAttempts == 1,
        "pass13 fast clean defense gets original base points and 125 percent coefficient");
    Expect(scoreSession.SetPaused(true),
        "pass13 score setup pauses practice between attempts");
    const DefensePracticeScoreReport beforePausedTime = scoreSession.Score(scoreCombat);
    Expect(!scoreSession.AdvanceTime(scoreCombat, scoreActions, 100.0f)
            && scoreSession.Score(scoreCombat).score == beforePausedTime.score
            && scoreSession.Score(scoreCombat).timeCoefficientPercent
                == beforePausedTime.timeCoefficientPercent,
        "pass13 paused time cannot dilute practice time coefficient");
    Expect(scoreSession.SetPaused(false),
        "pass13 score setup resumes practice");
    scoreSession.AdvanceTime(scoreCombat, scoreActions, 10.0f);
    const DefensePracticeScoreReport slowScore = scoreSession.Score(scoreCombat);
    Expect(slowScore.baseScore == fastScore.baseScore
            && slowScore.timeCoefficientPercent == 75
            && slowScore.score == 150,
        "pass13 valid unpaused idle time lowers only the time coefficient");

    // Hits contribute no base points, but they still count as resolved attempts
    // for the time coefficient denominator.
    CombatSandbox mixedScoreCombat;
    ShadowbladeActions mixedScoreActions;
    DefensePracticeSession mixedScoreSession;
    Expect(mixedScoreSession.SetPracticeSequence(quickOnly)
            && mixedScoreSession.QueueNextAttack(mixedScoreCombat, mixedScoreActions)
            && mixedScoreSession.TryDefend(mixedScoreCombat, mixedScoreActions,
                DefenseInput::Guard).result == DefenseResult::Guarded,
        "pass13 mixed-score regression records its initial successful guard");
    mixedScoreSession.AdvanceTime(mixedScoreCombat, mixedScoreActions, 0.85f);
    Expect(mixedScoreSession.QueueNextAttack(mixedScoreCombat, mixedScoreActions),
        "pass13 mixed-score regression queues first miss");
    EnemyAttackPlan mixedMiss = mixedScoreCombat.PendingEnemyAttack();
    Expect(mixedScoreSession.AdvanceTime(
            mixedScoreCombat, mixedScoreActions, mixedMiss.windupSeconds),
        "pass13 mixed-score regression resolves first miss");
    mixedScoreSession.AdvanceTime(
        mixedScoreCombat, mixedScoreActions, mixedMiss.recoverySeconds);
    Expect(mixedScoreSession.QueueNextAttack(mixedScoreCombat, mixedScoreActions),
        "pass13 mixed-score regression queues second miss");
    mixedMiss = mixedScoreCombat.PendingEnemyAttack();
    Expect(mixedScoreSession.AdvanceTime(
            mixedScoreCombat, mixedScoreActions, mixedMiss.windupSeconds),
        "pass13 mixed-score regression resolves second miss");
    const DefensePracticeScoreReport mixedScore = mixedScoreSession.Score(mixedScoreCombat);
    Expect(mixedScore.resolvedAttempts == 3
            && mixedScore.baseScore == 200
            && mixedScore.timeCoefficientPercent == 125
            && mixedScore.score == 250,
        "pass13 time coefficient averages over successes and hits without rewarding hits");

    // Hit-only sessions still expose the timing coefficient even though their
    // execution score stays at zero.
    CombatSandbox hitOnlyScoreCombat;
    ShadowbladeActions hitOnlyScoreActions;
    DefensePracticeSession hitOnlyScoreSession;
    Expect(hitOnlyScoreSession.SetPracticeSequence(quickOnly)
            && hitOnlyScoreSession.QueueNextAttack(
                hitOnlyScoreCombat, hitOnlyScoreActions),
        "pass13 hit-only score regression queues one QuickCut");
    const EnemyAttackPlan hitOnlyPlan = hitOnlyScoreCombat.PendingEnemyAttack();
    Expect(hitOnlyScoreSession.AdvanceTime(
            hitOnlyScoreCombat, hitOnlyScoreActions, hitOnlyPlan.windupSeconds),
        "pass13 hit-only score regression resolves the missed QuickCut");
    const DefensePracticeScoreReport hitOnlyScore =
        hitOnlyScoreSession.Score(hitOnlyScoreCombat);
    Expect(hitOnlyScore.resolvedAttempts == 1
            && hitOnlyScore.baseScore == 0
            && hitOnlyScore.timeCoefficientPercent == 125
            && hitOnlyScore.score == 0,
        "pass13 hit-only report keeps zero points while exposing its timing coefficient");

    // External CombatSandbox resets must not rewind already accumulated session
    // time or improve an earned coefficient.
    CombatSandbox resetClockCombat;
    ShadowbladeActions resetClockActions;
    DefensePracticeSession resetClockSession;
    Expect(resetClockSession.SetPracticeSequence(quickOnly)
            && resetClockSession.QueueNextAttack(resetClockCombat, resetClockActions)
            && resetClockSession.TryDefend(resetClockCombat, resetClockActions,
                DefenseInput::Guard).result == DefenseResult::Guarded,
        "pass13 reset-clock regression records one successful guard");
    resetClockSession.AdvanceTime(resetClockCombat, resetClockActions, 4.0f);
    const DefensePracticeScoreReport beforeCombatReset =
        resetClockSession.Score(resetClockCombat);
    Expect(beforeCombatReset.timeCoefficientPercent == 75
            && beforeCombatReset.score == 150,
        "pass13 reset-clock regression first earns the slow timing coefficient");
    resetClockCombat.ResetTrainingSession();
    const DefensePracticeScoreReport afterCombatReset =
        resetClockSession.Score(resetClockCombat);
    Expect(afterCombatReset.resolvedAttempts == beforeCombatReset.resolvedAttempts
            && afterCombatReset.baseScore == beforeCombatReset.baseScore
            && afterCombatReset.timeCoefficientPercent
                == beforeCombatReset.timeCoefficientPercent
            && afterCombatReset.score == beforeCombatReset.score,
        "pass13 combat clock reset cannot improve or erase accumulated session score timing");

    // GAME-066: goal categories keep pass-12 semantics while the session can
    // choose a bounded target count from one through ten.
    CombatSandbox goalCombat;
    ShadowbladeActions goalActions;
    DefensePracticeSession goalSession;
    Expect(goalSession.SetPracticeSequence(quickOnly)
            && goalSession.SetGoal(DefenseTrainingGoal::GuardDiscipline)
            && goalSession.SetGoalTarget(2),
        "pass13 guard goal accepts a two-success target");
    Expect(goalSession.QueueNextAttack(goalCombat, goalActions)
            && goalSession.TryDefend(goalCombat, goalActions,
                DefenseInput::Guard).result == DefenseResult::Guarded,
        "pass13 configurable goal records first guard");
    DefenseTrainingGoalStatus goalStatus = goalSession.GoalStatus();
    Expect(goalStatus.current == 1 && goalStatus.target == 2 && !goalStatus.complete,
        "pass13 configurable goal remains incomplete below selected target");
    goalSession.AdvanceTime(goalCombat, goalActions, 0.85f);
    Expect(goalSession.QueueNextAttack(goalCombat, goalActions)
            && goalSession.TryDefend(goalCombat, goalActions,
                DefenseInput::Guard).result == DefenseResult::Guarded,
        "pass13 configurable goal records second guard");
    goalStatus = goalSession.GoalStatus();
    Expect(goalStatus.current == 2 && goalStatus.target == 2 && goalStatus.complete,
        "pass13 configurable goal completes exactly at selected target");
    Expect(!goalSession.SetGoalTarget(0) && !goalSession.SetGoalTarget(11)
            && !goalSession.SetGoalTarget(2) && goalSession.GoalTarget() == 2,
        "pass13 invalid and unchanged goal targets fail without mutation");
    Expect(goalSession.SetGoal(DefenseTrainingGoal::FreePractice),
        "pass13 configurable goal can return to objective-free practice");
    const DefenseTrainingGoalStatus freeStatus = goalSession.GoalStatus();
    Expect(freeStatus.target == 0 && freeStatus.current == 0 && !freeStatus.complete,
        "pass13 Free Practice remains objective-free regardless of saved target count");

    // QOL-014 also covers authoritative interruption attribution, not just hits.
    CombatSandbox interruptedCombat;
    ShadowbladeActions interruptedActions;
    DefensePracticeSession interruptedSession;
    Expect(interruptedSession.SetPracticeSequence(quickOnly)
            && interruptedSession.QueueNextAttack(interruptedCombat, interruptedActions),
        "pass13 interruption analytics queues an owned QuickCut");
    interruptedCombat.TryAttack(AttackType::Light, {});
    interruptedSession.AdvanceTime(interruptedCombat, interruptedActions, 0.4f);
    const AttackReport interruptionHeavy = interruptedCombat.TryAttack(AttackType::Heavy, {});
    Expect(interruptionHeavy.staggerTriggered
            && interruptedSession.AdvanceTime(interruptedCombat, interruptedActions, 0.01f),
        "pass13 interruption analytics reconciles authoritative stagger cancellation");
    const DefensePracticePatternStats interruptedQuick =
        interruptedSession.PatternStats(EnemyAttackPattern::QuickCut);
    Expect(interruptedQuick.attempts == 1 && interruptedQuick.interruptions == 1
            && interruptedQuick.hitsTaken == 0 && interruptedQuick.damageTaken == 0,
        "pass13 analytics records interruption exactly once without fabricating damage");

    // An input arriving after authoritative interruption must reconcile the same
    // stale link immediately rather than wedging the session until another tick.
    CombatSandbox inputInterruptCombat;
    ShadowbladeActions inputInterruptActions;
    DefensePracticeSession inputInterruptSession;
    Expect(inputInterruptSession.SetPracticeSequence(quickOnly)
            && inputInterruptSession.QueueNextAttack(
                inputInterruptCombat, inputInterruptActions),
        "pass13 TryDefend interruption regression queues an owned QuickCut");
    inputInterruptCombat.TryAttack(AttackType::Light, {});
    inputInterruptSession.AdvanceTime(inputInterruptCombat, inputInterruptActions, 0.4f);
    const AttackReport inputInterruptHeavy =
        inputInterruptCombat.TryAttack(AttackType::Heavy, {});
    const DefenseReport staleDefense = inputInterruptSession.TryDefend(
        inputInterruptCombat, inputInterruptActions, DefenseInput::Guard);
    const DefensePracticePatternStats inputInterruptedQuick =
        inputInterruptSession.PatternStats(EnemyAttackPattern::QuickCut);
    Expect(inputInterruptHeavy.staggerTriggered
            && staleDefense.result == DefenseResult::NoThreat
            && inputInterruptSession.Stats().interruptions == 1
            && inputInterruptedQuick.interruptions == 1,
        "pass13 TryDefend reconciles authoritative interruption exactly once");
    Expect(inputInterruptSession.ResetMetrics()
            && inputInterruptSession.SetPace(DefensePracticePace::Learning),
        "pass13 TryDefend interruption clears session ownership for reset and reconfiguration");
}