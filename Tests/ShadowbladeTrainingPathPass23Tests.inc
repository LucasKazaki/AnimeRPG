void TestShadowbladeTrainingPathPass23() {
    using namespace Astral::Scene;

    // GAME-112 and GAME-113: a sequential authored curriculum configures the
    // existing defense-practice path atomically instead of creating new combat.
    ShadowbladeTrainingPath curriculum;
    DefensePracticeSession lessonSession;
    ShadowbladeActions lessonActions;
    CombatSandbox lessonCombat;
    Expect(curriculum.CurrentLesson() == ShadowbladeTrainingLesson::GuardFundamentals
            && curriculum.LessonUnlocked(ShadowbladeTrainingLesson::GuardFundamentals)
            && !curriculum.LessonUnlocked(ShadowbladeTrainingLesson::DodgeFundamentals)
            && curriculum.CompletedLessonCount() == 0,
        "pass23 curriculum starts with only guard fundamentals unlocked");
    Expect(curriculum.ApplyCurrentLesson(lessonSession, lessonActions)
            && lessonSession.PracticeSequenceLength() == 1
            && lessonSession.Pace() == DefensePracticePace::Learning
            && lessonSession.Goal() == DefenseTrainingGoal::GuardDiscipline
            && lessonSession.GoalTarget() == 3,
        "pass23 first lesson atomically configures sequence pace goal and target");

    for (int attempt = 0; attempt < 3; ++attempt) {
        Expect(lessonSession.QueueNextAttack(lessonCombat, lessonActions),
            "pass23 guard lesson queues its canonical QuickCut");
        const EnemyAttackPlan plan = lessonCombat.PendingEnemyAttack();
        Expect(lessonSession.TryDefend(lessonCombat, lessonActions,
                DefenseInput::Guard).result == DefenseResult::Guarded,
            "pass23 guard lesson resolves through authoritative defense logic");
        lessonSession.AdvanceTime(lessonCombat, lessonActions, plan.recoverySeconds);
    }
    Expect(curriculum.CurrentLessonComplete(lessonSession),
        "pass23 guard fundamentals completes at its authored target");

    // GAME-114: lesson mastery is derived from authoritative practice stats.
    Expect(curriculum.CurrentMedal(lessonSession) == ShadowbladeTrainingMedal::Silver,
        "pass23 three clean ordinary guards earn a silver lesson medal");
    Expect(curriculum.CommitCurrentLesson(lessonSession)
            && curriculum.CompletedLessonCount() == 1
            && curriculum.MedalFor(ShadowbladeTrainingLesson::GuardFundamentals)
                == ShadowbladeTrainingMedal::Silver
            && curriculum.CurrentLesson() == ShadowbladeTrainingLesson::DodgeFundamentals
            && curriculum.LessonUnlocked(ShadowbladeTrainingLesson::DodgeFundamentals),
        "pass23 committing a completed lesson records mastery and unlocks the next lesson");
    Expect(!curriculum.CommitCurrentLesson(lessonSession)
            && curriculum.CompletedLessonCount() == 1,
        "pass23 stale lesson metrics cannot double-complete the next lesson");

    // GAME-113 atomicity: reconfiguration refuses an active owned threat and
    // leaves the live practice session intact rather than partially resetting it.
    DefensePracticeSession atomicSession;
    ShadowbladeActions atomicActions;
    CombatSandbox atomicCombat;
    ShadowbladeTrainingPath atomicPath;
    Expect(atomicPath.ApplyCurrentLesson(atomicSession, atomicActions)
            && atomicSession.QueueNextAttack(atomicCombat, atomicActions),
        "pass23 atomic setup creates an owned active training threat");
    const DefenseTrainingStats atomicBefore = atomicSession.Stats();
    Expect(!atomicPath.ApplyCurrentLesson(atomicSession, atomicActions)
            && atomicActions.HasIncomingAttack()
            && atomicSession.Stats().attacksQueued == atomicBefore.attacksQueued
            && atomicSession.PracticeSequenceLength() == 1
            && atomicSession.Goal() == DefenseTrainingGoal::GuardDiscipline,
        "pass23 rejected mid-threat lesson setup makes no partial mutation");

    // Review regression: an incoming threat owned outside the practice session
    // still blocks lesson application, including timing-preset synchronization.
    ShadowbladeTrainingCheckpoint detachedThreatCheckpoint{};
    detachedThreatCheckpoint.forgivingTimingAssist = true;
    ShadowbladeTrainingPath detachedThreatPath;
    DefensePracticeSession detachedThreatSession;
    ShadowbladeActions detachedThreatActions;
    Expect(detachedThreatPath.RestoreCheckpoint(detachedThreatCheckpoint)
            && detachedThreatActions.BeginIncomingAttack({1.0f, 20, 10, true})
            && !detachedThreatPath.ApplyCurrentLesson(
                detachedThreatSession, detachedThreatActions)
            && detachedThreatActions.HasIncomingAttack()
            && detachedThreatSession.PracticeSequenceLength() == 0
            && Near(detachedThreatActions.PerfectDefenseWindowSeconds(),
                ShadowbladeActions::StandardPerfectDefenseWindowSeconds),
        "pass23 detached live threat rejects setup before session or timing mutation");

    // GAME-115: timing assistance is opt-in and only widens the existing perfect
    // window. It cannot be toggled under an active threat.
    ShadowbladeTrainingPath assistPath;
    DefensePracticeSession assistSession;
    ShadowbladeActions assistActions;
    CombatSandbox assistCombat;
    Expect(assistPath.SetForgivingTimingAssist(true, assistActions)
            && assistPath.ForgivingTimingAssistEnabled()
            && Near(assistActions.PerfectDefenseWindowSeconds(),
                ShadowbladeActions::ForgivingPerfectDefenseWindowSeconds)
            && assistPath.ApplyCurrentLesson(assistSession, assistActions),
        "pass23 forgiving timing assist applies the existing wider timing preset");
    Expect(assistSession.QueueNextAttack(assistCombat, assistActions),
        "pass23 assist regression queues a live QuickCut");
    Expect(!assistPath.SetForgivingTimingAssist(false, assistActions)
            && assistPath.ForgivingTimingAssistEnabled()
            && Near(assistActions.PerfectDefenseWindowSeconds(),
                ShadowbladeActions::ForgivingPerfectDefenseWindowSeconds),
        "pass23 timing assist cannot change during an incoming threat");
    const EnemyAttackPlan assistPlan = assistCombat.PendingEnemyAttack();
    Expect(assistSession.TryDefend(assistCombat, assistActions,
            DefenseInput::Guard).result == DefenseResult::Guarded,
        "pass23 timing assist still delegates defense to production logic");
    assistSession.AdvanceTime(assistCombat, assistActions, assistPlan.recoverySeconds);
    Expect(assistPath.SetForgivingTimingAssist(false, assistActions)
            && !assistPath.ForgivingTimingAssistEnabled()
            && Near(assistActions.PerfectDefenseWindowSeconds(),
                ShadowbladeActions::StandardPerfectDefenseWindowSeconds),
        "pass23 timing assist returns to standard only after the threat is resolved");

    // GAME-116: versioned in-memory checkpoints resume validated progress and
    // reject malformed or rollback state without partially mutating the path.
    const ShadowbladeTrainingCheckpoint firstLessonCheckpoint = curriculum.Checkpoint();
    ShadowbladeTrainingPath restored;
    Expect(restored.RestoreCheckpoint(firstLessonCheckpoint)
            && restored.CurrentLesson() == ShadowbladeTrainingLesson::DodgeFundamentals
            && restored.CompletedLessonCount() == 1
            && restored.MedalFor(ShadowbladeTrainingLesson::GuardFundamentals)
                == ShadowbladeTrainingMedal::Silver,
        "pass23 checkpoint restores exact sequential curriculum progress");
    const ShadowbladeTrainingCheckpoint restoredBeforeInvalid = restored.Checkpoint();
    ShadowbladeTrainingCheckpoint badSchema = firstLessonCheckpoint;
    badSchema.schemaVersion = 99;
    Expect(!restored.RestoreCheckpoint(badSchema)
            && restored.Checkpoint().completedMask == restoredBeforeInvalid.completedMask,
        "pass23 unknown checkpoint schema fails closed without progress mutation");
    ShadowbladeTrainingCheckpoint badMask = firstLessonCheckpoint;
    badMask.completedMask = 5;
    badMask.currentLesson = ShadowbladeTrainingLesson::DodgeFundamentals;
    Expect(!restored.RestoreCheckpoint(badMask)
            && restored.CompletedLessonCount() == 1,
        "pass23 non-prefix checkpoint progress fails closed");
    ShadowbladeTrainingPath emptyPath;
    Expect(!restored.RestoreCheckpoint(emptyPath.Checkpoint())
            && restored.CompletedLessonCount() == 1,
        "pass23 an older valid checkpoint cannot roll local progress backward");

    // Review regression: a lesson cannot complete from a same-length sequence
    // that omits one of its authored threat types.
    ShadowbladeTrainingCheckpoint mixedCheckpoint{};
    mixedCheckpoint.completedMask = 3;
    mixedCheckpoint.currentLesson = ShadowbladeTrainingLesson::MixedTiming;
    mixedCheckpoint.medals[0] = ShadowbladeTrainingMedal::Bronze;
    mixedCheckpoint.medals[1] = ShadowbladeTrainingMedal::Bronze;
    ShadowbladeTrainingPath provenancePath;
    DefensePracticeSession provenanceSession;
    ShadowbladeActions provenanceActions;
    CombatSandbox provenanceCombat;
    DefensePracticeSequence duplicateQuickCuts{};
    duplicateQuickCuts.patterns[0] = EnemyAttackPattern::QuickCut;
    duplicateQuickCuts.patterns[1] = EnemyAttackPattern::QuickCut;
    duplicateQuickCuts.count = 2;
    Expect(provenancePath.RestoreCheckpoint(mixedCheckpoint)
            && provenancePath.ApplyCurrentLesson(provenanceSession, provenanceActions)
            && provenanceSession.ClearPracticeSequence()
            && provenanceSession.SetPracticeSequence(duplicateQuickCuts),
        "pass23 provenance regression replaces mixed timing with same-length duplicate threats");
    for (int attempt = 0; attempt < 3; ++attempt) {
        Expect(provenanceSession.QueueNextAttack(provenanceCombat, provenanceActions)
                && provenanceCombat.PendingEnemyAttack().pattern
                    == EnemyAttackPattern::QuickCut,
            "pass23 provenance regression queues only the duplicated QuickCut");
        const EnemyAttackPlan plan = provenanceCombat.PendingEnemyAttack();
        provenanceSession.AdvanceTime(provenanceCombat, provenanceActions, 0.45f);
        Expect(provenanceSession.TryDefend(provenanceCombat, provenanceActions,
                DefenseInput::Dodge).result == DefenseResult::PerfectDodge,
            "pass23 provenance regression reaches the nominal perfect-defense target");
        provenanceSession.AdvanceTime(
            provenanceCombat, provenanceActions, plan.recoverySeconds);
    }
    Expect(provenanceSession.GoalStatus().complete
            && !provenancePath.CurrentLessonComplete(provenanceSession)
            && !provenancePath.CommitCurrentLesson(provenanceSession)
            && provenancePath.CurrentLesson() == ShadowbladeTrainingLesson::MixedTiming,
        "pass23 missing RiftBurst provenance blocks completion despite matching length and goal");

    // Review regression: Gold requires an all-perfect completion. A clean run
    // containing any ordinary defense earns Silver instead.
    ShadowbladeTrainingPath mixedMedalPath;
    DefensePracticeSession mixedMedalSession;
    ShadowbladeActions mixedMedalActions;
    CombatSandbox mixedMedalCombat;
    Expect(mixedMedalPath.RestoreCheckpoint(mixedCheckpoint)
            && mixedMedalPath.ApplyCurrentLesson(mixedMedalSession, mixedMedalActions)
            && mixedMedalSession.QueueNextAttack(mixedMedalCombat, mixedMedalActions)
            && mixedMedalSession.TryDefend(mixedMedalCombat, mixedMedalActions,
                DefenseInput::Guard).result == DefenseResult::Guarded,
        "pass23 mixed-medal regression records one clean ordinary defense");
    EnemyAttackPlan mixedPlan = mixedMedalCombat.PendingEnemyAttack();
    mixedMedalSession.AdvanceTime(
        mixedMedalCombat, mixedMedalActions, mixedPlan.recoverySeconds);
    for (int attempt = 0; attempt < 3; ++attempt) {
        Expect(mixedMedalSession.QueueNextAttack(mixedMedalCombat, mixedMedalActions),
            "pass23 mixed-medal regression queues a canonical follow-up");
        mixedPlan = mixedMedalCombat.PendingEnemyAttack();
        const float perfectLead = mixedPlan.windupSeconds - 0.10f;
        mixedMedalSession.AdvanceTime(
            mixedMedalCombat, mixedMedalActions, perfectLead);
        Expect(mixedMedalSession.TryDefend(mixedMedalCombat, mixedMedalActions,
                DefenseInput::Dodge).result == DefenseResult::PerfectDodge,
            "pass23 mixed-medal regression records a perfect defense");
        mixedMedalSession.AdvanceTime(
            mixedMedalCombat, mixedMedalActions, mixedPlan.recoverySeconds);
    }
    Expect(mixedMedalPath.CurrentLessonComplete(mixedMedalSession)
            && mixedMedalPath.CurrentMedal(mixedMedalSession)
                == ShadowbladeTrainingMedal::Silver,
        "pass23 ordinary plus perfect clean completion cannot receive Gold");

    // QOL-024: player-requested boss-style rehearsal uses the existing three
    // canonical threat types so all three can be practiced in one bounded loop.
    ShadowbladeTrainingCheckpoint bossCheckpoint{};
    bossCheckpoint.completedMask = 15;
    bossCheckpoint.currentLesson = ShadowbladeTrainingLesson::BossRehearsal;
    for (std::size_t index = 0; index < 4; ++index) {
        bossCheckpoint.medals[index] = ShadowbladeTrainingMedal::Bronze;
    }
    ShadowbladeTrainingPath bossPath;
    DefensePracticeSession bossSession;
    ShadowbladeActions bossActions;
    CombatSandbox bossCombat;
    Expect(bossPath.RestoreCheckpoint(bossCheckpoint)
            && bossPath.LessonUnlocked(ShadowbladeTrainingLesson::BossRehearsal)
            && bossPath.ApplyCurrentLesson(bossSession, bossActions)
            && bossSession.PracticeSequenceLength() == 3
            && bossSession.Goal() == DefenseTrainingGoal::FreePractice,
        "pass23 boss rehearsal resumes only after the four prerequisite lessons");

    Expect(bossSession.QueueNextAttack(bossCombat, bossActions)
            && bossCombat.PendingEnemyAttack().pattern == EnemyAttackPattern::QuickCut,
        "pass23 boss rehearsal starts with QuickCut");
    EnemyAttackPlan bossPlan = bossCombat.PendingEnemyAttack();
    Expect(bossSession.TryDefend(bossCombat, bossActions,
            DefenseInput::Guard).result == DefenseResult::Guarded,
        "pass23 boss rehearsal resolves QuickCut with guard");
    bossSession.AdvanceTime(bossCombat, bossActions, bossPlan.recoverySeconds);

    Expect(bossSession.QueueNextAttack(bossCombat, bossActions)
            && bossCombat.PendingEnemyAttack().pattern == EnemyAttackPattern::GuardBreaker,
        "pass23 boss rehearsal advances to GuardBreaker");
    bossPlan = bossCombat.PendingEnemyAttack();
    Expect(bossSession.TryDefend(bossCombat, bossActions,
            DefenseInput::Guard).result == DefenseResult::Guarded,
        "pass23 boss rehearsal resolves GuardBreaker with guard");
    bossSession.AdvanceTime(bossCombat, bossActions, bossPlan.recoverySeconds);

    Expect(bossSession.QueueNextAttack(bossCombat, bossActions)
            && bossCombat.PendingEnemyAttack().pattern == EnemyAttackPattern::RiftBurst,
        "pass23 boss rehearsal advances to unblockable RiftBurst");
    bossPlan = bossCombat.PendingEnemyAttack();
    bossSession.AdvanceTime(bossCombat, bossActions, 0.40f);
    const DefenseResult riftDefense = bossSession.TryDefend(
        bossCombat, bossActions, DefenseInput::Dodge).result;
    Expect(riftDefense == DefenseResult::Evaded || riftDefense == DefenseResult::PerfectDodge,
        "pass23 boss rehearsal requires dodge handling for RiftBurst");
    bossSession.AdvanceTime(bossCombat, bossActions, bossPlan.recoverySeconds);

    const DefensePracticePatternStats bossQuick =
        bossSession.PatternStats(EnemyAttackPattern::QuickCut);
    const DefensePracticePatternStats bossBreaker =
        bossSession.PatternStats(EnemyAttackPattern::GuardBreaker);
    const DefensePracticePatternStats bossRift =
        bossSession.PatternStats(EnemyAttackPattern::RiftBurst);
    Expect(bossQuick.attempts == 1 && bossQuick.ordinaryDefenses == 1
            && bossBreaker.attempts == 1 && bossBreaker.ordinaryDefenses == 1
            && bossRift.attempts == 1
            && bossRift.perfectDefenses + bossRift.ordinaryDefenses == 1,
        "pass23 boss rehearsal records one resolved success for every canonical threat");
    Expect(bossPath.CurrentLessonComplete(bossSession)
            && bossPath.CurrentMedal(bossSession) == ShadowbladeTrainingMedal::Silver
            && bossPath.CommitCurrentLesson(bossSession)
            && bossPath.CurrentLesson() == ShadowbladeTrainingLesson::Complete
            && bossPath.CompletedLessonCount() == 5,
        "pass23 boss rehearsal completes the curriculum with a deterministic silver medal");

    // Review regression: any hit during a completed boss rehearsal caps mastery
    // at Bronze even if the other authored threats are successfully defended.
    ShadowbladeTrainingPath bossMissPath;
    DefensePracticeSession bossMissSession;
    ShadowbladeActions bossMissActions;
    CombatSandbox bossMissCombat;
    Expect(bossMissPath.RestoreCheckpoint(bossCheckpoint)
            && bossMissPath.ApplyCurrentLesson(bossMissSession, bossMissActions)
            && bossMissSession.QueueNextAttack(bossMissCombat, bossMissActions),
        "pass23 boss-miss regression starts the authored rehearsal");
    EnemyAttackPlan bossMissPlan = bossMissCombat.PendingEnemyAttack();
    Expect(bossMissSession.AdvanceTime(
            bossMissCombat, bossMissActions, bossMissPlan.windupSeconds)
            && bossMissSession.PatternStats(EnemyAttackPattern::QuickCut).hitsTaken == 1,
        "pass23 boss-miss regression records the first threat as a hit");
    bossMissSession.AdvanceTime(
        bossMissCombat, bossMissActions, bossMissPlan.recoverySeconds);
    Expect(bossMissSession.QueueNextAttack(bossMissCombat, bossMissActions),
        "pass23 boss-miss regression queues GuardBreaker");
    bossMissPlan = bossMissCombat.PendingEnemyAttack();
    Expect(bossMissSession.TryDefend(bossMissCombat, bossMissActions,
            DefenseInput::Guard).result == DefenseResult::Guarded,
        "pass23 boss-miss regression resolves GuardBreaker successfully");
    bossMissSession.AdvanceTime(
        bossMissCombat, bossMissActions, bossMissPlan.recoverySeconds);
    Expect(bossMissSession.QueueNextAttack(bossMissCombat, bossMissActions),
        "pass23 boss-miss regression queues RiftBurst");
    bossMissPlan = bossMissCombat.PendingEnemyAttack();
    bossMissSession.AdvanceTime(
        bossMissCombat, bossMissActions, bossMissPlan.windupSeconds - 0.10f);
    Expect(bossMissSession.TryDefend(bossMissCombat, bossMissActions,
            DefenseInput::Dodge).result == DefenseResult::PerfectDodge,
        "pass23 boss-miss regression resolves RiftBurst successfully");
    Expect(bossMissPath.CurrentLessonComplete(bossMissSession)
            && bossMissPath.CurrentMedal(bossMissSession)
                == ShadowbladeTrainingMedal::Bronze,
        "pass23 any boss-rehearsal hit prevents Silver or Gold mastery");

    // Review regression: later perfect defenses cannot erase earlier ordinary
    // defenses from boss Gold eligibility. This remains a Silver clean run.
    ShadowbladeTrainingPath bossMixedPath;
    DefensePracticeSession bossMixedSession;
    ShadowbladeActions bossMixedActions;
    CombatSandbox bossMixedCombat;
    Expect(bossMixedPath.RestoreCheckpoint(bossCheckpoint)
            && bossMixedPath.ApplyCurrentLesson(bossMixedSession, bossMixedActions),
        "pass23 boss mixed-mastery regression starts the authored rehearsal");
    for (int pass = 0; pass < 2; ++pass) {
        for (int patternIndex = 0; patternIndex < 3; ++patternIndex) {
            Expect(bossMixedSession.QueueNextAttack(bossMixedCombat, bossMixedActions),
                "pass23 boss mixed-mastery regression queues the authored threat");
            const EnemyAttackPlan plan = bossMixedCombat.PendingEnemyAttack();
            if (pass == 0) {
                if (plan.pattern == EnemyAttackPattern::RiftBurst) {
                    bossMixedSession.AdvanceTime(bossMixedCombat, bossMixedActions, 0.40f);
                    Expect(bossMixedSession.TryDefend(bossMixedCombat, bossMixedActions,
                            DefenseInput::Dodge).result == DefenseResult::Evaded,
                        "pass23 boss mixed-mastery first RiftBurst is an ordinary evade");
                } else {
                    Expect(bossMixedSession.TryDefend(bossMixedCombat, bossMixedActions,
                            DefenseInput::Guard).result == DefenseResult::Guarded,
                        "pass23 boss mixed-mastery first blockable threat is an ordinary guard");
                }
            } else {
                bossMixedSession.AdvanceTime(
                    bossMixedCombat, bossMixedActions, plan.windupSeconds - 0.10f);
                const DefenseResult perfect = bossMixedSession.TryDefend(
                    bossMixedCombat, bossMixedActions, DefenseInput::Dodge).result;
                Expect(perfect == DefenseResult::PerfectDodge,
                    "pass23 boss mixed-mastery second-cycle threat is perfectly dodged");
            }
            bossMixedSession.AdvanceTime(
                bossMixedCombat, bossMixedActions, plan.recoverySeconds);
        }
    }
    Expect(bossMixedPath.CurrentLessonComplete(bossMixedSession)
            && bossMixedPath.CurrentMedal(bossMixedSession)
                == ShadowbladeTrainingMedal::Silver,
        "pass23 boss rehearsal with ordinary defenses cannot later upgrade itself to Gold");

    // Review regression: matching config and resolved metrics are not enough to
    // complete a lesson unless this path instance successfully applied that lesson.
    ShadowbladeTrainingPath unboundBossPath;
    DefensePracticeSession unboundBossSession;
    ShadowbladeActions unboundBossActions;
    CombatSandbox unboundBossCombat;
    const ShadowbladeTrainingLessonPlan unboundBossPlan =
        ShadowbladeTrainingPath::PlanForLesson(ShadowbladeTrainingLesson::BossRehearsal);
    Expect(unboundBossPath.RestoreCheckpoint(bossCheckpoint)
            && unboundBossSession.SetPracticeSequence(unboundBossPlan.sequence)
            && (unboundBossSession.Pace() == unboundBossPlan.pace
                || unboundBossSession.SetPace(unboundBossPlan.pace))
            && (unboundBossSession.Goal() == unboundBossPlan.goal
                || unboundBossSession.SetGoal(unboundBossPlan.goal))
            && (unboundBossSession.GoalTarget() == unboundBossPlan.goalTarget
                || unboundBossSession.SetGoalTarget(unboundBossPlan.goalTarget)),
        "pass23 stale-metric binding regression builds matching boss config without applying it");
    for (int patternIndex = 0; patternIndex < 3; ++patternIndex) {
        Expect(unboundBossSession.QueueNextAttack(unboundBossCombat, unboundBossActions),
            "pass23 stale-metric binding regression queues a matching boss threat");
        const EnemyAttackPlan plan = unboundBossCombat.PendingEnemyAttack();
        if (plan.pattern == EnemyAttackPattern::RiftBurst) {
            unboundBossSession.AdvanceTime(unboundBossCombat, unboundBossActions, 0.40f);
            const DefenseResult dodge = unboundBossSession.TryDefend(
                unboundBossCombat, unboundBossActions, DefenseInput::Dodge).result;
            Expect(dodge == DefenseResult::Evaded || dodge == DefenseResult::PerfectDodge,
                "pass23 stale-metric binding regression resolves RiftBurst");
        } else {
            Expect(unboundBossSession.TryDefend(unboundBossCombat, unboundBossActions,
                    DefenseInput::Guard).result == DefenseResult::Guarded,
                "pass23 stale-metric binding regression resolves a blockable threat");
        }
        unboundBossSession.AdvanceTime(
            unboundBossCombat, unboundBossActions, plan.recoverySeconds);
    }
    Expect(unboundBossSession.PatternStats(EnemyAttackPattern::QuickCut).attempts == 1
            && unboundBossSession.PatternStats(EnemyAttackPattern::GuardBreaker).attempts == 1
            && unboundBossSession.PatternStats(EnemyAttackPattern::RiftBurst).attempts == 1
            && !unboundBossPath.CurrentLessonComplete(unboundBossSession)
            && !unboundBossPath.CommitCurrentLesson(unboundBossSession)
            && unboundBossPath.CurrentLesson() == ShadowbladeTrainingLesson::BossRehearsal,
        "pass23 matching stale metrics cannot complete a lesson that was never applied");

    ShadowbladeTrainingCheckpoint malformedBoss = bossPath.Checkpoint();
    malformedBoss.medals[2] = static_cast<ShadowbladeTrainingMedal>(99);
    Expect(!bossPath.RestoreCheckpoint(malformedBoss)
            && bossPath.CurrentLesson() == ShadowbladeTrainingLesson::Complete,
        "pass23 invalid checkpoint medal fails closed after curriculum completion");
}
