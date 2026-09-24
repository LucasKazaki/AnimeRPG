namespace {

void UnlockShadowCryptProductionOwnerPass39(Astral::Scene::LandmarkInteraction& owner,
    Astral::Scene::WorldBlockout& world, Astral::Scene::ShadowbladeActions& actions) {
    using namespace Astral::Scene;
    const Astral::Math::Vec3 positions[3]{
        {-8.0f, 18.0f, 0.0f},
        {4.0f, 39.0f, 0.0f},
        {5.0f, 68.0f, 0.0f},
    };
    for (const Astral::Math::Vec3& position : positions) {
        Expect(owner.TryInteract(position, world, actions).result
                == LandmarkInteractionResult::Discovered,
            "pass39 production setup discovers each National Mall landmark");
    }
    Expect(owner.TryDialogueChoice(DialogueTopic::ShadowCrypt, DialogueChoice::Reassure).loreUnlocked
            && owner.TryDialogueChoice(DialogueTopic::ShadowCrypt,
                    DialogueChoice::ShareEvidence).clueUnlocked
            && owner.FieldGuide().OperationProgress(FieldOperation::ShadowCryptLead).Complete(),
        "pass39 production setup creates both authoritative Shadow Crypt lead witnesses");
}

void DefeatShadowCryptSkirmishTargetPass39(Astral::Scene::ShadowCryptSkirmish& skirmish,
    std::size_t index) {
    using namespace Astral::Scene;
    int guard = 0;
    while (!skirmish.Enemy(index).defeated && guard++ < 32) {
        const ShadowCryptAttackReport report =
            skirmish.AttackTarget(index, ShadowCryptAttackStyle::Heavy);
        Expect(report.accepted, "pass39 bounded defeat fixture keeps heavy attacks accepted");
    }
    Expect(skirmish.Enemy(index).defeated,
        "pass39 bounded defeat fixture defeats the requested target");
}

void DefeatProductionShadowCryptSkirmishPass39(Astral::Scene::LandmarkInteraction& owner) {
    using namespace Astral::Scene;
    for (std::size_t index = 0; index < owner.ShadowCryptSkirmishState().EnemyCount(); ++index) {
        int guard = 0;
        while (!owner.ShadowCryptSkirmishState().Enemy(index).defeated && guard++ < 32) {
            const ShadowCryptAttackReport report =
                owner.AttackShadowCryptSkirmishTarget(index, ShadowCryptAttackStyle::Heavy);
            Expect(report.accepted,
                "pass39 production defeat fixture keeps heavy attacks accepted");
        }
        Expect(owner.ShadowCryptSkirmishState().Enemy(index).defeated,
            "pass39 production defeat fixture defeats every encounter target");
    }
}

void TestShadowCryptSkirmishPass38() {
    using namespace Astral::Scene;

    // Retain pass-38 role/formation coverage and malformed-tier rejection.
    ShadowCryptSkirmish entry;
    Expect(!entry.Begin(static_cast<ShadowCryptSkirmishTier>(255))
            && entry.Begin(ShadowCryptSkirmishTier::EntrySeal)
            && entry.EnemyCount() == 2
            && entry.Enemy(0).role == ShadowCryptEnemyRole::RiftSkirmisher
            && entry.Enemy(1).role == ShadowCryptEnemyRole::GraveboundBulwark,
        "pass39 retains Entry Seal formation and invalid-tier rejection");

    ShadowCryptSkirmish archive;
    Expect(archive.Begin(ShadowCryptSkirmishTier::ArchiveGallery)
            && archive.EnemyCount() == 2
            && archive.Enemy(0).role == ShadowCryptEnemyRole::VeilChanneler,
        "pass39 retains Archive Gallery caster formation");

    ShadowCryptSkirmish nave;
    Expect(nave.Begin(ShadowCryptSkirmishTier::RiftNave)
            && nave.EnemyCount() == ShadowCryptSkirmish::EnemyCapacity
            && nave.Enemy(0).maxHealth > archive.Enemy(0).maxHealth,
        "pass39 retains Rift Nave escalation");

    // Retain pass-38 semantic telegraph, nonfinite timing, exact-boundary,
    // interrupt, and one-shot counter contracts.
    const ShadowCryptThreatTelegraph telegraph = nave.CurrentThreat();
    Expect(telegraph.valid && telegraph.role == ShadowCryptEnemyRole::VeilChanneler
            && telegraph.cue == ShadowCryptThreatCue::Interrupt
            && telegraph.expected == ShadowCryptDefenseResponse::Interrupt
            && telegraph.variant == ShadowCryptThreatVariant::VeilBind
            && telegraph.defensiveCounterAllowed,
        "pass39 retains initial Veil Channeler interrupt semantics");
    const int damageBeforeInvalid = nave.DamageTaken();
    const auto invalidTiming = nave.ResolveThreat(ShadowCryptDefenseResponse::Interrupt,
        std::numeric_limits<double>::infinity());
    Expect(!invalidTiming.accepted && nave.DamageTaken() == damageBeforeInvalid,
        "pass39 nonfinite reaction timing fails closed without mutation");
    const auto interrupted = nave.ResolveThreat(
        ShadowCryptDefenseResponse::Interrupt, telegraph.responseWindowSeconds);
    Expect(interrupted.accepted && interrupted.successful && !interrupted.precision
            && interrupted.interruptedChannel && interrupted.openedCounter
            && interrupted.damageTaken == 0,
        "pass39 exact-boundary interrupt remains successful and non-precision");
    const auto counter = nave.AttackTarget(0, ShadowCryptAttackStyle::Counter);
    const auto duplicateCounter = nave.AttackTarget(0, ShadowCryptAttackStyle::Counter);
    Expect(counter.accepted && counter.healthDamage == 36 && !duplicateCounter.accepted,
        "pass39 ordinary earned counter remains one-shot with legacy damage");

    // Retain pass-38 actual posture delta, bounded stagger consume/recovery,
    // and lethal-stagger signal suppression.
    ShadowCryptSkirmish posture;
    Expect(posture.Begin(ShadowCryptSkirmishTier::RiftNave),
        "pass39 posture fixture begins");
    ShadowCryptAttackReport heavy{};
    for (int i = 0; i < 4; ++i) {
        heavy = posture.AttackTarget(1, ShadowCryptAttackStyle::Heavy);
    }
    Expect(heavy.staggerOpened && heavy.postureDamage == 1
            && posture.Enemy(1).staggered && posture.Enemy(1).posture == 0,
        "pass39 repeated pressure reports only actual posture removed");
    const auto punish = posture.AttackTarget(1, ShadowCryptAttackStyle::Light);
    Expect(punish.staggerConsumed && punish.healthDamage == 40 && punish.postureDamage == 0
            && !posture.Enemy(1).staggered
            && posture.Enemy(1).posture == posture.Enemy(1).maxPosture,
        "pass39 one punish consumes stagger and restores posture without phantom damage");

    ShadowCryptSkirmish lethalStagger;
    Expect(lethalStagger.Begin(ShadowCryptSkirmishTier::EntrySeal),
        "pass39 lethal-stagger fixture begins");
    Expect(lethalStagger.ResolveThreat(ShadowCryptDefenseResponse::Dodge, 0.10).openedCounter,
        "pass39 lethal fixture earns first skirmisher counter");
    Expect(lethalStagger.AttackTarget(0, ShadowCryptAttackStyle::Counter).accepted
            && lethalStagger.AttackTarget(0, ShadowCryptAttackStyle::Light).accepted,
        "pass39 lethal fixture reduces skirmisher health and posture");
    Expect(lethalStagger.ResolveThreat(ShadowCryptDefenseResponse::Guard, 0.10).successful
            && lethalStagger.ResolveThreat(ShadowCryptDefenseResponse::Dodge, 0.10).openedCounter,
        "pass39 repeated role pattern keeps semantic defense contract stable");
    const auto lethalCounter = lethalStagger.AttackTarget(0, ShadowCryptAttackStyle::Counter);
    Expect(lethalCounter.accepted && lethalCounter.defeated
            && !lethalCounter.staggerOpened && !lethalStagger.Enemy(0).staggered,
        "pass39 lethal posture break suppresses impossible stagger follow-up");

    // Retain pass-38 sticky manual focus and deterministic fallback.
    ShadowCryptSkirmish targeting;
    Expect(targeting.Begin(ShadowCryptSkirmishTier::RiftNave),
        "pass39 targeting fixture begins");
    const auto recommended = targeting.RecommendedTarget();
    Expect(recommended.valid && recommended.enemyIndex == 0
            && recommended.role == ShadowCryptEnemyRole::VeilChanneler,
        "pass39 live caster remains deterministic default priority");
    Expect(targeting.LockTarget(1) && targeting.SelectedTarget() == 1,
        "pass39 player can override recommendation with sticky target");
    const auto resolvedOtherThreat = targeting.ResolveThreat(
        ShadowCryptDefenseResponse::Interrupt, 0.25);
    Expect(resolvedOtherThreat.accepted && targeting.SelectedTarget() == 1,
        "pass39 resolving another threat does not steal manual target");
    DefeatShadowCryptSkirmishTargetPass39(targeting, 1);
    Expect(!targeting.HasTargetLock() && targeting.SelectedTarget() != 1,
        "pass39 manual target invalidates after target defeat");

    // GAME-192: every role now owns a deterministic two-variant attack pattern.
    ShadowCryptSkirmish patterns;
    Expect(patterns.Begin(ShadowCryptSkirmishTier::RiftNave)
            && patterns.CurrentThreat().variant == ShadowCryptThreatVariant::VeilBind,
        "GAME-192 Channeler begins on Veil Bind");
    Expect(patterns.ResolveThreat(ShadowCryptDefenseResponse::Interrupt, 0.50).successful
            && patterns.CurrentThreat().variant == ShadowCryptThreatVariant::GraveHammer,
        "GAME-192 Bulwark begins on Grave Hammer");
    Expect(patterns.ResolveThreat(ShadowCryptDefenseResponse::Guard, 0.40).successful
            && patterns.CurrentThreat().variant == ShadowCryptThreatVariant::RiftLunge,
        "GAME-192 Skirmisher begins on Rift Lunge");
    Expect(patterns.ResolveThreat(ShadowCryptDefenseResponse::Dodge, 0.30).successful,
        "GAME-192 first Skirmisher response advances its pattern");
    const auto secondChannel = patterns.CurrentThreat();
    Expect(secondChannel.variant == ShadowCryptThreatVariant::VeilBurst
            && secondChannel.patternStep == 1
            && secondChannel.expected == ShadowCryptDefenseResponse::Interrupt
            && secondChannel.responseWindowSeconds < 0.70
            && secondChannel.failureDamage > 24,
        "GAME-192 Channeler alternates to a faster, harder Veil Burst");

    // GAME-193: low-health enemies enter a bounded desperation state.
    ShadowCryptSkirmish desperation;
    Expect(desperation.Begin(ShadowCryptSkirmishTier::EntrySeal)
            && desperation.AttackTarget(0, ShadowCryptAttackStyle::Heavy).accepted
            && desperation.AttackTarget(0, ShadowCryptAttackStyle::Light).accepted,
        "GAME-193 desperation fixture lowers a living Skirmisher below half health");
    const auto desperateThreat = desperation.CurrentThreat();
    Expect(desperateThreat.enraged
            && desperateThreat.responseWindowSeconds < 0.42
            && desperateThreat.responseWindowSeconds >= 0.20
            && desperateThreat.failureDamage == 24
            && desperateThreat.failureDamage <= 40,
        "GAME-193 desperation tightens timing and raises damage within explicit caps");

    // GAME-194 + GAME-195: very early correct defense is marked precision and
    // upgrades only the one earned counter, while exact-boundary behavior above
    // retains the legacy counter values.
    ShadowCryptSkirmish precision;
    Expect(precision.Begin(ShadowCryptSkirmishTier::RiftNave),
        "GAME-194 precision fixture begins");
    const auto precisionDefense = precision.ResolveThreat(
        ShadowCryptDefenseResponse::Interrupt, 0.05);
    Expect(precisionDefense.accepted && precisionDefense.successful
            && precisionDefense.precision && precisionDefense.openedCounter,
        "GAME-194 very early correct interrupt earns precision");
    const auto precisionCounter = precision.AttackTarget(0, ShadowCryptAttackStyle::Counter);
    Expect(precisionCounter.accepted && precisionCounter.healthDamage == 44
            && precisionCounter.postureDamage == 30
            && !precision.AttackTarget(0, ShadowCryptAttackStyle::Counter).accepted,
        "GAME-195 precision counter gets one bounded health/posture bonus");

    // GAME-196: successful brace against the Bulwark pushes posture without
    // creating a counter the role does not advertise.
    ShadowCryptSkirmish brace;
    Expect(brace.Begin(ShadowCryptSkirmishTier::EntrySeal)
            && brace.ResolveThreat(ShadowCryptDefenseResponse::Dodge, 0.20).successful,
        "GAME-196 brace fixture advances to Gravebound Bulwark");
    const auto braceDefense = brace.ResolveThreat(ShadowCryptDefenseResponse::Guard, 0.20);
    Expect(braceDefense.accepted && braceDefense.successful && !braceDefense.openedCounter
            && braceDefense.postureDamageApplied == 10 && !braceDefense.staggerOpened
            && brace.Enemy(1).posture == 60,
        "GAME-196 successful brace applies exactly ten actual posture pressure");

    // QOL-040: a read-only semantic queue can expose the current and next two
    // living attackers without depending on camera framing or color.
    ShadowCryptSkirmish queue;
    Expect(queue.Begin(ShadowCryptSkirmishTier::RiftNave),
        "QOL-040 queue fixture begins");
    const auto threatQueue = queue.ThreatQueue();
    Expect(threatQueue.count == 3
            && threatQueue.threats[0].enemyIndex == 0
            && threatQueue.threats[0].cue == ShadowCryptThreatCue::Interrupt
            && threatQueue.threats[1].enemyIndex == 1
            && threatQueue.threats[1].cue == ShadowCryptThreatCue::Brace
            && threatQueue.threats[2].enemyIndex == 2
            && threatQueue.threats[2].cue == ShadowCryptThreatCue::Evade
            && queue.CurrentThreat().enemyIndex == 0,
        "QOL-040 threat queue exposes three semantic attackers without mutation");
    Expect(queue.ThreatQueue(0).count == 0 && queue.ThreatQueue(1).count == 1,
        "QOL-040 queue request is explicitly bounded");

    // Production integration retained through the live LandmarkInteraction owner:
    // admission, timeline lock, authoritative damage, objective completion, retry,
    // and replay cleanup remain covered.
    WorldBlockout world;
    ShadowbladeActions actions;
    CharacterProgression progression;
    LandmarkInteraction owner;
    UnlockShadowCryptProductionOwnerPass39(owner, world, actions);
    owner.SetCharacterProgression(&progression);
    Expect(owner.BeginShadowCrypt() && owner.ShadowCryptBriefing().active
            && owner.ShadowCryptBriefing().room == ShadowCryptRoom::EntrySeal,
        "pass39 production owner enters Shadow Crypt");
    Expect(owner.BeginShadowCryptSkirmish()
            && owner.ShadowCryptSkirmishState().Tier() == ShadowCryptSkirmishTier::EntrySeal,
        "pass39 production owner derives skirmish tier from authoritative room");
    Expect(!owner.AdvanceShadowCryptObjective() && !owner.SuspendShadowCrypt()
            && owner.ShadowCryptBriefing().objectiveProgress == 0,
        "pass39 active combat blocks direct objective advancement and suspension");

    const auto failedDefense = owner.ResolveShadowCryptSkirmishThreat(
        ShadowCryptDefenseResponse::Guard, 0.10);
    Expect(failedDefense.accepted && !failedDefense.successful
            && failedDefense.damageTaken == 18
            && owner.ShadowCryptBriefing().damageTaken == 18,
        "pass39 failed defense records authoritative mission damage once");

    int capGuard = 0;
    while (owner.ShadowCryptSkirmishState().DamageTaken() < 9999 && capGuard++ < 600) {
        const int skirmishBefore = owner.ShadowCryptSkirmishState().DamageTaken();
        const int missionBefore = owner.ShadowCryptBriefing().damageTaken;
        const ShadowCryptThreatTelegraph capThreat = owner.ShadowCryptSkirmishThreat();
        const ShadowCryptDefenseReport capReport = owner.ResolveShadowCryptSkirmishThreat(
            ShadowCryptDefenseResponse::None, 0.0);
        const int expectedApplied = std::min(capThreat.failureDamage, 9999 - skirmishBefore);
        Expect(capThreat.valid && capReport.accepted && !capReport.successful
                && capReport.damageTaken == expectedApplied
                && owner.ShadowCryptSkirmishState().DamageTaken()
                    == skirmishBefore + expectedApplied
                && owner.ShadowCryptBriefing().damageTaken == missionBefore + expectedApplied,
            "pass39 capped defense keeps skirmish and mission damage synchronized");
    }
    Expect(owner.ShadowCryptSkirmishState().DamageTaken() == 9999
            && owner.ShadowCryptBriefing().damageTaken == 9999,
        "pass39 skirmish and mission damage agree at skirmish cap");

    LandmarkInteraction missionCapOwner;
    WorldBlockout missionCapWorld;
    ShadowbladeActions missionCapActions;
    CharacterProgression missionCapProgression;
    UnlockShadowCryptProductionOwnerPass39(
        missionCapOwner, missionCapWorld, missionCapActions);
    missionCapOwner.SetCharacterProgression(&missionCapProgression);
    Expect(missionCapOwner.BeginShadowCrypt()
            && missionCapOwner.RecordShadowCryptDamageTaken(999995) == 999995
            && missionCapOwner.BeginShadowCryptSkirmish(),
        "pass39 mission-cap fixture reaches five below authoritative cap");
    const auto missionCapDefense = missionCapOwner.ResolveShadowCryptSkirmishThreat(
        ShadowCryptDefenseResponse::Guard, 0.10);
    Expect(missionCapDefense.accepted && !missionCapDefense.successful
            && missionCapDefense.damageTaken == 5
            && missionCapOwner.ShadowCryptBriefing().damageTaken == 1000000,
        "pass39 defense report exposes only damage accepted by mission cap");

    const int progressBeforeClear = owner.ShadowCryptBriefing().objectiveProgress;
    DefeatProductionShadowCryptSkirmishPass39(owner);
    Expect(owner.ShadowCryptSkirmishState().Complete()
            && owner.ShadowCryptBriefing().objectiveProgress == progressBeforeClear + 1,
        "pass39 first room completion advances exactly one mission objective");
    const auto afterComplete = owner.AttackShadowCryptSkirmishTarget(
        0, ShadowCryptAttackStyle::Heavy);
    Expect(!afterComplete.accepted
            && owner.ShadowCryptBriefing().objectiveProgress == progressBeforeClear + 1,
        "pass39 completed encounter cannot duplicate objective advancement");

    Expect(owner.ReportShadowCryptDefeat()
            && !owner.ShadowCryptSkirmishState().Active()
            && !owner.ShadowCryptSkirmishState().Complete()
            && owner.ShadowCryptSkirmishState().EnemyCount() == 0
            && owner.ShadowCryptBriefing().objectiveProgress == 0,
        "pass39 post-completion defeat clears stale skirmish state");
    Expect(owner.BeginShadowCryptSkirmish()
            && owner.ReportShadowCryptDefeat()
            && !owner.ShadowCryptSkirmishState().Active()
            && !owner.ShadowCryptSkirmishState().Complete()
            && owner.ShadowCryptBriefing().objectiveProgress == 0
            && owner.BeginShadowCryptSkirmish(),
        "pass39 active defeat resets room combat and permits clean retry");

    LandmarkInteraction replayOwner;
    WorldBlockout replayWorld;
    ShadowbladeActions replayActions;
    CharacterProgression replayProgression;
    UnlockShadowCryptProductionOwnerPass39(replayOwner, replayWorld, replayActions);
    replayOwner.SetCharacterProgression(&replayProgression);
    Expect(replayOwner.BeginShadowCrypt() && replayOwner.BeginShadowCryptSkirmish(),
        "pass39 replay fixture begins Entry Seal skirmish");
    DefeatProductionShadowCryptSkirmishPass39(replayOwner);
    int replayGuard = 0;
    while (!replayOwner.ShadowCryptBriefing().complete && replayGuard++ < 16) {
        Expect(replayOwner.AdvanceShadowCryptObjective(),
            "pass39 replay fixture advances remaining accepted objectives");
    }
    Expect(replayOwner.ShadowCryptBriefing().complete
            && replayOwner.ShadowCryptSkirmishState().Complete(),
        "pass39 replay fixture retains completed encounter until replay");
    Expect(replayOwner.ReplayCompletedShadowCrypt()
            && replayOwner.ShadowCryptBriefing().active
            && replayOwner.ShadowCryptBriefing().room == ShadowCryptRoom::EntrySeal
            && !replayOwner.ShadowCryptSkirmishState().Active()
            && !replayOwner.ShadowCryptSkirmishState().Complete()
            && replayOwner.ShadowCryptSkirmishState().EnemyCount() == 0,
        "pass39 replay clears stale skirmish state before new run");
}

} // namespace
