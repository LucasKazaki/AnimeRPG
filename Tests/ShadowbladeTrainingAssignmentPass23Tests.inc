void TestShadowbladeTrainingAssignmentPass23() {
    using namespace Astral::Scene;

    // Final review regression: binding a lesson to an object address is not
    // sufficient because DefensePracticeSession is intentionally copy-assignable.
    // Replacing the applied object with another completed session must invalidate
    // that lesson application instead of accepting the copied metrics.
    ShadowbladeTrainingPath boundPath;
    DefensePracticeSession appliedSession;
    ShadowbladeActions appliedActions;
    Expect(boundPath.ApplyCurrentLesson(appliedSession, appliedActions),
        "pass23 assignment regression binds guard fundamentals to session A");
    const std::uint64_t appliedGeneration = appliedSession.AssignmentGeneration();

    ShadowbladeTrainingPath sourcePath;
    DefensePracticeSession completedSource;
    ShadowbladeActions sourceActions;
    CombatSandbox sourceCombat;
    Expect(sourcePath.ApplyCurrentLesson(completedSource, sourceActions),
        "pass23 assignment regression configures independent session B");
    for (int attempt = 0; attempt < 3; ++attempt) {
        Expect(completedSource.QueueNextAttack(sourceCombat, sourceActions),
            "pass23 assignment regression queues source QuickCut");
        const EnemyAttackPlan plan = sourceCombat.PendingEnemyAttack();
        Expect(completedSource.TryDefend(sourceCombat, sourceActions,
                DefenseInput::Guard).result == DefenseResult::Guarded,
            "pass23 assignment regression completes source guard attempt");
        completedSource.AdvanceTime(sourceCombat, sourceActions, plan.recoverySeconds);
    }
    Expect(sourcePath.CurrentLessonComplete(completedSource),
        "pass23 assignment regression source session contains valid completion metrics");

    appliedSession = completedSource;
    Expect(appliedSession.AssignmentGeneration() != appliedGeneration,
        "pass23 copy assignment changes the destination session generation witness");
    Expect(!boundPath.CurrentLessonComplete(appliedSession)
            && !boundPath.CommitCurrentLesson(appliedSession)
            && boundPath.CurrentLesson() == ShadowbladeTrainingLesson::GuardFundamentals
            && boundPath.CompletedLessonCount() == 0,
        "pass23 copied completion metrics cannot satisfy the earlier lesson application");
}
