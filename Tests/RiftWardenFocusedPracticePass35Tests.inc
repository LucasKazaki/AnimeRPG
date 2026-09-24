namespace {

void ClearRiftWardenPass35Direct(Astral::Scene::RiftWardenTrial& trial) {
    using namespace Astral::Scene;
    int guard = 0;
    while (trial.Briefing().active && guard++ < 256) {
        const RiftWardenTelegraph telegraph = trial.CurrentTelegraph();
        Expect(trial.Resolve(telegraph.recommendedResponse, 0.10).accepted,
            "pass35 direct setup accepts the authoritative Warden response");
        if (trial.Briefing().active) {
            Expect(trial.AdvanceTime(0.25),
                "pass35 direct setup advances bounded Warden time");
        }
    }
    Expect(guard < 256 && trial.Briefing().complete,
        "pass35 direct setup reaches a ranked Warden clear");
}

void ClearRiftWardenPass35Owner(Astral::Scene::LandmarkInteraction& owner) {
    using namespace Astral::Scene;
    int guard = 0;
    while (owner.RiftWardenTrialBriefing().active && guard++ < 256) {
        const RiftWardenTelegraph telegraph = owner.RiftWardenTrialBriefing().telegraph;
        Expect(owner.ResolveRiftWardenAction(telegraph.recommendedResponse, 0.10).accepted,
            "pass35 production setup accepts the authoritative Warden response");
        if (owner.RiftWardenTrialBriefing().active) {
            Expect(owner.AdvanceRiftWardenTrial(0.25),
                "pass35 production setup advances bounded Warden time");
        }
    }
    Expect(guard < 256 && owner.RiftWardenTrialBriefing().complete,
        "pass35 production setup reaches a ranked Warden clear");
}

void TestRiftWardenFocusedPracticePass35() {
    using namespace Astral::Scene;

    RiftWardenTrial trial;
    Expect(!trial.BeginFocusedPractice(true, RiftWardenDifficulty::Story,
            RiftWardenAttack::RiftSlash),
        "pass35 focused boss practice is unavailable before a ranked clear teaches attacks");
    Expect(trial.Begin(true, RiftWardenDifficulty::Story),
        "pass35 ranked setup begins Story Warden trial");
    ClearRiftWardenPass35Direct(trial);

    const RiftWardenRecord rankedBeforePractice = trial.BestRecord(
        RiftWardenDifficulty::Story);
    const RiftWardenAttackGuide slashGuide = trial.AttackGuide(
        RiftWardenAttack::RiftSlash, RiftWardenDifficulty::Story);
    Expect(slashGuide.available && slashGuide.attempts > 0
            && slashGuide.response == RiftWardenResponse::Dodge
            && slashGuide.cue == RiftWardenCue::Evade
            && slashGuide.standardWindowSeconds > 0.0
            && std::abs(slashGuide.practiceWindowSeconds
                - slashGuide.standardWindowSeconds) < 1e-9,
        "pass35 learned-attack guide exposes response, cue, evidence, and standard timing without mutation");
    Expect(!trial.AttackGuide(static_cast<RiftWardenAttack>(255),
            RiftWardenDifficulty::Story).available
            && !trial.AttackGuide(RiftWardenAttack::RiftSlash,
                static_cast<RiftWardenDifficulty>(255)).available,
        "pass35 invalid attack-guide inputs fail closed");
    Expect(!trial.BeginFocusedPractice(true, RiftWardenDifficulty::Story,
            RiftWardenAttack::RiftSlash, static_cast<RiftWardenPracticePace>(255)),
        "pass35 invalid focused-practice pace fails closed");

    // GAME-172/173/174/175: choose one actually learned attack, expose a
    // non-mutating control reminder and learned guide, and apply a wider Guided
    // timing window without changing the ranked record.
    const RiftWardenTrainingRecord slashBeforeOneShot = trial.TrainingRecord(
        RiftWardenAttack::RiftSlash);
    Expect(trial.BeginFocusedPractice(true, RiftWardenDifficulty::Story,
            RiftWardenAttack::RiftSlash, RiftWardenPracticePace::Guided),
        "pass35 starts a one-shot focused drill for a learned Rift Slash");
    const RiftWardenBriefing guided = trial.Briefing();
    Expect(guided.active && guided.practice && guided.focusedPractice
            && !guided.loopingFocusedPractice && !guided.practicePaused
            && guided.practicePace == RiftWardenPracticePace::Guided
            && guided.telegraph.attack == RiftWardenAttack::RiftSlash
            && guided.controlReminder.available
            && guided.controlReminder.attack == guided.telegraph.attack
            && guided.controlReminder.response == guided.telegraph.recommendedResponse
            && guided.controlReminder.cue == guided.telegraph.cue
            && guided.attackGuide.available
            && std::abs(guided.telegraph.responseWindowSeconds
                - guided.attackGuide.standardWindowSeconds
                    * RiftWardenTrial::GuidedPracticeWindowMultiplier) < 1e-9
            && std::abs(guided.attackGuide.practiceWindowSeconds
                - guided.telegraph.responseWindowSeconds) < 1e-9,
        "pass35 guided focused briefing combines target, reminder, learned guide, and adjusted timing");
    const std::uint32_t guidedSequence = guided.telegraph.sequence;
    (void)trial.ControlReminder();
    (void)trial.AttackGuide(RiftWardenAttack::RiftSlash, RiftWardenDifficulty::Story);
    Expect(trial.CurrentTelegraph().sequence == guidedSequence,
        "pass35 reminder and attack-guide reads do not advance the practice drill");

    const RiftWardenActionReport oneShot = trial.Resolve(
        guided.telegraph.recommendedResponse, guided.telegraph.responseWindowSeconds);
    const RiftWardenTrainingRecord slashAfterOneShot = trial.TrainingRecord(
        RiftWardenAttack::RiftSlash);
    Expect(oneShot.accepted && oneShot.resolution == RiftWardenResolution::PerfectDefense
            && oneShot.complete && !trial.Briefing().active && trial.Briefing().complete
            && slashAfterOneShot.attempts == slashBeforeOneShot.attempts + 1
            && slashAfterOneShot.successes == slashBeforeOneShot.successes + 1,
        "pass35 one-shot focused practice records exactly one accepted learned-attack attempt and then completes");
    Expect(SameRiftWardenRecord(
            trial.BestRecord(RiftWardenDifficulty::Story), rankedBeforePractice),
        "pass35 focused practice cannot replace the ranked Story personal best");

    // GAME-176 + QOL-036: a looping focused boss drill may pause cleanly,
    // change practice pace, and repeat one boss move without health/phase drift.
    const RiftWardenAttackGuide gravityGuide = trial.AttackGuide(
        RiftWardenAttack::GravityPulse, RiftWardenDifficulty::Story);
    Expect(gravityGuide.available,
        "pass35 ranked setup learned Gravity Pulse for repeated boss practice");
    Expect(trial.BeginFocusedPractice(true, RiftWardenDifficulty::Story,
            RiftWardenAttack::GravityPulse, RiftWardenPracticePace::Expert, true),
        "pass35 starts an expert looping Gravity Pulse drill");
    RiftWardenBriefing loop = trial.Briefing();
    Expect(loop.active && loop.focusedPractice && loop.loopingFocusedPractice
            && loop.telegraph.attack == RiftWardenAttack::GravityPulse
            && loop.telegraph.recommendedResponse == RiftWardenResponse::Guard
            && loop.telegraph.cue == RiftWardenCue::Brace
            && std::abs(loop.telegraph.responseWindowSeconds
                - loop.attackGuide.standardWindowSeconds
                    * RiftWardenTrial::ExpertPracticeWindowMultiplier) < 1e-9,
        "pass35 expert pace narrows only the active practice timing window");

    const double pausedTime = loop.elapsedSeconds;
    const RiftWardenTrainingRecord gravityBeforePause = trial.TrainingRecord(
        RiftWardenAttack::GravityPulse);
    Expect(trial.SetPracticePaused(true) && trial.Briefing().practicePaused,
        "pass35 focused practice can be paused explicitly");
    Expect(!trial.AdvanceTime(10.0)
            && !trial.Resolve(loop.telegraph.recommendedResponse, 0.10).accepted
            && trial.Briefing().elapsedSeconds == pausedTime
            && trial.TrainingRecord(RiftWardenAttack::GravityPulse).attempts
                == gravityBeforePause.attempts,
        "pass35 pause freezes time and rejects combat resolution without training mutation");
    Expect(!trial.SetPracticePace(static_cast<RiftWardenPracticePace>(255)),
        "pass35 invalid practice pace is rejected while preserving the paused drill");
    Expect(trial.SetPracticePaused(false) && !trial.Briefing().practicePaused,
        "pass35 focused practice resumes explicitly");
    Expect(trial.SetPracticePace(RiftWardenPracticePace::Guided),
        "pass35 active practice can switch from Expert to Guided pace");
    loop = trial.Briefing();
    Expect(std::abs(loop.telegraph.responseWindowSeconds
            - loop.attackGuide.standardWindowSeconds
                * RiftWardenTrial::GuidedPracticeWindowMultiplier) < 1e-9,
        "pass35 pace switch immediately updates the authoritative practice window");

    const int loopBossHealth = loop.bossHealth;
    const RiftWardenPhase loopPhase = loop.phase;
    const RiftWardenRecord recordBeforeLoop = trial.BestRecord(RiftWardenDifficulty::Story);
    const RiftWardenTrainingRecord gravityBeforeLoop = trial.TrainingRecord(
        RiftWardenAttack::GravityPulse);
    for (int repetition = 0; repetition < 12; ++repetition) {
        const RiftWardenTelegraph telegraph = trial.CurrentTelegraph();
        Expect(telegraph.attack == RiftWardenAttack::GravityPulse,
            "pass35 looping practice keeps the selected boss attack fixed");
        const RiftWardenActionReport repeated = trial.Resolve(
            telegraph.recommendedResponse, 0.10);
        Expect(repeated.accepted && repeated.resolution == RiftWardenResolution::PerfectDefense
                && !repeated.complete && trial.Briefing().active,
            "pass35 looping practice accepts repeated correct defenses without ending the drill");
    }
    const RiftWardenTrainingRecord gravityAfterLoop = trial.TrainingRecord(
        RiftWardenAttack::GravityPulse);
    Expect(gravityAfterLoop.attempts == gravityBeforeLoop.attempts + 12
            && gravityAfterLoop.successes == gravityBeforeLoop.successes + 12
            && trial.Briefing().bossHealth == loopBossHealth
            && trial.Briefing().phase == loopPhase
            && trial.Briefing().posture == 0
            && SameRiftWardenRecord(
                trial.BestRecord(RiftWardenDifficulty::Story), recordBeforeLoop),
        "pass35 looping boss practice trains the move repeatedly without boss progression or ranked-record mutation");
    Expect(trial.SetLoopingFocusedPractice(false),
        "pass35 player can exit endless repetition without discarding the active focused drill");
    const RiftWardenTelegraph finalLoopTelegraph = trial.CurrentTelegraph();
    Expect(trial.Resolve(finalLoopTelegraph.recommendedResponse, 0.10).complete
            && !trial.Briefing().active,
        "pass35 the next accepted focused attempt completes after looping is disabled");

    // Focused stagger-opening practice simulates the punish damage while keeping
    // boss health fixed, so even the boss-specific finisher drill remains safe.
    const RiftWardenAttackGuide punishGuide = trial.AttackGuide(
        RiftWardenAttack::StaggerOpening, RiftWardenDifficulty::Story);
    Expect(punishGuide.available,
        "pass35 ranked setup learned the stagger punish opening");
    Expect(trial.BeginFocusedPractice(true, RiftWardenDifficulty::Story,
            RiftWardenAttack::StaggerOpening, RiftWardenPracticePace::Standard, true),
        "pass35 starts a looping stagger-punish drill");
    const int punishPracticeHealth = trial.Briefing().bossHealth;
    const RiftWardenTelegraph punishTelegraph = trial.CurrentTelegraph();
    const RiftWardenActionReport punish = trial.Resolve(
        punishTelegraph.recommendedResponse, punishTelegraph.responseWindowSeconds);
    Expect(punish.accepted && punish.resolution == RiftWardenResolution::Punish
            && punish.bossDamage > 0 && trial.Briefing().bossHealth == punishPracticeHealth
            && trial.Briefing().staggerOpen && trial.Briefing().active,
        "pass35 looping punish practice reports simulated damage but never consumes boss health or the next repeat");
    Expect(trial.SetLoopingFocusedPractice(false),
        "pass35 stagger practice can leave looping mode");
    const RiftWardenTelegraph finalPunish = trial.CurrentTelegraph();
    Expect(trial.Resolve(finalPunish.recommendedResponse, 0.10).complete,
        "pass35 stagger practice completes cleanly after endless mode is disabled");

    // Production-owner regression: the same functionality is reachable only
    // through the persistent protagonist that completed Shadow Crypt and the
    // ranked Warden clear. A swapped owner cannot operate the practice session.
    WorldBlockout world;
    ShadowbladeActions actions;
    CharacterProgression progression;
    LandmarkInteraction owner;
    owner.SetCharacterProgression(&progression);
    const Astral::Math::Vec3 positions[3]{
        {-8.0f, 18.0f, 0.0f},
        {4.0f, 39.0f, 0.0f},
        {5.0f, 68.0f, 0.0f},
    };
    for (const Astral::Math::Vec3& position : positions) {
        Expect(owner.TryInteract(position, world, actions).result
                == LandmarkInteractionResult::Discovered,
            "pass35 production setup discovers each National Mall landmark");
    }
    owner.TryDialogueChoice(DialogueTopic::ShadowCrypt, DialogueChoice::Reassure);
    owner.TryDialogueChoice(DialogueTopic::ShadowCrypt, DialogueChoice::ShareEvidence);
    Expect(owner.BeginShadowCrypt(),
        "pass35 production setup begins Shadow Crypt");
    AdvanceShadowCryptPass27ToRiftNave(owner);
    FinishShadowCryptPass27FromRiftNave(owner);
    Expect(owner.ShadowCryptBriefing().complete
            && owner.BeginRiftWardenTrial(RiftWardenDifficulty::Story),
        "pass35 production setup completes Shadow Crypt and starts ranked Warden trial");
    ClearRiftWardenPass35Owner(owner);
    Expect(owner.BeginFocusedRiftWardenPractice(RiftWardenDifficulty::Story,
            RiftWardenAttack::RiftSlash, RiftWardenPracticePace::Guided, true),
        "pass35 live landmark owner exposes focused learned-attack practice");
    const RiftWardenAttackGuide ownerGuide = owner.RiftWardenAttackGuideFor(
        RiftWardenAttack::RiftSlash, RiftWardenDifficulty::Story);
    Expect(ownerGuide.available && owner.RiftWardenTrialBriefing().controlReminder.available,
        "pass35 live owner exposes the attack guide and current control reminder");

    CharacterProgression unrelated;
    owner.SetCharacterProgression(&unrelated);
    const RiftWardenBriefing beforeOwnerSwap = owner.RiftWardenTrialBriefing();
    Expect(!owner.SetRiftWardenPracticePaused(true)
            && !owner.SetRiftWardenPracticePace(RiftWardenPracticePace::Expert)
            && !owner.SetRiftWardenLoopingFocusedPractice(false)
            && !owner.ResolveRiftWardenAction(
                beforeOwnerSwap.telegraph.recommendedResponse, 0.10).accepted,
        "pass35 swapped protagonist cannot mutate another owner's focused boss practice");
    owner.SetCharacterProgression(&progression);
    Expect(owner.SetRiftWardenPracticePaused(true)
            && owner.SetRiftWardenPracticePaused(false)
            && owner.SetRiftWardenPracticePace(RiftWardenPracticePace::Expert)
            && owner.SetRiftWardenLoopingFocusedPractice(false),
        "pass35 bound protagonist can pause, resume, retune, and leave looping practice");
    const RiftWardenTelegraph ownerFinish = owner.RiftWardenTrialBriefing().telegraph;
    Expect(owner.ResolveRiftWardenAction(ownerFinish.recommendedResponse, 0.10).complete,
        "pass35 production focused drill completes after looping is disabled");
}

} // namespace
