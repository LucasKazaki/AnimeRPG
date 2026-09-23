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

    ShadowbladeTrainingCheckpoint malformedBoss = bossPath.Checkpoint();
    malformedBoss.medals[2] = static_cast<ShadowbladeTrainingMedal>(99);
    Expect(!bossPath.RestoreCheckpoint(malformedBoss)
            && bossPath.CurrentLesson() == ShadowbladeTrainingLesson::Complete,
        "pass23 invalid checkpoint medal fails closed after curriculum completion");
}
