namespace {

bool SameRiftWardenRecord(const Astral::Scene::RiftWardenRecord& left,
    const Astral::Scene::RiftWardenRecord& right) {
    return left.valid == right.valid
        && left.score == right.score
        && left.mastery == right.mastery
        && left.damageTaken == right.damageTaken
        && left.perfectDefenses == right.perfectDefenses
        && left.punishes == right.punishes
        && left.missedOpenings == right.missedOpenings
        && left.bestStreak == right.bestStreak
        && left.clearSeconds == right.clearSeconds;
}

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

void VerifyRiftWardenTimingMatrix(Astral::Scene::RiftWardenTrial& trial) {
    using namespace Astral::Scene;
    struct TimingCase {
        RiftWardenDifficulty difficulty;
        RiftWardenPhase phase;
        RiftWardenAttack firstAttack;
        double windowSeconds;
    };
    const TimingCase cases[]{
        {RiftWardenDifficulty::Story, RiftWardenPhase::Opening,
            RiftWardenAttack::RiftSlash, 1.20},
        {RiftWardenDifficulty::Story, RiftWardenPhase::Fracture,
            RiftWardenAttack::GravityPulse, 1.15},
        {RiftWardenDifficulty::Story, RiftWardenPhase::Overload,
            RiftWardenAttack::EchoBurst, 1.10},
        {RiftWardenDifficulty::Standard, RiftWardenPhase::Opening,
            RiftWardenAttack::RiftSlash, 0.90},
        {RiftWardenDifficulty::Standard, RiftWardenPhase::Fracture,
            RiftWardenAttack::GravityPulse, 0.85},
        {RiftWardenDifficulty::Standard, RiftWardenPhase::Overload,
            RiftWardenAttack::EchoBurst, 0.80},
        {RiftWardenDifficulty::Expert, RiftWardenPhase::Opening,
            RiftWardenAttack::RiftSlash, 0.65},
        {RiftWardenDifficulty::Expert, RiftWardenPhase::Fracture,
            RiftWardenAttack::GravityPulse, 0.60},
        {RiftWardenDifficulty::Expert, RiftWardenPhase::Overload,
            RiftWardenAttack::EchoBurst, 0.55},
    };

    for (const TimingCase& timing : cases) {
        Expect(trial.BeginPractice(true, timing.difficulty, timing.phase),
            "pass32 timing matrix starts the requested unlocked difficulty and learned phase");
        const RiftWardenTelegraph boundaryTelegraph = trial.CurrentTelegraph();
        Expect(boundaryTelegraph.attack == timing.firstAttack
                && std::abs(boundaryTelegraph.responseWindowSeconds - timing.windowSeconds) < 1e-9
                && boundaryTelegraph.responseWindowSeconds > 0.0,
            "pass32 timing matrix exposes the expected difficulty- and phase-specific positive window");
        const RiftWardenActionReport boundary = trial.Resolve(
            boundaryTelegraph.recommendedResponse, boundaryTelegraph.responseWindowSeconds);
        Expect(boundary.accepted && boundary.resolution == RiftWardenResolution::PerfectDefense,
            "pass32 exact timing-window boundary remains a successful defense");
        ClearRiftWardenDirect(trial);

        Expect(trial.BeginPractice(true, timing.difficulty, timing.phase),
            "pass32 timing matrix restarts the same practice slice for outside-boundary verification");
        const RiftWardenTelegraph outsideTelegraph = trial.CurrentTelegraph();
        const RiftWardenActionReport outside = trial.Resolve(
            outsideTelegraph.recommendedResponse,
            outsideTelegraph.responseWindowSeconds + 0.000001);
        Expect(outside.accepted && outside.resolution == RiftWardenResolution::Hit
                && outside.playerDamage > 0,
            "pass32 response just outside each timing window becomes an authoritative hit");
        ClearRiftWardenDirect(trial);
    }
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
    Expect(!progressionTrial.Begin(true, static_cast<RiftWardenDifficulty>(255)),
        "pass32 invalid difficulty fails closed before trial mutation");
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

    // GAME-159: negative, nonfinite and invalid response input fail closed with
    // no mutation. A late finite response is accepted as an authoritative hit.
    const RiftWardenBriefing beforeInfinity = progressionTrial.Briefing();
    Expect(!progressionTrial.Resolve(RiftWardenResponse::Dodge,
            std::numeric_limits<double>::infinity()).accepted,
        "pass32 positive-infinite reaction timing is rejected before combat mutation");
    Expect(progressionTrial.Briefing().bossHealth == beforeInfinity.bossHealth
            && progressionTrial.Briefing().damageTaken == beforeInfinity.damageTaken
            && progressionTrial.Briefing().posture == beforeInfinity.posture
            && progressionTrial.Briefing().telegraph.sequence == beforeInfinity.telegraph.sequence,
        "pass32 positive-infinite timing leaves boss/player/posture/sequence state unchanged");

    const RiftWardenBriefing beforeNegative = progressionTrial.Briefing();
    Expect(!progressionTrial.Resolve(RiftWardenResponse::Dodge, -0.01).accepted,
        "pass32 negative reaction timing is rejected before combat mutation");
    Expect(progressionTrial.Briefing().bossHealth == beforeNegative.bossHealth
            && progressionTrial.Briefing().damageTaken == beforeNegative.damageTaken
            && progressionTrial.Briefing().posture == beforeNegative.posture
            && progressionTrial.Briefing().telegraph.sequence == beforeNegative.telegraph.sequence,
        "pass32 negative timing leaves boss/player/posture/sequence state unchanged");

    const RiftWardenBriefing beforeNaN = progressionTrial.Briefing();
    Expect(!progressionTrial.Resolve(RiftWardenResponse::Dodge,
            std::numeric_limits<double>::quiet_NaN()).accepted,
        "pass32 NaN reaction timing is rejected before combat mutation");
    Expect(progressionTrial.Briefing().bossHealth == beforeNaN.bossHealth
            && progressionTrial.Briefing().damageTaken == beforeNaN.damageTaken
            && progressionTrial.Briefing().posture == beforeNaN.posture
            && progressionTrial.Briefing().telegraph.sequence == beforeNaN.telegraph.sequence,
        "pass32 NaN timing leaves boss/player/posture/sequence state unchanged");

    const RiftWardenBriefing beforeInvalidResponse = progressionTrial.Briefing();
    Expect(!progressionTrial.Resolve(static_cast<RiftWardenResponse>(255), 0.10).accepted,
        "pass32 invalid response enum is rejected before combat mutation");
    Expect(progressionTrial.Briefing().bossHealth == beforeInvalidResponse.bossHealth
            && progressionTrial.Briefing().damageTaken == beforeInvalidResponse.damageTaken
            && progressionTrial.Briefing().posture == beforeInvalidResponse.posture
            && progressionTrial.Briefing().telegraph.sequence == beforeInvalidResponse.telegraph.sequence,
        "pass32 invalid response leaves authoritative combat state unchanged");

    const RiftWardenActionReport late = progressionTrial.Resolve(
        telegraph.recommendedResponse, telegraph.responseWindowSeconds + 0.01);
    Expect(late.accepted && late.resolution == RiftWardenResolution::Hit
            && late.playerDamage > 0 && progressionTrial.Briefing().damageTaken > 0,
        "pass32 late response is accepted as an authoritative hit rather than a perfect defense");

    // GAME-160: clean defenses fill bounded posture and expose one strike-only
    // punish opening. Missing it resets posture without boss damage.
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

    // QOL-033: practice is deliberately checked while Expert remains locked so
    // the test can prove practice does not mutate records or difficulty unlocks.
    const RiftWardenRecord recordBeforePractice = progressionTrial.BestRecord(
        RiftWardenDifficulty::Story);
    const bool standardBeforePractice = progressionTrial.DifficultyUnlocked(
        RiftWardenDifficulty::Standard);
    const bool expertBeforePractice = progressionTrial.DifficultyUnlocked(
        RiftWardenDifficulty::Expert);
    Expect(!progressionTrial.BeginPractice(true, RiftWardenDifficulty::Story,
            RiftWardenPhase::Defeated),
        "pass32 defeated state is never a valid practice starting phase");
    Expect(!progressionTrial.BeginPractice(true, RiftWardenDifficulty::Story,
            static_cast<RiftWardenPhase>(255)),
        "pass32 invalid practice phase fails closed");
    Expect(progressionTrial.BeginPractice(true, RiftWardenDifficulty::Story,
            RiftWardenPhase::Overload),
        "pass32 learned boss practice starts directly at the requested Overload phase");
    const RiftWardenBriefing practice = progressionTrial.Briefing();
    Expect(practice.active && practice.practice && practice.phase == RiftWardenPhase::Overload
            && practice.bossHealth * 100 <= practice.bossMaxHealth * 35,
        "pass32 phase practice uses bounded phase-appropriate boss health and marks the run non-ranked");
    ClearRiftWardenDirect(progressionTrial);
    const RiftWardenRecord recordAfterPractice = progressionTrial.BestRecord(
        RiftWardenDifficulty::Story);
    Expect(SameRiftWardenRecord(recordAfterPractice, recordBeforePractice)
            && progressionTrial.DifficultyUnlocked(RiftWardenDifficulty::Standard)
                == standardBeforePractice
            && progressionTrial.DifficultyUnlocked(RiftWardenDifficulty::Expert)
                == expertBeforePractice,
        "pass32 practice preserves every ranked record field and all difficulty unlock bits");

    const RiftWardenRecord storyBeforeStandard = progressionTrial.BestRecord(
        RiftWardenDifficulty::Story);
    Expect(progressionTrial.Begin(true, RiftWardenDifficulty::Standard),
        "pass32 ranked Story completion admits Standard difficulty");
    ClearRiftWardenDirect(progressionTrial);
    const RiftWardenRecord standardRecord = progressionTrial.BestRecord(
        RiftWardenDifficulty::Standard);
    const RiftWardenRecord storyAfterStandard = progressionTrial.BestRecord(
        RiftWardenDifficulty::Story);
    Expect(standardRecord.valid
            && SameRiftWardenRecord(storyAfterStandard, storyBeforeStandard)
            && progressionTrial.DifficultyUnlocked(RiftWardenDifficulty::Expert),
        "pass32 Standard clear stores a separate record without overwriting Story and unlocks Expert");

    // GAME-159 exact matrix coverage: all nine difficulty/phase windows are
    // positive, phase-specific, difficulty-specific, inclusive on the boundary,
    // and rejecting just-outside responses as hits.
    VerifyRiftWardenTimingMatrix(progressionTrial);

    // GAME-160 successful punish path is one-shot even at the exact stagger
    // boundary. A second Strike after consumption is ordinary wrong input.
    RiftWardenTrial punishTrial;
    Expect(punishTrial.Begin(true, RiftWardenDifficulty::Story),
        "pass32 one-shot punish regression begins");
    while (!punishTrial.Briefing().staggerOpen) {
        const RiftWardenTelegraph defense = punishTrial.CurrentTelegraph();
        Expect(punishTrial.Resolve(defense.recommendedResponse, 0.10).accepted,
            "pass32 one-shot punish setup reaches the stagger opening");
    }
    const int beforePunishHealth = punishTrial.Briefing().bossHealth;
    const RiftWardenActionReport punish = punishTrial.Resolve(
        RiftWardenResponse::Strike, RiftWardenTrial::StaggerResponseWindowSeconds);
    Expect(punish.accepted && punish.resolution == RiftWardenResolution::Punish
            && punish.bossDamage > 0
            && punishTrial.Briefing().bossHealth < beforePunishHealth
            && punishTrial.Briefing().posture == 0
            && !punishTrial.Briefing().staggerOpen,
        "pass32 exact-boundary strike consumes the stagger once and resets posture");
    const int afterPunishHealth = punishTrial.Briefing().bossHealth;
    const RiftWardenActionReport duplicatePunish = punishTrial.Resolve(
        RiftWardenResponse::Strike, 0.10);
    Expect(duplicatePunish.accepted
            && duplicatePunish.resolution == RiftWardenResolution::Hit
            && punishTrial.Briefing().bossHealth == afterPunishHealth,
        "pass32 consumed stagger cannot be punished twice");

    // Review repair: deliberately skipping stagger openings lowers the actual
    // run quality, and replaying that worse run on the same trial cannot replace
    // the already-established Gold personal best.
    RiftWardenTrial cleanScoreTrial;
    Expect(cleanScoreTrial.Begin(true, RiftWardenDifficulty::Story),
        "pass32 anti-farming clean baseline begins");
    ClearRiftWardenDirect(cleanScoreTrial);
    const RiftWardenRecord cleanScoreRecord = cleanScoreTrial.BestRecord(
        RiftWardenDifficulty::Story);
    Expect(cleanScoreRecord.valid && cleanScoreRecord.missedOpenings == 0
            && cleanScoreRecord.mastery == RiftWardenMastery::Gold,
        "pass32 clean Story execution establishes a Gold no-miss score baseline");

    Expect(cleanScoreTrial.Begin(true, RiftWardenDifficulty::Story),
        "pass32 worse same-owner replay begins on the trial holding the Gold record");
    for (int cycle = 0; cycle < 3; ++cycle) {
        while (!cleanScoreTrial.Briefing().staggerOpen) {
            const RiftWardenTelegraph farmTelegraph = cleanScoreTrial.CurrentTelegraph();
            Expect(cleanScoreTrial.Resolve(farmTelegraph.recommendedResponse, 0.10).accepted,
                "pass32 worse replay earns a legitimate stagger opening");
        }
        Expect(cleanScoreTrial.Resolve(RiftWardenResponse::Guard, 0.10).resolution
                == RiftWardenResolution::MissedOpening,
            "pass32 worse replay intentionally skips the stagger opening");
    }
    ClearRiftWardenDirect(cleanScoreTrial);
    Expect(SameRiftWardenRecord(cleanScoreTrial.BestRecord(RiftWardenDifficulty::Story),
            cleanScoreRecord),
        "pass32 lower-mastery replay on the same trial cannot roll back the Gold personal best");

    RiftWardenTrial missedScoreTrial;
    Expect(missedScoreTrial.Begin(true, RiftWardenDifficulty::Story),
        "pass32 anti-farming independent missed-opening run begins");
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
        "pass32 skipped stagger openings lower mastery and score instead of farming a better ranked record");

