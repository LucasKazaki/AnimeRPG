namespace {

void UnlockShadowCryptProductionOwnerPass38(Astral::Scene::LandmarkInteraction& owner,
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
            "pass38 production setup discovers each National Mall landmark");
    }
    Expect(owner.TryDialogueChoice(DialogueTopic::ShadowCrypt, DialogueChoice::Reassure).loreUnlocked
            && owner.TryDialogueChoice(DialogueTopic::ShadowCrypt,
                    DialogueChoice::ShareEvidence).clueUnlocked
            && owner.FieldGuide().OperationProgress(FieldOperation::ShadowCryptLead).Complete(),
        "pass38 production setup creates both authoritative Shadow Crypt lead witnesses");
}

void DefeatShadowCryptSkirmishTargetPass38(Astral::Scene::ShadowCryptSkirmish& skirmish,
    std::size_t index) {
    using namespace Astral::Scene;
    int guard = 0;
    while (!skirmish.Enemy(index).defeated && guard++ < 32) {
        const ShadowCryptAttackReport report =
            skirmish.AttackTarget(index, ShadowCryptAttackStyle::Heavy);
        Expect(report.accepted, "pass38 bounded defeat fixture keeps heavy attacks accepted");
    }
    Expect(skirmish.Enemy(index).defeated,
        "pass38 bounded defeat fixture defeats the requested target");
}

void DefeatProductionShadowCryptSkirmishPass38(Astral::Scene::LandmarkInteraction& owner) {
    using namespace Astral::Scene;
    for (std::size_t index = 0; index < owner.ShadowCryptSkirmishState().EnemyCount(); ++index) {
        int guard = 0;
        while (!owner.ShadowCryptSkirmishState().Enemy(index).defeated && guard++ < 32) {
            const ShadowCryptAttackReport report =
                owner.AttackShadowCryptSkirmishTarget(index, ShadowCryptAttackStyle::Heavy);
            Expect(report.accepted,
                "pass38 production defeat fixture keeps heavy attacks accepted");
        }
        Expect(owner.ShadowCryptSkirmishState().Enemy(index).defeated,
            "pass38 production defeat fixture defeats every encounter target");
    }
}

