namespace {

void TestShadowCryptSkirmishPass41() {
    using namespace Astral::Scene;

    // GAME-202: a missed Rift Skirmisher read leaves a one-shot Rift Mark.
    // The authoritative next telegraph includes its bounded +6 damage so mission
    // accounting and presentation can stay synchronized with actual resolution.
    ShadowCryptSkirmish marked;
    Expect(marked.Begin(ShadowCryptSkirmishTier::EntrySeal),
        "GAME-202 mark fixture begins");
    const auto missedRift = marked.ResolveThreat(ShadowCryptDefenseResponse::Guard, 0.10);
    Expect(missedRift.accepted && !missedRift.successful
            && missedRift.damageTaken == 18 && missedRift.pressureApplied,
        "GAME-202 missed Rift read applies the bounded mark after base damage");
    const auto markedLedger = marked.StatusLedger();
    Expect(markedLedger.count == 1
            && markedLedger.entries[0].kind == ShadowCryptStatusKind::RiftMark
            && markedLedger.entries[0].sourceEnemyIndex == 0
            && markedLedger.entries[0].targetEnemyIndex == ShadowCryptSkirmish::EnemyCapacity
            && markedLedger.entries[0].magnitude == 6
            && marked.CurrentThreat().failureDamage == 36,
        "GAME-202 Rift Mark exposes its source and exact next-failure surcharge");
    const auto markedFailure = marked.ResolveThreat(ShadowCryptDefenseResponse::Dodge, 0.10);
    Expect(markedFailure.accepted && !markedFailure.successful
            && markedFailure.damageTaken == 36,
        "GAME-202 next failed read consumes the advertised Rift Mark surcharge");

    // GAME-203: a missed Veil Channeler interrupt creates a one-shot 14-point
    // ward on the deterministic lowest-health-ratio ally. The ward absorbs only
    // health damage, reports the actual absorbed amount, and cannot go negative.
    ShadowCryptSkirmish warded;
    Expect(warded.Begin(ShadowCryptSkirmishTier::ArchiveGallery),
        "GAME-203 ward fixture begins");
    const auto missedVeil = warded.ResolveThreat(ShadowCryptDefenseResponse::Guard, 0.10);
    const auto wardLedger = warded.StatusLedger();
    Expect(missedVeil.accepted && !missedVeil.successful && missedVeil.pressureApplied
            && wardLedger.count == 1
            && wardLedger.entries[0].kind == ShadowCryptStatusKind::VeilWard
            && wardLedger.entries[0].sourceEnemyIndex == 0
            && wardLedger.entries[0].targetEnemyIndex == 1
            && wardLedger.entries[0].magnitude == 14,
        "GAME-203 Channeler miss wards the deterministic ally for fourteen");
    const auto wardHit = warded.AttackTarget(1, ShadowCryptAttackStyle::Light);
    Expect(wardHit.accepted && wardHit.wardAbsorbed == 14
            && wardHit.healthDamage == 6 && warded.StatusLedger().count == 0,
        "GAME-203 ward absorbs exactly fourteen health damage once");

    // GAME-204: a missed Bulwark brace creates one guard link to the highest
    // priority living ally. The link protects exactly ten posture on one attack.
    ShadowCryptSkirmish guarded;
    Expect(guarded.Begin(ShadowCryptSkirmishTier::EntrySeal)
            && guarded.ResolveThreat(ShadowCryptDefenseResponse::Dodge, 0.10).successful,
        "GAME-204 guard-link fixture advances to the Bulwark");
    Expect(guarded.AttackTarget(0, ShadowCryptAttackStyle::Light).accepted,
        "GAME-204 fixture consumes the earlier Skirmisher breach without matching it");
    const auto missedBulwark = guarded.ResolveThreat(ShadowCryptDefenseResponse::Dodge, 0.10);
    const auto guardLedger = guarded.StatusLedger();
    Expect(missedBulwark.accepted && !missedBulwark.successful
            && missedBulwark.pressureApplied && guardLedger.count == 1
            && guardLedger.entries[0].kind == ShadowCryptStatusKind::BulwarkGuardLink
            && guardLedger.entries[0].sourceEnemyIndex == 1
            && guardLedger.entries[0].targetEnemyIndex == 0
            && guardLedger.entries[0].magnitude == 10,
        "GAME-204 Bulwark miss protects its highest-priority living ally");
    const auto protectedHit = guarded.AttackTarget(0, ShadowCryptAttackStyle::Heavy);
    Expect(protectedHit.accepted && protectedHit.postureProtected == 10
            && protectedHit.postureDamage == 18 && guarded.StatusLedger().count == 0,
        "GAME-204 guard link reduces one posture hit by exactly ten then expires");

    // GAME-205: target recommendation reacts to an active enemy-created pressure
    // source instead of blindly retaining the static role ordering.
    ShadowCryptSkirmish pressureTarget;
    Expect(pressureTarget.Begin(ShadowCryptSkirmishTier::EntrySeal)
            && pressureTarget.ResolveThreat(ShadowCryptDefenseResponse::Dodge, 0.10).successful
            && pressureTarget.AttackTarget(0, ShadowCryptAttackStyle::Light).accepted,
        "GAME-205 pressure-target fixture reaches the Bulwark read");
    const auto baselineTarget = pressureTarget.RecommendedTarget();
    Expect(baselineTarget.valid && baselineTarget.enemyIndex == 0,
        "GAME-205 Skirmisher is the default remaining priority before support pressure");
    Expect(!pressureTarget.ResolveThreat(ShadowCryptDefenseResponse::Dodge, 0.10).successful,
        "GAME-205 failed Bulwark read creates support pressure");
    const auto pressureRecommendation = pressureTarget.RecommendedTarget();
    Expect(pressureRecommendation.valid && pressureRecommendation.enemyIndex == 1
            && pressureRecommendation.role == ShadowCryptEnemyRole::GraveboundBulwark
            && pressureRecommendation.priority > baselineTarget.priority,
        "GAME-205 active support pressure raises its source above the static priority");

    // GAME-206: status pressure has two skillful removal routes. A matching
    // breach exploit can purge the source's pressure, and a precision semantic
    // defense against that source can do the same without weakening timing gates.
    ShadowCryptSkirmish breachPurge;
    Expect(breachPurge.Begin(ShadowCryptSkirmishTier::ArchiveGallery),
        "GAME-206 breach purge fixture begins");
    Expect(!breachPurge.ResolveThreat(ShadowCryptDefenseResponse::Guard, 0.10).successful
            && breachPurge.ResolveThreat(ShadowCryptDefenseResponse::Dodge, 0.20).successful,
        "GAME-206 fixture creates a ward and cycles back to the Channeler");
    const auto nonPrecision = breachPurge.ResolveThreat(
        ShadowCryptDefenseResponse::Interrupt, 0.50);
    Expect(nonPrecision.accepted && nonPrecision.successful && !nonPrecision.precision
            && !nonPrecision.pressurePurged && breachPurge.StatusLedger().count == 1,
        "GAME-206 ordinary successful defense keeps existing source pressure");
    const auto purgeAttack = breachPurge.AttackTarget(0, ShadowCryptAttackStyle::Light);
    Expect(purgeAttack.accepted && purgeAttack.weaknessExploited
            && purgeAttack.pressurePurged && breachPurge.StatusLedger().count == 0,
        "GAME-206 matching breach exploit purges pressure sourced by that enemy");

    ShadowCryptSkirmish precisionPurge;
    Expect(precisionPurge.Begin(ShadowCryptSkirmishTier::ArchiveGallery),
        "GAME-206 precision purge fixture begins");
    Expect(!precisionPurge.ResolveThreat(ShadowCryptDefenseResponse::Guard, 0.10).successful
            && precisionPurge.ResolveThreat(ShadowCryptDefenseResponse::Dodge, 0.20).successful,
        "GAME-206 precision fixture creates the same ward state");
    const auto precision = precisionPurge.ResolveThreat(
        ShadowCryptDefenseResponse::Interrupt, 0.05);
    Expect(precision.accepted && precision.successful && precision.precision
            && precision.pressurePurged && precisionPurge.StatusLedger().count == 0,
        "GAME-206 precision read purges the threat source's active pressure");

    // QOL-042: the read-only ledger names every active pressure source, target,
    // and exact magnitude. It is deterministic, bounded, and does not advance
    // threats or mutate combat state when inspected repeatedly.
    ShadowCryptSkirmish ledger;
    Expect(ledger.Begin(ShadowCryptSkirmishTier::RiftNave),
        "QOL-042 ledger fixture begins");
    Expect(!ledger.ResolveThreat(ShadowCryptDefenseResponse::Guard, 0.10).successful
            && !ledger.ResolveThreat(ShadowCryptDefenseResponse::Dodge, 0.10).successful
            && !ledger.ResolveThreat(ShadowCryptDefenseResponse::Guard, 0.10).successful,
        "QOL-042 fixture creates ward, guard link, and Rift Mark");
    const auto beforeRead = ledger.CurrentThreat();
    const auto statusRead = ledger.StatusLedger();
    const auto statusReadAgain = ledger.StatusLedger();
    const auto afterRead = ledger.CurrentThreat();
    Expect(statusRead.count == 3 && statusReadAgain.count == statusRead.count
            && statusRead.entries[0].kind == ShadowCryptStatusKind::RiftMark
            && statusRead.entries[0].magnitude == 6
            && statusRead.entries[1].kind == ShadowCryptStatusKind::VeilWard
            && statusRead.entries[1].magnitude == 14
            && statusRead.entries[2].kind == ShadowCryptStatusKind::BulwarkGuardLink
            && statusRead.entries[2].magnitude == 10
            && beforeRead.enemyIndex == afterRead.enemyIndex
            && beforeRead.patternStep == afterRead.patternStep
            && beforeRead.failureDamage == afterRead.failureDamage,
        "QOL-042 status ledger is bounded, ordered, exact, and read-only");

    const auto ledgerBeforeInvalid = ledger.StatusLedger();
    const auto invalid = ledger.ResolveThreat(ShadowCryptDefenseResponse::Interrupt,
        std::numeric_limits<double>::quiet_NaN());
    Expect(!invalid.accepted && ledger.StatusLedger().count == ledgerBeforeInvalid.count,
        "QOL-042 rejected nonfinite timing cannot mutate active statuses");
    Expect(ledger.Cancel() && ledger.StatusLedger().count == 0,
        "QOL-042 cancel clears all transient status pressure");
}

} // namespace