    // Pathological repeated misses saturate before signed score overflow, and a
    // later run beyond the saturation sentinel cannot win uncertain tie-breaks.
    RiftWardenTrial saturatedScoreTrial;
    Expect(saturatedScoreTrial.Begin(true, RiftWardenDifficulty::Story),
        "pass32 overflow regression begins a bounded Story trial");
    const RiftWardenTelegraph firstSaturatedTelegraph = saturatedScoreTrial.CurrentTelegraph();
    const RiftWardenResponse wrongFirstResponse = firstSaturatedTelegraph.recommendedResponse
            == RiftWardenResponse::Dodge
        ? RiftWardenResponse::Guard
        : RiftWardenResponse::Dodge;
    Expect(saturatedScoreTrial.Resolve(wrongFirstResponse, 0.10).resolution
            == RiftWardenResolution::Hit,
        "pass32 saturation baseline intentionally records one authoritative hit");
    for (int cycle = 0; cycle < RiftWardenTrial::MaximumMissedOpenings; ++cycle) {
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
    const RiftWardenRecord firstSaturatedRecord = saturatedScoreTrial.BestRecord(
        RiftWardenDifficulty::Story);
    Expect(firstSaturatedRecord.valid
            && firstSaturatedRecord.missedOpenings == RiftWardenTrial::MaximumMissedOpenings
            && firstSaturatedRecord.damageTaken > 0
            && firstSaturatedRecord.mastery == RiftWardenMastery::Bronze
            && firstSaturatedRecord.score == 0,
        "pass32 saturated baseline remains a bounded zero-score Bronze record");

    Expect(saturatedScoreTrial.Begin(true, RiftWardenDifficulty::Story),
        "pass32 saturation ordering regression starts a later Story replay");
    for (int cycle = 0; cycle < RiftWardenTrial::MaximumMissedOpenings + 5; ++cycle) {
        while (!saturatedScoreTrial.Briefing().staggerOpen) {
            const RiftWardenTelegraph farmTelegraph = saturatedScoreTrial.CurrentTelegraph();
            Expect(saturatedScoreTrial.Resolve(farmTelegraph.recommendedResponse, 0.10).accepted,
                "pass32 saturation ordering replay earns the next stagger opening");
        }
        Expect(saturatedScoreTrial.Resolve(RiftWardenResponse::Guard, 0.10).resolution
                == RiftWardenResolution::MissedOpening,
            "pass32 saturation ordering replay intentionally misses beyond the cap");
    }
    ClearRiftWardenDirect(saturatedScoreTrial);
    const RiftWardenRecord retainedSaturatedRecord = saturatedScoreTrial.BestRecord(
        RiftWardenDifficulty::Story);
    Expect(retainedSaturatedRecord.valid
            && retainedSaturatedRecord.missedOpenings == firstSaturatedRecord.missedOpenings
            && retainedSaturatedRecord.damageTaken == firstSaturatedRecord.damageTaken
            && retainedSaturatedRecord.score == firstSaturatedRecord.score,
        "pass32 later unknown-or-worse saturated replay cannot replace the established personal best through damage/time ties");

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