void TestShadowCryptSkirmishPass38() {
    using namespace Astral::Scene;

    // GAME-187 + GAME-188: three original roles are arranged into bounded,
    // room-scaled formations instead of cloning comparator enemies.
    ShadowCryptSkirmish entry;
    Expect(!entry.Begin(static_cast<ShadowCryptSkirmishTier>(255))
            && entry.Begin(ShadowCryptSkirmishTier::EntrySeal)
            && entry.EnemyCount() == 2
            && entry.Enemy(0).role == ShadowCryptEnemyRole::RiftSkirmisher
            && entry.Enemy(1).role == ShadowCryptEnemyRole::GraveboundBulwark,
        "pass38 Entry Seal exposes its two-role formation and rejects invalid tiers");

    ShadowCryptSkirmish archive;
    Expect(archive.Begin(ShadowCryptSkirmishTier::ArchiveGallery)
            && archive.EnemyCount() == 2
            && archive.Enemy(0).role == ShadowCryptEnemyRole::VeilChanneler,
        "pass38 Archive Gallery introduces the caster role");

    ShadowCryptSkirmish nave;
    Expect(nave.Begin(ShadowCryptSkirmishTier::RiftNave)
            && nave.EnemyCount() == ShadowCryptSkirmish::EnemyCapacity
            && nave.Enemy(0).maxHealth > archive.Enemy(0).maxHealth,
        "pass38 Rift Nave escalates to all three roles with tougher bounded values");

    // GAME-189 + GAME-190: semantic threat cues expose the intended response,
    // exact timing boundary, and one-shot caster interrupt without depending on color.
    const ShadowCryptThreatTelegraph telegraph = nave.CurrentThreat();
    Expect(telegraph.valid && telegraph.role == ShadowCryptEnemyRole::VeilChanneler
            && telegraph.cue == ShadowCryptThreatCue::Interrupt
            && telegraph.expected == ShadowCryptDefenseResponse::Interrupt
            && telegraph.defensiveCounterAllowed,
        "pass38 Veil Channeler publishes a semantic interrupt cue and counterability");
    const int damageBeforeInvalid = nave.DamageTaken();
    const auto invalidTiming = nave.ResolveThreat(ShadowCryptDefenseResponse::Interrupt,
        std::numeric_limits<double>::infinity());
    Expect(!invalidTiming.accepted && nave.DamageTaken() == damageBeforeInvalid,
        "pass38 nonfinite reaction timing fails closed without mutation");
    const auto interrupted = nave.ResolveThreat(
        ShadowCryptDefenseResponse::Interrupt, telegraph.responseWindowSeconds);
    Expect(interrupted.accepted && interrupted.successful && interrupted.interruptedChannel
            && interrupted.openedCounter && interrupted.damageTaken == 0,
        "pass38 exact-boundary interrupt succeeds and opens one earned counter");
    const auto counter = nave.AttackTarget(0, ShadowCryptAttackStyle::Counter);
    const auto duplicateCounter = nave.AttackTarget(0, ShadowCryptAttackStyle::Counter);
    Expect(counter.accepted && counter.healthDamage == 36 && !duplicateCounter.accepted,
        "pass38 caster counter opportunity is one-shot");

    // GAME-191: posture creates a bounded stagger window and the next strike
    // consumes it rather than allowing a permanent stun lock.
    ShadowCryptSkirmish posture;
    Expect(posture.Begin(ShadowCryptSkirmishTier::RiftNave),
        "pass38 posture fixture begins");
    ShadowCryptAttackReport heavy{};
    for (int i = 0; i < 4; ++i) {
        heavy = posture.AttackTarget(1, ShadowCryptAttackStyle::Heavy);
    }
    Expect(heavy.staggerOpened && heavy.postureDamage == 1
            && posture.Enemy(1).staggered && posture.Enemy(1).posture == 0,
        "pass38 repeated heavy pressure reports only actual posture removed and opens stagger at zero");
    const auto punish = posture.AttackTarget(1, ShadowCryptAttackStyle::Light);
    Expect(punish.staggerConsumed && punish.healthDamage == 40 && punish.postureDamage == 0
            && !posture.Enemy(1).staggered
            && posture.Enemy(1).posture == posture.Enemy(1).maxPosture,
        "pass38 one follow-up consumes stagger, reports no phantom posture damage, and restores posture");

    // QOL-039: an explicit manual target lock stays sticky until that target is
    // defeated, then the deterministic role/threat fallback resumes.
    ShadowCryptSkirmish targeting;
    Expect(targeting.Begin(ShadowCryptSkirmishTier::RiftNave),
        "pass38 targeting fixture begins");
    const auto recommended = targeting.RecommendedTarget();
    Expect(recommended.valid && recommended.enemyIndex == 0
            && recommended.role == ShadowCryptEnemyRole::VeilChanneler,
        "pass38 live caster threat receives the deterministic default priority");
    Expect(targeting.LockTarget(1) && targeting.SelectedTarget() == 1,
        "pass38 player can override recommendation with a sticky manual target");
    const auto resolvedOtherThreat = targeting.ResolveThreat(
        ShadowCryptDefenseResponse::Interrupt, 0.25);
    Expect(resolvedOtherThreat.accepted && targeting.SelectedTarget() == 1,
        "pass38 resolving another enemy threat does not silently steal manual target focus");
    DefeatShadowCryptSkirmishTargetPass38(targeting, 1);
    Expect(!targeting.HasTargetLock() && targeting.SelectedTarget() != 1,
        "pass38 manual target invalidates only after the target is defeated");

    // Production integration: a room skirmish is admitted only through the live
    // protagonist-owned Shadow Crypt mission. Failed defense records mission
    // damage, and exactly one completed skirmish advances one objective step.
    WorldBlockout world;
    ShadowbladeActions actions;
    CharacterProgression progression;
    LandmarkInteraction owner;
    UnlockShadowCryptProductionOwnerPass38(owner, world, actions);
    owner.SetCharacterProgression(&progression);
    Expect(owner.BeginShadowCrypt() && owner.ShadowCryptBriefing().active
            && owner.ShadowCryptBriefing().room == ShadowCryptRoom::EntrySeal,
        "pass38 live production owner enters Shadow Crypt before room combat");
    Expect(owner.BeginShadowCryptSkirmish()
            && owner.ShadowCryptSkirmishState().Tier() == ShadowCryptSkirmishTier::EntrySeal,
        "pass38 live production owner derives skirmish tier from authoritative mission room");
    Expect(!owner.AdvanceShadowCryptObjective() && !owner.SuspendShadowCrypt()
            && owner.ShadowCryptBriefing().objectiveProgress == 0,
        "pass38 active room combat blocks direct objective advancement and checkpoint suspension");
    const auto failedDefense = owner.ResolveShadowCryptSkirmishThreat(
        ShadowCryptDefenseResponse::Guard, 0.10);
    Expect(failedDefense.accepted && !failedDefense.successful
            && failedDefense.damageTaken == 18
            && owner.ShadowCryptBriefing().damageTaken == 18,
        "pass38 failed room defense is charged into authoritative mission damage exactly once");

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
            "pass38 capped defense damage reports only the applied delta and keeps mission damage synchronized");
    }
    Expect(owner.ShadowCryptSkirmishState().DamageTaken() == 9999
            && owner.ShadowCryptBriefing().damageTaken == 9999,
        "pass38 skirmish and mission damage remain equal at the skirmish tracking cap");

    const int progressBeforeClear = owner.ShadowCryptBriefing().objectiveProgress;
    DefeatProductionShadowCryptSkirmishPass38(owner);
    Expect(owner.ShadowCryptSkirmishState().Complete()
            && owner.ShadowCryptBriefing().objectiveProgress == progressBeforeClear + 1,
        "pass38 first room skirmish completion advances exactly one mission objective step");
    const auto afterComplete = owner.AttackShadowCryptSkirmishTarget(
        0, ShadowCryptAttackStyle::Heavy);
    Expect(!afterComplete.accepted
            && owner.ShadowCryptBriefing().objectiveProgress == progressBeforeClear + 1,
        "pass38 completed encounter cannot duplicate objective advancement");

    Expect(owner.ReportShadowCryptDefeat()
            && !owner.ShadowCryptSkirmishState().Active()
            && !owner.ShadowCryptSkirmishState().Complete()
            && owner.ShadowCryptSkirmishState().EnemyCount() == 0
            && owner.ShadowCryptBriefing().objectiveProgress == 0,
        "pass38 defeat after completion clears stale inactive skirmish state and resets mission progress");
    Expect(owner.BeginShadowCryptSkirmish(),
        "pass38 clean retry can begin after completed-skirmish defeat cleanup");
    Expect(owner.ReportShadowCryptDefeat()
            && !owner.ShadowCryptSkirmishState().Active()
            && !owner.ShadowCryptSkirmishState().Complete()
            && owner.ShadowCryptBriefing().objectiveProgress == 0
            && owner.BeginShadowCryptSkirmish(),
        "pass38 active defeat cancels room combat, resets mission progress, and permits another clean retry");
}

} // namespace