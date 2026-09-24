namespace {

void CompleteMallObjectivePass31(Astral::Scene::LandmarkInteraction& owner,
    Astral::Scene::WorldBlockout& world, Astral::Scene::ShadowbladeActions& actions) {
    using namespace Astral::Scene;
    const Astral::Math::Vec3 positions[3]{
        {-8.0f, 18.0f, 0.0f},
        {4.0f, 39.0f, 0.0f},
        {5.0f, 68.0f, 0.0f},
    };
    for (const Astral::Math::Vec3& position : positions) {
        const LandmarkInteractionReport report = owner.TryInteract(position, world, actions);
        Expect(report.result == LandmarkInteractionResult::Discovered,
            "pass31 setup discovers each National Mall landmark through the production interaction path");
    }
    Expect(owner.ObjectiveComplete(),
        "pass31 setup completes the authoritative National Mall objective prerequisite");
}

void SolveMallResonancePass31(Astral::Scene::LandmarkInteraction& owner,
    Astral::Scene::MallResonanceDifficulty difficulty) {
    using namespace Astral::Scene;
    switch (difficulty) {
    case MallResonanceDifficulty::Survey:
        Expect(owner.RotateMallResonanceAnchor(MallResonanceAnchor::LincolnMemorial).result
                == MallResonanceActionResult::Rotated
                && owner.RotateMallResonanceAnchor(MallResonanceAnchor::ReflectingPool).result
                == MallResonanceActionResult::Rotated
                && owner.RotateMallResonanceAnchor(MallResonanceAnchor::WashingtonMonument).result
                == MallResonanceActionResult::Rotated,
            "pass31 survey solution tunes each landmark resonance once");
        break;
    case MallResonanceDifficulty::Standard:
        Expect(owner.RotateMallResonanceAnchor(MallResonanceAnchor::LincolnMemorial).result
                == MallResonanceActionResult::Rotated
                && owner.RotateMallResonanceAnchor(MallResonanceAnchor::LincolnMemorial).result
                == MallResonanceActionResult::Rotated
                && owner.RotateMallResonanceAnchor(MallResonanceAnchor::ReflectingPool).result
                == MallResonanceActionResult::Rotated
                && owner.RotateMallResonanceAnchor(MallResonanceAnchor::WashingtonMonument).result
                == MallResonanceActionResult::Rotated
                && owner.RotateMallResonanceAnchor(MallResonanceAnchor::WashingtonMonument).result
                == MallResonanceActionResult::Rotated,
            "pass31 standard solution uses its distinct five-move phase target");
        break;
    case MallResonanceDifficulty::Expert:
        Expect(owner.RotateMallResonanceAnchor(MallResonanceAnchor::LincolnMemorial).result
                == MallResonanceActionResult::Rotated
                && owner.RotateMallResonanceAnchor(MallResonanceAnchor::LincolnMemorial).result
                == MallResonanceActionResult::Rotated
                && owner.RotateMallResonanceAnchor(MallResonanceAnchor::ReflectingPool).result
                == MallResonanceActionResult::Rotated
                && owner.RotateMallResonanceAnchor(MallResonanceAnchor::ReflectingPool).result
                == MallResonanceActionResult::Rotated
                && owner.RotateMallResonanceAnchor(MallResonanceAnchor::WashingtonMonument).result
                == MallResonanceActionResult::Rotated,
            "pass31 expert solution uses its distinct landmark phase target");
        break;
    }
}

void TestMallResonancePuzzlePass31() {
    using namespace Astral::Scene;

    // GAME-152 + GAME-153: the puzzle is quest-gated and provides three bounded
    // difficulty profiles with separate deterministic board targets.
    MallResonancePuzzle direct;
    Expect(!direct.Begin(false, MallResonanceDifficulty::Survey)
            && !direct.Begin(true, static_cast<MallResonanceDifficulty>(99))
            && !direct.Begin(true, MallResonanceDifficulty::Survey,
                static_cast<MallResonanceAssistMode>(99)),
        "pass31 direct puzzle rejects missing prerequisites and invalid configuration atomically");
    Expect(direct.Begin(true, MallResonanceDifficulty::Survey)
            && direct.Briefing().state == MallResonanceState::Active
            && direct.Briefing().moves == 0 && direct.Briefing().alignedAnchors == 0,
        "pass31 survey difficulty starts one clean active three-anchor board");
    const MallResonanceActionReport invalidAnchor = direct.Rotate(
        static_cast<MallResonanceAnchor>(99));
    Expect(invalidAnchor.result == MallResonanceActionResult::Invalid
            && direct.Briefing().moves == 0,
        "pass31 invalid anchor ids fail closed without consuming a move");
    Expect(!direct.AdvanceTime(-1.0f)
            && !direct.AdvanceTime(std::numeric_limits<float>::infinity())
            && direct.Briefing().elapsedSeconds == 0.0,
        "pass31 non-positive and nonfinite time deltas cannot mutate the puzzle clock");

    direct.Rotate(MallResonanceAnchor::LincolnMemorial);
    direct.Rotate(MallResonanceAnchor::ReflectingPool);
    direct.Rotate(MallResonanceAnchor::WashingtonMonument);
    Expect(direct.AdvanceTime(30.0f),
        "pass31 finite time advances only the active puzzle clock");
    const MallResonanceActionReport surveyClear = direct.TryStabilize();
    const MallResonanceRecord surveyBest = direct.BestRecord(MallResonanceDifficulty::Survey);
    Expect(surveyClear.result == MallResonanceActionResult::Completed
            && surveyClear.rank == MallResonanceRank::Gold
            && surveyBest.valid && surveyBest.rank == MallResonanceRank::Gold
            && surveyBest.moves == 3 && surveyBest.elapsedSeconds == 30.0,
        "pass31 par survey solution records a deterministic Gold personal best");

    // GAME-154: Guidance is opt-in, read-only assistance. It points to the first
    // mismatched landmark and never rotates a phase on the player's behalf.
    Expect(direct.Begin(true, MallResonanceDifficulty::Standard,
            MallResonanceAssistMode::Guidance),
        "pass31 a completed run can start a new configured difficulty");
    MallResonanceBriefing briefing = direct.Briefing();
    Expect(briefing.guidance.available && !briefing.guidance.readyToStabilize
            && briefing.guidance.anchor == MallResonanceAnchor::LincolnMemorial
            && briefing.guidance.rotationsNeeded == 2
            && briefing.phases[0] == 0 && briefing.moves == 0,
        "pass31 guidance names the next mismatched landmark without mutating the board");
    direct.Rotate(MallResonanceAnchor::LincolnMemorial);
    direct.Rotate(MallResonanceAnchor::LincolnMemorial);
    briefing = direct.Briefing();
    Expect(briefing.guidance.anchor == MallResonanceAnchor::ReflectingPool
            && briefing.guidance.rotationsNeeded == 1 && briefing.moves == 2,
        "pass31 guidance advances deterministically after the player solves one anchor");

    const MallResonanceActionReport mismatch = direct.TryStabilize();
    Expect(mismatch.result == MallResonanceActionResult::Mismatch
            && direct.Briefing().failedStabilizations == 1,
        "pass31 premature stabilization reports a bounded mismatch instead of completing");

    // QOL-032: explicit reset clears only transient puzzle state. The prior Survey
    // best survives, avoiding a relog/teleport-style recovery requirement.
    Expect(direct.AdvanceTime(20.0f) && direct.ResetActiveRun(),
        "pass31 active puzzle exposes an explicit recovery reset");
    briefing = direct.Briefing();
    const MallResonanceRecord retainedSurvey = direct.BestRecord(MallResonanceDifficulty::Survey);
    Expect(briefing.state == MallResonanceState::Active
            && briefing.phases == std::array<int, 3>{0, 0, 0}
            && briefing.moves == 0 && briefing.elapsedSeconds == 0.0
            && briefing.failedStabilizations == 0
            && retainedSurvey.valid && retainedSurvey.score == surveyBest.score
            && !direct.ResetActiveRun() == false,
        "pass31 reset restores a clean active board while preserving the prior personal best");

    // The reset remains active, so finish Standard and then verify GAME-155's
    // per-difficulty personal best and monotonic record retention on a worse replay.
    direct.Rotate(MallResonanceAnchor::LincolnMemorial);
    direct.Rotate(MallResonanceAnchor::LincolnMemorial);
    direct.Rotate(MallResonanceAnchor::ReflectingPool);
    direct.Rotate(MallResonanceAnchor::WashingtonMonument);
    direct.Rotate(MallResonanceAnchor::WashingtonMonument);
    direct.AdvanceTime(50.0f);
    Expect(direct.Briefing().guidance.readyToStabilize,
        "pass31 guidance switches to stabilization once all phases align");
    const MallResonanceActionReport standardClear = direct.TryStabilize();
    const MallResonanceRecord standardBest = direct.BestRecord(MallResonanceDifficulty::Standard);
    Expect(standardClear.result == MallResonanceActionResult::Completed
            && standardBest.valid && standardBest.rank == MallResonanceRank::Gold
            && standardBest.moves == 5
            && direct.BestRecord(MallResonanceDifficulty::Expert).valid == false,
        "pass31 records independent bests per valid difficulty and fails closed for untouched difficulty");

    Expect(direct.Begin(true, MallResonanceDifficulty::Standard),
        "pass31 repeat play can restart a completed difficulty");
    direct.Rotate(MallResonanceAnchor::LincolnMemorial);
    direct.Rotate(MallResonanceAnchor::LincolnMemorial);
    direct.Rotate(MallResonanceAnchor::LincolnMemorial);
    direct.Rotate(MallResonanceAnchor::LincolnMemorial);
    direct.Rotate(MallResonanceAnchor::LincolnMemorial);
    direct.Rotate(MallResonanceAnchor::ReflectingPool);
    direct.Rotate(MallResonanceAnchor::WashingtonMonument);
    direct.Rotate(MallResonanceAnchor::WashingtonMonument);
    direct.AdvanceTime(200.0f);
    Expect(direct.TryStabilize().result == MallResonanceActionResult::Completed
            && direct.BestRecord(MallResonanceDifficulty::Standard).score == standardBest.score,
        "pass31 a slower extra-move replay cannot roll back the retained Standard best");

    // Production-owner integration plus GAME-156: only the completed National Mall
    // objective and one bound persistent progression owner can start/reward runs.
    LandmarkInteraction lockedOwner;
    CharacterProgression lockedProgression;
    lockedOwner.SetCharacterProgression(&lockedProgression);
    Expect(!lockedOwner.BeginMallResonancePuzzle(MallResonanceDifficulty::Expert),
        "pass31 production puzzle remains locked before the National Mall objective is complete");

    WorldBlockout world;
    ShadowbladeActions actions;
    LandmarkInteraction owner;
    CompleteMallObjectivePass31(owner, world, actions);
    Expect(!owner.BeginMallResonancePuzzle(MallResonanceDifficulty::Expert),
        "pass31 production entry also requires a persistent progression owner");

    CharacterProgression progression;
    owner.SetCharacterProgression(&progression);
    Expect(owner.BeginMallResonancePuzzle(MallResonanceDifficulty::Expert,
               MallResonanceAssistMode::Guidance)
            && !owner.BeginMallResonancePuzzle(MallResonanceDifficulty::Survey),
        "pass31 production owner starts exactly one active configured resonance run");
    SolveMallResonancePass31(owner, MallResonanceDifficulty::Expert);
    Expect(owner.AdvanceMallResonancePuzzle(30.0f),
        "pass31 production owner advances only the bounded puzzle clock");
    const MallResonanceActionReport expertClear = owner.StabilizeMallResonancePuzzle();
    const MallResonanceRecord expertBest = owner.MallResonanceBestRecord(
        MallResonanceDifficulty::Expert);
    Expect(expertClear.result == MallResonanceActionResult::Completed
            && expertClear.rank == MallResonanceRank::Gold
            && expertBest.valid && expertBest.moves == 5
            && owner.MallResonancePuzzleBriefing().firstClearRewardAvailable,
        "pass31 production clear records Expert mastery and exposes one first-clear entitlement");

    CharacterProgression unrelated;
    owner.SetCharacterProgression(&unrelated);
    Expect(!owner.ClaimMallResonanceFirstClearReward().granted
            && !owner.BeginMallResonancePuzzle(MallResonanceDifficulty::Survey),
        "pass31 swapping progression pointers cannot redirect reward or replay authority");
    owner.SetCharacterProgression(&progression);
    const MallResonanceRewardReport reward = owner.ClaimMallResonanceFirstClearReward();
    Expect(reward.granted
            && reward.progression.experienceApplied == MallResonancePuzzle::FirstClearExperience
            && reward.progression.masteryPointsApplied == MallResonancePuzzle::FirstClearMastery
            && reward.progression.enhancementMaterialsApplied
                == MallResonancePuzzle::FirstClearEnhancementMaterials
            && !owner.ClaimMallResonanceFirstClearReward().granted,
        "pass31 first clear grants bounded progression exactly once through the bound protagonist owner");

    Expect(owner.BeginMallResonancePuzzle(MallResonanceDifficulty::Survey)
            && owner.ResetMallResonancePuzzle()
            && owner.MallResonanceBestRecord(MallResonanceDifficulty::Expert).score
                == expertBest.score
            && owner.MallResonancePuzzleState().FirstClearRewardClaimed(),
        "pass31 replay and explicit reset preserve the earned Expert record and claimed reward history");
    Expect(!owner.AdvanceMallResonancePuzzle(0.0f)
            && owner.RotateMallResonanceAnchor(static_cast<MallResonanceAnchor>(99)).result
                == MallResonanceActionResult::Invalid,
        "pass31 production wrappers preserve invalid-input and zero-delta safety gates");
}

} // namespace
