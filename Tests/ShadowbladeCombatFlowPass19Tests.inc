void TestShadowbladeCombatFlowPass19() {
    using namespace Astral::Scene;

    // GAME-092: only perfect defenses award bounded Shadow Momentum.
    ShadowbladeActions momentum;
    for (int attempt = 0; attempt < 4; ++attempt) {
        Expect(momentum.BeginIncomingAttack({0.10f, 20, 20, true}),
            "pass19 momentum fixture accepts a fresh blockable threat");
        const DefenseReport perfect = momentum.TryDefend(DefenseInput::Guard);
        Expect(perfect.result == DefenseResult::PerfectGuard && perfect.counterGranted,
            "pass19 perfect guard preserves the existing counter contract");
    }
    Expect(momentum.ShadowMomentum() == ShadowbladeActions::MaximumShadowMomentum,
        "pass19 repeated perfect defenses saturate Shadow Momentum at three");
    momentum.ResetDefenseState();
    Expect(momentum.ShadowMomentum() == 0,
        "pass19 defense reset clears transient Shadow Momentum");

    Expect(momentum.BeginIncomingAttack({0.50f, 20, 20, true}),
        "pass19 ordinary-guard fixture accepts a threat");
    const DefenseReport ordinaryGuard = momentum.TryDefend(DefenseInput::Guard);
    Expect(ordinaryGuard.result == DefenseResult::Guarded && momentum.ShadowMomentum() == 0,
        "pass19 ordinary guard does not award Shadow Momentum");
    Expect(momentum.BeginIncomingAttack({0.20f, 20, 20, true}),
        "pass19 ordinary-evade fixture accepts a threat");
    const DefenseReport ordinaryEvade = momentum.TryDefend(DefenseInput::Dodge);
    Expect(ordinaryEvade.result == DefenseResult::Evaded && momentum.ShadowMomentum() == 0,
        "pass19 ordinary evade does not award Shadow Momentum");
    Expect(momentum.BeginIncomingAttack({0.10f, 20, 20, false}),
        "pass19 unblockable fixture accepts a threat");
    const DefenseReport unblockable = momentum.TryDefend(DefenseInput::Guard);
    Expect(unblockable.result == DefenseResult::UnblockableHit
            && momentum.ShadowMomentum() == 0,
        "pass19 unblockable impact cannot create Shadow Momentum");

    // GAME-093: one valid Fatal Strike consumes one stack for a bounded damage payoff.
    ShadowbladeActions momentumStrike;
    CombatSandbox momentumCombat;
    Expect(momentumStrike.BeginIncomingAttack({0.10f, 20, 20, true}),
        "pass19 momentum-strike fixture accepts a threat");
    Expect(momentumStrike.TryDefend(DefenseInput::Guard).result
            == DefenseResult::PerfectGuard
            && momentumStrike.ShadowMomentum() == 1,
        "pass19 perfect defense earns exactly one momentum stack");
    const auto rejectedMomentumStrike =
        momentumStrike.TryFatalStrike({-10.0f, 0.0f, 0.0f}, momentumCombat);
    Expect(rejectedMomentumStrike.result == ShadowActionResult::OutOfRange
            && momentumStrike.ShadowMomentum() == 1
            && Near(momentumStrike.Resource(), ShadowbladeActions::MaximumResource),
        "pass19 rejected Fatal Strike preserves momentum and resource");
    const auto boostedStrike =
        momentumStrike.TryFatalStrike({0.0f, 0.0f, 0.0f}, momentumCombat);
    Expect(boostedStrike.result == ShadowActionResult::Activated
            && boostedStrike.damageApplied
                == ShadowbladeActions::FatalStrikeDamage
                    + ShadowbladeActions::MomentumFatalStrikeDamageBonus
            && momentumCombat.Dummy().health
                == 100 - ShadowbladeActions::FatalStrikeDamage
                    - ShadowbladeActions::MomentumFatalStrikeDamageBonus
            && momentumStrike.ShadowMomentum() == 0,
        "pass19 valid Fatal Strike consumes one stack for exactly twelve bonus damage");
    Expect(momentumStrike.BeginIncomingAttack({0.10f, 20, 20, true}),
        "pass19 cooldown-rejection fixture accepts a second threat");
    Expect(momentumStrike.TryDefend(DefenseInput::Dodge).result
            == DefenseResult::PerfectDodge
            && momentumStrike.ShadowMomentum() == 1,
        "pass19 second perfect defense re-earns one momentum stack");
    const auto cooldownRejected =
        momentumStrike.TryFatalStrike({0.0f, 0.0f, 0.0f}, momentumCombat);
    Expect(cooldownRejected.result == ShadowActionResult::Cooldown
            && momentumStrike.ShadowMomentum() == 1,
        "pass19 cooldown rejection cannot consume newly earned momentum");

    CharacterProgression maximumProgression;
    int levelsGained = 0;
    maximumProgression.GrantExperience(100000, levelsGained);
    Expect(maximumProgression.Level() == CharacterProgression::MaximumLevel,
        "pass19 weapon-style fixture reaches the production level cap");

    // GAME-094: Riftsteel improves only earned Fatal Strike follow-up efficiency.
    ShadowbladeActions riftsteel;
    Expect(riftsteel.Loadout().AcquireWeapon(ShadowbladeWeapon::RiftsteelSabre)
            && riftsteel.Loadout().TryEquipWeapon(
                ShadowbladeWeapon::RiftsteelSabre, maximumProgression)
                == LoadoutActionResult::Success,
        "pass19 Riftsteel fixture equips through existing ownership and level gates");
    CombatSandbox riftsteelOrdinaryCombat;
    const auto riftsteelOrdinary =
        riftsteel.TryFatalStrike({0.0f, 0.0f, 0.0f}, riftsteelOrdinaryCombat);
    Expect(riftsteelOrdinary.result == ShadowActionResult::Activated
            && !riftsteelOrdinary.followUp
            && Near(riftsteelOrdinary.resourceSpent, ShadowbladeActions::FatalStrikeCost),
        "pass19 Riftsteel ordinary Fatal Strike keeps the existing base cost");

    ShadowbladeActions riftsteelCounter;
    Expect(riftsteelCounter.Loadout().AcquireWeapon(ShadowbladeWeapon::RiftsteelSabre)
            && riftsteelCounter.Loadout().TryEquipWeapon(
                ShadowbladeWeapon::RiftsteelSabre, maximumProgression)
                == LoadoutActionResult::Success
            && riftsteelCounter.BeginIncomingAttack({0.10f, 20, 20, true})
            && riftsteelCounter.TryDefend(DefenseInput::Dodge).result
                == DefenseResult::PerfectDodge,
        "pass19 Riftsteel counter fixture earns a defense follow-up");
    CombatSandbox riftsteelCounterCombat;
    const auto riftsteelFollowUp =
        riftsteelCounter.TryFatalStrike({0.0f, 0.0f, 0.0f}, riftsteelCounterCombat);
    Expect(riftsteelFollowUp.result == ShadowActionResult::Activated
            && riftsteelFollowUp.followUp
            && Near(riftsteelFollowUp.resourceSpent,
                ShadowbladeActions::DefenseCounterFatalStrikeCost
                    - ShadowbladeActions::RiftsteelFollowUpCostReduction),
        "pass19 Riftsteel defense follow-up costs exactly five less resource");

    // GAME-095: CryoEdge Perfect Guard restores bounded Guard Integrity only.
    ShadowbladeActions cryo;
    Expect(cryo.Loadout().AcquireWeapon(ShadowbladeWeapon::CryoEdge)
            && cryo.Loadout().TryEquipWeapon(
                ShadowbladeWeapon::CryoEdge, maximumProgression)
                == LoadoutActionResult::Success,
        "pass19 CryoEdge fixture equips through existing gates");
    Expect(cryo.BeginIncomingAttack({0.50f, 20, 30, true}),
        "pass19 CryoEdge ordinary-guard fixture accepts a threat");
    const DefenseReport cryoGuard = cryo.TryDefend(DefenseInput::Guard);
    const int guardAfterOrdinary = cryo.GuardIntegrity();
    Expect(cryoGuard.result == DefenseResult::Guarded
            && guardAfterOrdinary < ShadowbladeActions::MaximumGuardIntegrity,
        "pass19 CryoEdge ordinary guard still loses bounded Guard Integrity");
    Expect(cryo.BeginIncomingAttack({0.10f, 20, 30, true}),
        "pass19 CryoEdge perfect-guard fixture accepts a threat");
    const DefenseReport cryoPerfect = cryo.TryDefend(DefenseInput::Guard);
    Expect(cryoPerfect.result == DefenseResult::PerfectGuard
            && cryo.GuardIntegrity()
                == std::min(ShadowbladeActions::MaximumGuardIntegrity,
                    guardAfterOrdinary + ShadowbladeActions::CryoEdgePerfectGuardRestore),
        "pass19 CryoEdge Perfect Guard restores exactly twenty bounded Guard Integrity");
    const int guardBeforeDodge = cryo.GuardIntegrity();
    Expect(cryo.BeginIncomingAttack({0.10f, 20, 30, true}),
        "pass19 CryoEdge dodge isolation fixture accepts a threat");
    Expect(cryo.TryDefend(DefenseInput::Dodge).result == DefenseResult::PerfectDodge
            && cryo.GuardIntegrity() == guardBeforeDodge,
        "pass19 CryoEdge Perfect Dodge does not trigger the guard-recovery perk");

    // GAME-096: Training Blade Perfect Dodge accelerates only the existing dash cooldown.
    ShadowbladeActions trainingPerfect;
    const auto trainingDash = trainingPerfect.TryDash({0.0f, 0.0f, 0.0f});
    Expect(trainingDash.result == ShadowActionResult::Activated
            && Near(trainingPerfect.DashCooldownRemaining(), 1.0f),
        "pass19 Training Blade fixture starts the normal dash cooldown");
    Expect(trainingPerfect.BeginIncomingAttack({0.20f, 20, 20, true}),
        "pass19 Training Blade perfect-dodge fixture accepts a threat");
    trainingPerfect.AdvanceTime(0.10f);
    Expect(Near(trainingPerfect.DashCooldownRemaining(), 0.90f),
        "pass19 ordinary time advancement reduces dash cooldown before defense");
    Expect(trainingPerfect.TryDefend(DefenseInput::Dodge).result
            == DefenseResult::PerfectDodge
            && Near(trainingPerfect.DashCooldownRemaining(), 0.40f),
        "pass19 Training Blade Perfect Dodge removes exactly another half second");

    ShadowbladeActions trainingOrdinary;
    Expect(trainingOrdinary.TryDash({}).result == ShadowActionResult::Activated
            && trainingOrdinary.BeginIncomingAttack({0.20f, 20, 20, true}),
        "pass19 Training Blade ordinary-evade fixture is ready");
    const DefenseReport trainingEvade = trainingOrdinary.TryDefend(DefenseInput::Dodge);
    Expect(trainingEvade.result == DefenseResult::Evaded
            && Near(trainingOrdinary.DashCooldownRemaining(), 1.0f),
        "pass19 ordinary Evade does not receive the Training Blade cooldown perk");

    // QOL-020: one immutable snapshot exposes action readiness without UI-state inference.
    ShadowbladeActions telemetry;
    const ShadowActionReadiness initialReadiness = telemetry.CurrentActionReadiness();
    Expect(Near(initialReadiness.resource, ShadowbladeActions::MaximumResource)
            && Near(initialReadiness.dashCooldownRemaining, 0.0f)
            && Near(initialReadiness.fatalStrikeCooldownRemaining, 0.0f)
            && Near(initialReadiness.dashCooldownNormalized, 0.0f)
            && Near(initialReadiness.fatalStrikeCooldownNormalized, 0.0f)
            && initialReadiness.dashResourceAffordable
            && initialReadiness.fatalStrikeBaseResourceAffordable
            && !initialReadiness.guarding
            && initialReadiness.shadowMomentum == 0,
        "pass19 initial readiness snapshot reports authoritative ready state");
    Expect(telemetry.TryDash({}).result == ShadowActionResult::Activated,
        "pass19 telemetry fixture starts a dash cooldown");
    const ShadowActionReadiness afterDash = telemetry.CurrentActionReadiness();
    Expect(Near(afterDash.resource, 75.0f)
            && Near(afterDash.dashCooldownRemaining, ShadowbladeActions::DashCooldownSeconds)
            && Near(afterDash.dashCooldownNormalized, 1.0f)
            && afterDash.dashResourceAffordable
            && afterDash.fatalStrikeBaseResourceAffordable,
        "pass19 readiness snapshot exposes resource and normalized dash cooldown");
    CombatSandbox telemetryCombat;
    const auto telemetryFatal = telemetry.TryFatalStrike({}, telemetryCombat);
    Expect(telemetryFatal.result == ShadowActionResult::Activated,
        "pass19 telemetry fixture starts a Fatal Strike cooldown");
    telemetry.SetGuarding(true);
    const ShadowActionReadiness afterFatal = telemetry.CurrentActionReadiness();
    Expect(Near(afterFatal.resource, 25.0f)
            && Near(afterFatal.fatalStrikeCooldownRemaining,
                ShadowbladeActions::FatalStrikeCooldownSeconds)
            && Near(afterFatal.fatalStrikeCooldownNormalized, 1.0f)
            && afterFatal.dashResourceAffordable
            && !afterFatal.fatalStrikeBaseResourceAffordable
            && afterFatal.guarding,
        "pass19 readiness snapshot exposes cooldown affordability and guard state");

    // Retry/reset compatibility: preserve persistent loadout, clear transient momentum/actions.
    ShadowbladeActions retry;
    Expect(retry.Loadout().AcquireWeapon(ShadowbladeWeapon::RiftsteelSabre)
            && retry.Loadout().TryEquipWeapon(
                ShadowbladeWeapon::RiftsteelSabre, maximumProgression)
                == LoadoutActionResult::Success
            && retry.BeginIncomingAttack({0.10f, 20, 20, true})
            && retry.TryDefend(DefenseInput::Guard).result == DefenseResult::PerfectGuard
            && retry.ShadowMomentum() == 1,
        "pass19 retry fixture has persistent loadout and transient momentum");
    retry.ResetTransientStatePreservingLoadout();
    Expect(retry.Loadout().EquippedWeapon() == ShadowbladeWeapon::RiftsteelSabre
            && retry.ShadowMomentum() == 0
            && Near(retry.Resource(), ShadowbladeActions::MaximumResource)
            && Near(retry.DashCooldownRemaining(), 0.0f)
            && Near(retry.FatalStrikeCooldownRemaining(), 0.0f),
        "pass19 retry reset preserves loadout while clearing transient combat flow");
}
