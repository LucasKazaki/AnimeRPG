namespace {

void DefeatShadowCryptPass40(Astral::Scene::ShadowCryptSkirmish& skirmish) {
    using namespace Astral::Scene;
    for (std::size_t index = 0; index < skirmish.EnemyCount(); ++index) {
        int guard = 0;
        while (!skirmish.Enemy(index).defeated && guard++ < 32) {
            const auto report = skirmish.AttackTarget(index, ShadowCryptAttackStyle::Heavy);
            Expect(report.accepted,
                "pass40 bounded defeat fixture keeps heavy attacks accepted");
        }
        Expect(skirmish.Enemy(index).defeated,
            "pass40 bounded defeat fixture defeats each encounter target");
    }
}

void TestShadowCryptSkirmishPass40() {
    using namespace Astral::Scene;

    // GAME-197: role intel exposes an original, deterministic recommended attack
    // without mutating the live encounter.
    ShadowCryptSkirmish intel;
    Expect(intel.Begin(ShadowCryptSkirmishTier::RiftNave),
        "GAME-197 intel fixture begins");
    const auto channelIntel = intel.EnemyIntel(0);
    const auto bulwarkIntel = intel.EnemyIntel(1);
    const auto skirmisherIntel = intel.EnemyIntel(2);
    Expect(channelIntel.valid
            && channelIntel.role == ShadowCryptEnemyRole::VeilChanneler
            && channelIntel.recommendedAttack == ShadowCryptAttackStyle::Light
            && bulwarkIntel.valid
            && bulwarkIntel.recommendedAttack == ShadowCryptAttackStyle::Heavy
            && skirmisherIntel.valid
            && skirmisherIntel.recommendedAttack == ShadowCryptAttackStyle::Heavy
            && !intel.EnemyIntel(ShadowCryptSkirmish::EnemyCapacity).valid
            && intel.CurrentThreat().enemyIndex == 0,
        "GAME-197 role intel is bounded, deterministic, and read-only");

    // GAME-198: a correct semantic defense creates one breach opening on the
    // threat source. Attacking another enemy does not consume that opening.
    const auto opening = intel.ResolveThreat(ShadowCryptDefenseResponse::Interrupt, 0.20);
    Expect(opening.accepted && opening.successful && opening.breachOpened
            && intel.EnemyIntel(0).breachOpening,
        "GAME-198 correct defense opens a source-specific breach");
    const auto unrelatedAttack = intel.AttackTarget(2, ShadowCryptAttackStyle::Light);
    Expect(unrelatedAttack.accepted && !unrelatedAttack.breachConsumed
            && intel.EnemyIntel(0).breachOpening,
        "GAME-198 attacking another enemy preserves the earned breach");

    // GAME-199: using the role's recommended attack consumes the breach once and
    // adds a bounded +10 health / +8 posture payoff. A later attack gets no bonus.
    const auto exploit = intel.AttackTarget(0, ShadowCryptAttackStyle::Light);
    Expect(exploit.accepted && exploit.breachConsumed && exploit.weaknessExploited
            && exploit.healthDamage == 30 && exploit.postureDamage == 20
            && !intel.EnemyIntel(0).breachOpening,
        "GAME-199 matching attack consumes one breach with the bounded payoff");
    const auto ordinaryLight = intel.AttackTarget(0, ShadowCryptAttackStyle::Light);
    Expect(ordinaryLight.accepted && !ordinaryLight.breachConsumed
            && !ordinaryLight.weaknessExploited && ordinaryLight.healthDamage == 20,
        "GAME-199 consumed breach cannot be farmed repeatedly");

    // GAME-200: consecutive successful semantic reads build bounded Shadow Flow;
    // invalid timing does not mutate it and a real failed read resets it.
    ShadowCryptSkirmish flow;
    Expect(flow.Begin(ShadowCryptSkirmishTier::RiftNave),
        "GAME-200 flow fixture begins");
    const auto flow1 = flow.ResolveThreat(ShadowCryptDefenseResponse::Interrupt, 0.20);
    const auto flow2 = flow.ResolveThreat(ShadowCryptDefenseResponse::Guard, 0.20);
    const auto flow3 = flow.ResolveThreat(ShadowCryptDefenseResponse::Dodge, 0.20);
    Expect(flow1.shadowFlow == 1 && flow2.shadowFlow == 2 && flow3.shadowFlow == 3
            && flow.ShadowFlowReady(),
        "GAME-200 three consecutive correct reads fill bounded Shadow Flow");
    const auto invalidFlow = flow.ResolveThreat(ShadowCryptDefenseResponse::Interrupt,
        std::numeric_limits<double>::infinity());
    Expect(!invalidFlow.accepted && flow.ShadowFlowReady(),
        "GAME-200 rejected nonfinite timing cannot consume or reset Shadow Flow");
    const auto failedFlow = flow.ResolveThreat(ShadowCryptDefenseResponse::Guard, 0.0);
    Expect(failedFlow.accepted && !failedFlow.successful && failedFlow.shadowFlow == 0
            && flow.ShadowFlow() == 0 && !flow.ShadowFlowReady(),
        "GAME-200 accepted failed read resets Shadow Flow exactly once");

    // GAME-201: full Shadow Flow is spent only by the next accepted Heavy attack.
    // Consume the Skirmisher breach first with Light so the later Heavy isolates
    // the flow-break bonus from the role weakness bonus.
    ShadowCryptSkirmish flowBreak;
    Expect(flowBreak.Begin(ShadowCryptSkirmishTier::RiftNave)
            && flowBreak.ResolveThreat(ShadowCryptDefenseResponse::Interrupt, 0.20).successful
            && flowBreak.ResolveThreat(ShadowCryptDefenseResponse::Guard, 0.20).successful
            && flowBreak.ResolveThreat(ShadowCryptDefenseResponse::Dodge, 0.20).successful
            && flowBreak.ShadowFlowReady(),
        "GAME-201 flow-break fixture earns the three-read threshold");
    const auto lightDoesNotSpendFlow =
        flowBreak.AttackTarget(2, ShadowCryptAttackStyle::Light);
    Expect(lightDoesNotSpendFlow.accepted && lightDoesNotSpendFlow.breachConsumed
            && !lightDoesNotSpendFlow.weaknessExploited
            && !lightDoesNotSpendFlow.flowBreakConsumed && flowBreak.ShadowFlowReady(),
        "GAME-201 Light attack does not spend full Shadow Flow");
    const auto flowBreakAttack = flowBreak.AttackTarget(0, ShadowCryptAttackStyle::Heavy);
    Expect(flowBreakAttack.accepted && flowBreakAttack.breachConsumed
            && !flowBreakAttack.weaknessExploited && flowBreakAttack.flowBreakConsumed
            && flowBreakAttack.healthDamage == 40 && flowBreakAttack.postureDamage == 38
            && flowBreak.ShadowFlow() == 0,
        "GAME-201 Heavy attack spends Shadow Flow for one bounded finisher bonus");
    const auto laterHeavy = flowBreak.AttackTarget(0, ShadowCryptAttackStyle::Heavy);
    Expect(laterHeavy.accepted && !laterHeavy.flowBreakConsumed
            && laterHeavy.healthDamage == 28,
        "GAME-201 flow-break payoff cannot repeat without rebuilding the meter");

    // QOL-041: a completed room can restart immediately at the same tier. Replay
    // resets transient combat state and cannot recursively replay an active room.
    ShadowCryptSkirmish replay;
    Expect(replay.Begin(ShadowCryptSkirmishTier::EntrySeal),
        "QOL-041 replay fixture begins");
    DefeatShadowCryptPass40(replay);
    Expect(replay.Complete() && !replay.Active(),
        "QOL-041 fixture reaches a completed room");
    const auto completedTier = replay.Tier();
    Expect(replay.Replay() && replay.Active() && replay.ReplayMode()
            && !replay.Complete() && replay.Tier() == completedTier
            && replay.DamageTaken() == 0 && replay.ShadowFlow() == 0
            && replay.Enemy(0).health == replay.Enemy(0).maxHealth
            && !replay.Replay(),
        "QOL-041 immediate replay restores the same room with fresh transient state");
}

} // namespace
