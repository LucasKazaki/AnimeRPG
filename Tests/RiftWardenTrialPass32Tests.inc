namespace {

void ClearRiftWardenDirect(Astral::Scene::RiftWardenTrial& trial) {
    using namespace Astral::Scene;
    int guard = 0;
    while (trial.Briefing().active && guard++ < 256) {
        const RiftWardenTelegraph telegraph = trial.CurrentTelegraph();
        const RiftWardenResponse response = telegraph.recommendedResponse;
        const RiftWardenActionReport report = trial.Resolve(response, 0.10);
        Expect(report.accepted, "pass32 direct clear accepts the authoritative recommended response");
        if (trial.Briefing().active) {
            Expect(trial.AdvanceTime(0.50),
                "pass32 direct clear advances bounded trial time while the run remains active");
        }
    }
    Expect(guard < 256 && trial.Briefing().complete,
        "pass32 direct clear reaches a deterministic bounded completion");
}

void ClearRiftWardenOwner(Astral::Scene::LandmarkInteraction& owner) {
    using namespace Astral::Scene;
    int guard = 0;
    while (owner.RiftWardenTrialBriefing().active && guard++ < 256) {
        const RiftWardenTelegraph telegraph = owner.RiftWardenTrialBriefing().telegraph;
        const RiftWardenActionReport report = owner.ResolveRiftWardenAction(
            telegraph.recommendedResponse, 0.10);
        Expect(report.accepted,
            "pass32 production clear accepts the current authoritative trial response");
        if (owner.RiftWardenTrialBriefing().active) {
            Expect(owner.AdvanceRiftWardenTrial(0.50),
                "pass32 production clear advances bounded trial time while the run remains active");
        }
    }
    Expect(guard < 256 && owner.RiftWardenTrialBriefing().complete,
        "pass32 production owner reaches a deterministic bounded Warden completion");
}

void TestRiftWardenTrialPass32() {
    using namespace Astral::Scene;

    // GAME-157 + GAME-161: entry is Shadow-Crypt-gated, only Story starts
    // unlocked, and harder tiers unlock monotonically from ranked clears.
    RiftWardenTrial progressionTrial;
    Expect(!progressionTrial.Begin(false, RiftWardenDifficulty::Story),
        "pass32 Warden trial rejects entry without completed Shadow Crypt evidence");
    Expect(!progressionTrial.Begin(true, RiftWardenDifficulty::Standard)
            && !progressionTrial.DifficultyUnlocked(RiftWardenDifficulty::Standard)
            && !progressionTrial.DifficultyUnlocked(RiftWardenDifficulty::Expert),
        "pass32 Warden trial keeps harder difficulties locked before ranked progression");
    Expect(!progressionTrial.BeginPractice(true, RiftWardenDifficulty::Story,
            RiftWardenPhase::Opening),
        "pass32 boss practice is unavailable before a ranked clear");
    Expect(progressionTrial.Begin(true, RiftWardenDifficulty::Story),
        "pass32 completed Shadow Crypt evidence admits the initial Story Warden trial");

    // GAME-158: each phase has an explicit deterministic attack order and the
    // briefing exposes the exact defensive recommendation without mutating it.
    RiftWardenTelegraph telegraph = progressionTrial.CurrentTelegraph();
    Expect(telegraph.attack == RiftWardenAttack::RiftSlash
            && telegraph.recommendedResponse == RiftWardenResponse::Dodge
            && telegraph.sequence == 0,
        "pass32 opening phase begins with deterministic Rift Slash dodge telegraph");
    const RiftWardenTelegraph repeatedTelegraph = progressionTrial.CurrentTelegraph();
    Expect(repeatedTelegraph.attack == telegraph.attack
            && repeatedTelegraph.sequence == telegraph.sequence,
        "pass32 reading the current telegraph is non-mutating");

    // GAME-159: timing is part of defense acceptance. Nonfinite input fails
    // closed, and a late otherwise-correct response becomes a real hit.
    const RiftWardenBriefing beforeInvalid = progressionTrial.Briefing();
    Expect(!progressionTrial.Resolve(RiftWardenResponse::Dodge,
            std::numeric_limits<double>::infinity()).accepted,
        "pass32 nonfinite reaction timing is rejected before combat mutation");
    Expect(progressionTrial.Briefing().bossHealth == beforeInvalid.bossHealth
            && progressionTrial.Briefing().damageTaken == beforeInvalid.damageTaken
            && progressionTrial.Briefing().telegraph.sequence == beforeInvalid.telegraph.sequence,
        "pass32 rejected reaction timing leaves boss/player/sequence state unchanged");
    const RiftWardenActionReport late = progressionTrial.Resolve(
        telegraph.recommendedResponse, telegraph.responseWindowSeconds + 0.01);
    Expect(late.accepted && late.resolution == RiftWardenResolution::Hit
            && late.playerDamage > 0 && progressionTrial.Briefing().damageTaken > 0,
        "pass32 late response is accepted as an authoritative hit rather than a perfect defense");

    // GAME-160: clean defenses fill bounded posture, expose one strike-only
    // punish opening, and consuming/missing that opening resets posture once.
    while (!progressionTrial.Briefing().staggerOpen) {
        telegraph = progressionTrial.CurrentTelegraph();
        Expect(progressionTrial.Resolve(telegraph.recommendedResponse, 0.10).accepted,
            "pass32 on-time recommended defense advances posture toward stagger");
    }
    Expect(progressionTrial.Briefing().posture == RiftWardenTrial::MaximumPosture
            && progressionTrial.CurrentTelegraph().attack == RiftWardenAttack::StaggerOpening
            && progressionTrial.CurrentTelegraph().recommendedResponse == RiftWardenResponse::Strike,
        "pass32 full posture creates one explicit strike-only stagger opening");
    const int healthBeforeMissedOpening = progressionTrial.Briefing().bossHealth;
    const RiftWardenActionReport missed = progressionTrial.Resolve(
        RiftWardenResponse::Guard, 0.10);
    Expect(missed.resolution == RiftWardenResolution::MissedOpening
            && progressionTrial.Briefing().bossHealth == healthBeforeMissedOpening
            && progressionTrial.Briefing().posture == 0
            && progressionTrial.Briefing().missedOpenings == 1
            && !progressionTrial.Briefing().staggerOpen,
        "pass32 missed stagger opening is recorded, deals no boss damage, and resets the one-shot posture window");

    ClearRiftWardenDirect(progressionTrial);
    const RiftWardenRecord storyRecord = progressionTrial.BestRecord(
        RiftWardenDifficulty::Story);
    Expect(storyRecord.valid && storyRecord.mastery != RiftWardenMastery::Unranked
            && storyRecord.missedOpenings == 1
            && progressionTrial.DifficultyUnlocked(RiftWardenDifficulty::Standard)
            && !progressionTrial.DifficultyUnlocked(RiftWardenDifficulty::Expert),
        "pass32 Story clear stores missed-opening evidence and unlocks Standard but not Expert");

    Expect(progressionTrial.Begin(true, RiftWardenDifficulty::Standard),
        "pass32 ranked Story completion admits Standard difficulty");
    ClearRiftWardenDirect(progressionTrial);
    const RiftWardenRecord standardRecord = progressionTrial.BestRecord(
        RiftWardenDifficulty::Standard);
    Expect(standardRecord.valid
            && progressionTrial.DifficultyUnlocked(RiftWardenDifficulty::Expert),
        "pass32 Standard clear stores a separate record and unlocks Expert");

    // Review repair: deliberately skipping stagger openings cannot farm a better
    // score/mastery by accumulating otherwise-perfect defense cycles.
    RiftWardenTrial cleanScoreTrial;
    Expect(cleanScoreTrial.Begin(true, RiftWardenDifficulty::Story),
        "pass32 anti-farming clean baseline begins");
    ClearRiftWardenDirect(cleanScoreTrial);
    const RiftWardenRecord cleanScoreRecord = cleanScoreTrial.BestRecord(
        RiftWardenDifficulty::Story);
    Expect(cleanScoreRecord.valid && cleanScoreRecord.missedOpenings == 0
            && cleanScoreRecord.mastery == RiftWardenMastery::Gold,
        "pass32 clean Story execution establishes a Gold no-miss score baseline");

    RiftWardenTrial missedScoreTrial;
    Expect(missedScoreTrial.Begin(true, RiftWardenDifficulty::Story),
        "pass32 anti-farming missed-opening run begins");
    for (int cycle = 0; cycle < 3; ++cycle) {
        while (!missedScoreTrial.Briefing().staggerOpen) {
            const RiftWardenTelegraph farmTelegraph = missedScoreTrial.CurrentTelegraph();
            Expect(missedScoreTrial.Resolve(farmTelegraph.recommendedResponse, 0.10).accepted,
                "pass32 anti-farming setup earns a legitimate stagger opening");
        }
        Expect(missedScoreTrial.Resolve(RiftWardenResponse::Guard, 0.10).resolution
                == RiftWardenResolution::MissedOpening,
            "pass32 anti-farming setup intentionally skips the stagger opening");
    }
    ClearRiftWardenDirect(missedScoreTrial);
    const RiftWardenRecord missedScoreRecord = missedScoreTrial.BestRecord(
        RiftWardenDifficulty::Story);
    Expect(missedScoreRecord.valid && missedScoreRecord.missedOpenings == 3
            && missedScoreRecord.mastery == RiftWardenMastery::Bronze
            && missedScoreRecord.score < cleanScoreRecord.score,
        "pass32 skipped stagger openings lower mastery/score instead of farming a better ranked record");

    // Exact-head review repair: pathological repeated misses saturate instead of
    // overflowing the signed score penalty and turning a worse Bronze run into
    // a huge positive personal best.
    RiftWardenTrial saturatedScoreTrial;
    Expect(saturatedScoreTrial.Begin(true, RiftWardenDifficulty::Story),
        "pass32 overflow regression begins a bounded Story trial");
    for (int cycle = 0; cycle < RiftWardenTrial::MaximumMissedOpenings + 5; ++cycle) {
        while (!saturatedScoreTrial.Briefing().staggerOpen) {
            const RiftWardenTelegraph farmTelegraph = saturatedScoreTrial.CurrentTelegraph();
            Expect(saturatedScoreTrial.Resolve(farmTelegraph.recommendedResponse, 0.10).accepted,
                "pass32 overflow regression earns the next bounded stagger opening");
        }
        Expect(saturatedScoreTrial.Resolve(RiftWardenResponse::Guard, 0.10).resolution
                == RiftWardenResolution::MissedOpening,
            "pass32 overflow regression intentionally misses the bounded opening");
    }
    Expect(saturatedScoreTrial.Briefing().missedOpenings
            == RiftWardenTrial::MaximumMissedOpenings,
        "pass32 missed-opening evidence saturates at the explicit safe bound");
    ClearRiftWardenDirect(saturatedScoreTrial);
    const RiftWardenRecord saturatedScoreRecord = saturatedScoreTrial.BestRecord(
        RiftWardenDifficulty::Story);
    Expect(saturatedScoreRecord.valid
            && saturatedScoreRecord.missedOpenings == RiftWardenTrial::MaximumMissedOpenings
            && saturatedScoreRecord.mastery == RiftWardenMastery::Bronze
            && saturatedScoreRecord.score == 0,
        "pass32 saturated missed-opening penalty remains bounded at a zero Bronze score");

    // QOL-033: community-requested boss practice can start at a chosen learned
    // phase after a ranked clear. It preserves the ranked personal best and does
    // not create a shortcut for a still-locked difficulty.
    const RiftWardenRecord retainedStory = progressionTrial.BestRecord(
        RiftWardenDifficulty::Story);
    Expect(progressionTrial.BeginPractice(true, RiftWardenDifficulty::Story,
            RiftWardenPhase::Overload),
        "pass32 learned boss practice starts directly at the requested Overload phase");
    RiftWardenBriefing practice = progressionTrial.Briefing();
    Expect(practice.active && practice.practice && practice.phase == RiftWardenPhase::Overload
            && practice.bossHealth * 100 <= practice.bossMaxHealth * 35,
        "pass32 phase practice uses bounded phase-appropriate boss health and marks the run non-ranked");
    ClearRiftWardenDirect(progressionTrial);
    const RiftWardenRecord afterPractice = progressionTrial.BestRecord(
        RiftWardenDifficulty::Story);
    Expect(afterPractice.valid && afterPractice.score == retainedStory.score
            && afterPractice.mastery == retainedStory.mastery,
        "pass32 practice completion cannot overwrite the ranked Story personal best");

    // Production-owner integration: the Warden trial is unavailable on a fresh
    // owner, then consumes the already-authoritative completed Shadow Crypt path.
    WorldBlockout world;
    ShadowbladeActions actions;
    CharacterProgression progression;
    LandmarkInteraction owner;
    owner.SetCharacterProgression(&progression);
    Expect(!owner.BeginRiftWardenTrial(RiftWardenDifficulty::Story),
        "pass32 production owner rejects Warden trial before Shadow Crypt completion");

    const Astral::Math::Vec3 positions[3]{
        {-8.0f, 18.0f, 0.0f},
        {4.0f, 39.0f, 0.0f},
        {5.0f, 68.0f, 0.0f},
    };
    for (const Astral::Math::Vec3& position : positions) {
        Expect(owner.TryInteract(position, world, actions).result
                == LandmarkInteractionResult::Discovered,
            "pass32 setup discovers each National Mall landmark through production interaction");
    }
    owner.TryDialogueChoice(DialogueTopic::ShadowCrypt, DialogueChoice::Reassure);
    owner.TryDialogueChoice(DialogueTopic::ShadowCrypt, DialogueChoice::ShareEvidence);
    Expect(owner.BeginShadowCrypt(),
        "pass32 setup begins the authoritative Shadow Crypt expedition");
    AdvanceShadowCryptPass27ToRiftNave(owner);
    FinishShadowCryptPass27FromRiftNave(owner);
    Expect(owner.ShadowCryptBriefing().complete,
        "pass32 setup completes authoritative Shadow Crypt before Warden entry checks");

    CharacterProgression unrelated;
    owner.SetCharacterProgression(&unrelated);
    Expect(!owner.BeginRiftWardenTrial(RiftWardenDifficulty::Story),
        "pass32 a different protagonist cannot inherit another owner's completed Shadow Crypt Warden unlock");
    owner.SetCharacterProgression(&progression);
    Expect(owner.BeginRiftWardenTrial(RiftWardenDifficulty::Story),
        "pass32 completed Shadow Crypt unlocks the Warden trial for the same persistent protagonist owner");

    owner.SetCharacterProgression(&unrelated);
    const RiftWardenBriefing beforeOwnerSwap = owner.RiftWardenTrialBriefing();
    Expect(!owner.ResolveRiftWardenAction(
            beforeOwnerSwap.telegraph.recommendedResponse, 0.10).accepted
            && !owner.AdvanceRiftWardenTrial(1.0),
        "pass32 protagonist owner swap cannot mutate an active Warden trial");
    owner.SetCharacterProgression(&progression);
    ClearRiftWardenOwner(owner);
    Expect(owner.RiftWardenBestRecord(RiftWardenDifficulty::Story).valid
            && owner.RiftWardenDifficultyUnlocked(RiftWardenDifficulty::Standard),
        "pass32 production owner exposes the ranked record and next difficulty unlock");
}

} // namespace
