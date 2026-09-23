void TestShadowbladeLoadoutEffectsPass17() {
    using namespace Astral::Scene;

    ShadowbladeActions baseline;
    const ShadowbladeActionTuning baselineTuning = baseline.CurrentLoadoutTuning();
    Expect(baselineTuning.fatalStrikeDamage == ShadowbladeActions::FatalStrikeDamage
            && Near(baselineTuning.dashDistance, ShadowbladeActions::DashDistance)
            && Near(baselineTuning.resourceRegenerationPerSecond,
                ShadowbladeActions::ResourceRegenerationPerSecond)
            && baselineTuning.guardDamageMitigation == 0
            && baselineTuning.readinessScore == 30
            && baselineTuning.activeResonanceFamilies == 0,
        "pass17 Training-Blade-only loadout is baseline-neutral across live action tuning");

    CombatSandbox baselineCombat;
    const ShadowActionReport baselineFatal = baseline.TryFatalStrike({}, baselineCombat);
    Expect(baselineFatal.result == ShadowActionResult::Activated
            && baselineFatal.damageApplied == 80 && baselineCombat.Dummy().health == 20,
        "pass17 default live Fatal Strike preserves the pre-loadout 80 damage contract");

    CharacterProgression progression;
    int levelsGained = 0;
    progression.GrantExperience(100000, levelsGained);
    Expect(progression.Level() == CharacterProgression::MaximumLevel,
        "pass17 fixture reaches the real protagonist level cap through production progression");

    ShadowbladeActions riftsteel;
    Expect(riftsteel.Loadout().AcquireWeapon(ShadowbladeWeapon::RiftsteelSabre)
            && riftsteel.Loadout().TryEquipWeapon(ShadowbladeWeapon::RiftsteelSabre, progression)
                == LoadoutActionResult::Success,
        "pass17 live Shadowblade owner can equip an eligible owned Riftsteel frame");
    const ShadowbladeActionTuning riftsteelTuning = riftsteel.CurrentLoadoutTuning();
    Expect(riftsteelTuning.fatalStrikeDamage == 88
            && Near(riftsteelTuning.dashDistance, 6.25f)
            && Near(riftsteelTuning.resourceRegenerationPerSecond, 16.0f)
            && riftsteelTuning.guardDamageMitigation == 2
            && riftsteelTuning.readinessScore == 42,
        "pass17 Riftsteel profile maps deterministically into bounded live action tuning");

    CombatSandbox riftsteelCombat;
    const ShadowActionReport boostedFatal = riftsteel.TryFatalStrike({}, riftsteelCombat);
    Expect(boostedFatal.result == ShadowActionResult::Activated
            && boostedFatal.damageApplied == 88 && riftsteelCombat.Dummy().health == 12,
        "pass17 net loadout Attack raises live Fatal Strike damage without changing activation gates");

    ShadowbladeActions mobility;
    Expect(mobility.Loadout().AcquireWeapon(ShadowbladeWeapon::RiftsteelSabre)
            && mobility.Loadout().TryEquipWeapon(ShadowbladeWeapon::RiftsteelSabre, progression)
                == LoadoutActionResult::Success,
        "pass17 mobility fixture equips the live Riftsteel frame");
    const ShadowActionReport diagonalDash = mobility.TryDash({}, {3.0f, 4.0f, 0.0f});
    Expect(diagonalDash.result == ShadowActionResult::Activated
            && Near(diagonalDash.dashDestination.x, 3.75f)
            && Near(diagonalDash.dashDestination.y, 5.0f)
            && Near(diagonalDash.resourceSpent, ShadowbladeActions::DashCost),
        "pass17 net Mobility extends normalized dash distance without changing resource cost");
    const float beforeRegen = mobility.Resource();
    mobility.AdvanceTime(0.5f);
    Expect(Near(beforeRegen, 75.0f) && Near(mobility.Resource(), 83.0f),
        "pass17 net Resource Recovery raises finite positive-delta Shadow regeneration");
    const float beforeInvalidDelta = mobility.Resource();
    mobility.AdvanceTime(std::numeric_limits<float>::infinity());
    mobility.AdvanceTime(-1.0f);
    Expect(Near(mobility.Resource(), beforeInvalidDelta),
        "pass17 nonfinite and negative deltas remain resource no-ops with loadout tuning active");

    ShadowbladeActions fallbackDash;
    Expect(fallbackDash.Loadout().AcquireWeapon(ShadowbladeWeapon::RiftsteelSabre)
            && fallbackDash.Loadout().TryEquipWeapon(
                   ShadowbladeWeapon::RiftsteelSabre, progression)
                == LoadoutActionResult::Success,
        "pass17 invalid-direction fixture equips Riftsteel");
    const ShadowActionReport fallbackReport = fallbackDash.TryDash(
        {}, {std::numeric_limits<float>::infinity(), 0.0f, 0.0f});
    Expect(fallbackReport.result == ShadowActionResult::Activated
            && Near(fallbackReport.dashDestination.x, 0.0f)
            && Near(fallbackReport.dashDestination.y, 6.25f),
        "pass17 nonfinite dash direction still uses the deterministic forward fallback at tuned distance");

    ShadowbladeActions guarded;
    Expect(guarded.Loadout().AcquireWeapon(ShadowbladeWeapon::RiftsteelSabre)
            && guarded.Loadout().TryEquipWeapon(ShadowbladeWeapon::RiftsteelSabre, progression)
                == LoadoutActionResult::Success,
        "pass17 guard fixture equips Riftsteel");
    Expect(guarded.BeginIncomingAttack({1.0f, 20, 10, true}),
        "pass17 queues a blockable guard-mitigation threat");
    const DefenseReport guardedReport = guarded.TryDefend(DefenseInput::Guard);
    Expect(guardedReport.result == DefenseResult::Guarded
            && guardedReport.guardDamageTaken == 8
            && guarded.GuardIntegrity() == 92 && guarded.PlayerHealth() == 100,
        "pass17 net Guard mitigates only blockable non-perfect guard-integrity damage");
    Expect(guarded.BeginIncomingAttack({1.0f, 11, 99, false}),
        "pass17 queues an unblockable control threat after the guarded hit");
    const DefenseReport unblockable = guarded.TryDefend(DefenseInput::Guard);
    Expect(unblockable.result == DefenseResult::UnblockableHit
            && unblockable.damageTaken == 11 && unblockable.guardDamageTaken == 0
            && guarded.PlayerHealth() == 89 && guarded.GuardIntegrity() == 92,
        "pass17 loadout Guard mitigation does not alter unblockable health damage or guard integrity");

    ShadowbladeActions cooling;
    Expect(cooling.Loadout().AcquireModule(ResonanceModule::CoolingEdge)
            && cooling.Loadout().AcquireModule(ResonanceModule::CoolingWard)
            && cooling.Loadout().TryEquipModule(
                   ResonanceSlot::Edge, ResonanceModule::CoolingEdge)
                == LoadoutActionResult::Success,
        "pass17 resonance fixture equips one Cooling module");
    const ShadowbladeActionTuning oneCooling = cooling.CurrentLoadoutTuning();
    Expect(oneCooling.activeResonanceFamilies == 0
            && oneCooling.fatalStrikeDamage == 84
            && Near(oneCooling.resourceRegenerationPerSecond, 16.0f)
            && oneCooling.guardDamageMitigation == 0,
        "pass17 one Cooling module contributes only its own stats without family resonance");
    Expect(cooling.Loadout().TryEquipModule(
               ResonanceSlot::Ward, ResonanceModule::CoolingWard)
            == LoadoutActionResult::Success,
        "pass17 second typed Cooling module crosses the existing resonance threshold");
    const ShadowbladeActionTuning twoCooling = cooling.CurrentLoadoutTuning();
    Expect(twoCooling.activeResonanceFamilies == 1
            && twoCooling.fatalStrikeDamage == 84
            && Near(twoCooling.resourceRegenerationPerSecond, 19.0f)
            && twoCooling.guardDamageMitigation == 10
            && twoCooling.readinessScore == 53,
        "pass17 Cooling family resonance now changes real resource and guard action tuning");

    ShadowbladeActions previewActions;
    ShadowbladeLoadout& previewLoadout = previewActions.Loadout();
    Expect(previewLoadout.AcquireWeapon(ShadowbladeWeapon::RiftsteelSabre)
            && previewLoadout.TryEquipWeapon(ShadowbladeWeapon::RiftsteelSabre, progression)
                == LoadoutActionResult::Success
            && previewLoadout.SavePreset(0) == LoadoutActionResult::Success
            && previewLoadout.TryEquipWeapon(ShadowbladeWeapon::TrainingBlade, progression)
                == LoadoutActionResult::Success,
        "pass17 preview fixture saves Riftsteel then restores the baseline build");
    const ShadowbladeActionTuning beforePreview = previewActions.CurrentLoadoutTuning();
    ShadowbladeActionTuning preview{};
    const LoadoutActionResult previewResult = previewActions.PreviewPresetTuning(
        0, progression, preview);
    Expect(previewResult == LoadoutActionResult::Success
            && preview.fatalStrikeDamage == 88 && Near(preview.dashDistance, 6.25f)
            && Near(preview.resourceRegenerationPerSecond, 16.0f)
            && preview.guardDamageMitigation == 2,
        "pass17 valid preset preview reports exact effective action numbers without applying it");
    const ShadowbladeActionTuning afterPreview = previewActions.CurrentLoadoutTuning();
    Expect(previewLoadout.EquippedWeapon() == ShadowbladeWeapon::TrainingBlade
            && afterPreview.fatalStrikeDamage == beforePreview.fatalStrikeDamage
            && Near(afterPreview.dashDistance, beforePreview.dashDistance)
            && Near(afterPreview.resourceRegenerationPerSecond,
                beforePreview.resourceRegenerationPerSecond)
            && afterPreview.guardDamageMitigation == beforePreview.guardDamageMitigation,
        "pass17 successful preview leaves the currently equipped build untouched");

    ShadowbladeActionTuning failureSentinel{123, 9.0f, 8.0f, 7, 6, 5};
    Expect(previewActions.PreviewPresetTuning(1, progression, failureSentinel)
                == LoadoutActionResult::EmptyPreset
            && failureSentinel.fatalStrikeDamage == 123
            && Near(failureSentinel.dashDistance, 9.0f)
            && Near(failureSentinel.resourceRegenerationPerSecond, 8.0f)
            && failureSentinel.guardDamageMitigation == 7
            && failureSentinel.readinessScore == 6
            && failureSentinel.activeResonanceFamilies == 5,
        "pass17 failed preset preview returns the loadout error and preserves caller output atomically");
    Expect(previewLoadout.ApplyPreset(0, progression) == LoadoutActionResult::Success,
        "pass17 valid previewed preset can still be applied through the authoritative loadout API");
    const ShadowbladeActionTuning appliedPreview = previewActions.CurrentLoadoutTuning();
    Expect(appliedPreview.fatalStrikeDamage == preview.fatalStrikeDamage
            && Near(appliedPreview.dashDistance, preview.dashDistance)
            && Near(appliedPreview.resourceRegenerationPerSecond,
                preview.resourceRegenerationPerSecond)
            && appliedPreview.guardDamageMitigation == preview.guardDamageMitigation
            && appliedPreview.readinessScore == preview.readinessScore
            && appliedPreview.activeResonanceFamilies == preview.activeResonanceFamilies,
        "pass17 preview and later application produce the same authoritative action tuning");

    ShadowbladeActions bounded;
    ShadowbladeLoadout& boundedLoadout = bounded.Loadout();
    Expect(boundedLoadout.AcquireWeapon(ShadowbladeWeapon::CryoEdge)
            && boundedLoadout.TryEquipWeapon(ShadowbladeWeapon::CryoEdge, progression)
                == LoadoutActionResult::Success,
        "pass17 bound fixture equips Cryo Edge");
    for (const ResonanceModule module : {
             ResonanceModule::RiftEdge,
             ResonanceModule::CivicWard,
             ResonanceModule::RiftFlow,
             ResonanceModule::RiftInsight}) {
        Expect(boundedLoadout.AcquireModule(module),
            "pass17 bound fixture acquires each unique high-output module once");
    }
    Expect(boundedLoadout.TryEquipModule(ResonanceSlot::Edge, ResonanceModule::RiftEdge)
                == LoadoutActionResult::Success
            && boundedLoadout.TryEquipModule(ResonanceSlot::Ward, ResonanceModule::CivicWard)
                == LoadoutActionResult::Success
            && boundedLoadout.TryEquipModule(ResonanceSlot::Flow, ResonanceModule::RiftFlow)
                == LoadoutActionResult::Success
            && boundedLoadout.TryEquipModule(ResonanceSlot::Insight, ResonanceModule::RiftInsight)
                == LoadoutActionResult::Success,
        "pass17 bound fixture equips a complete four-slot build");
    const ShadowbladeActionTuning boundedTuning = bounded.CurrentLoadoutTuning();
    Expect(boundedTuning.fatalStrikeDamage
                <= ShadowbladeActions::FatalStrikeDamage
                    + ShadowbladeActions::MaximumLoadoutAttackDamageBonus
            && boundedTuning.dashDistance
                <= ShadowbladeActions::DashDistance
                    + ShadowbladeActions::MaximumLoadoutDashDistanceBonus + 0.001f
            && boundedTuning.resourceRegenerationPerSecond
                <= ShadowbladeActions::ResourceRegenerationPerSecond
                    + ShadowbladeActions::MaximumLoadoutResourceRegenerationBonus + 0.001f
            && boundedTuning.guardDamageMitigation
                <= ShadowbladeActions::MaximumLoadoutGuardDamageMitigation
            && boundedTuning.readinessScore >= 0 && boundedTuning.readinessScore <= 100,
        "pass17 full catalogue build remains inside every explicit live tuning bound");

    ShadowbladeLoadoutProfile extremeProfile{};
    extremeProfile.attackBonus = std::numeric_limits<int>::max();
    extremeProfile.guardBonus = std::numeric_limits<int>::max();
    extremeProfile.resourceRecoveryBonus = std::numeric_limits<int>::max();
    extremeProfile.mobilityBonus = std::numeric_limits<int>::max();
    extremeProfile.readinessScore = std::numeric_limits<int>::max();
    extremeProfile.activeResonanceFamilies = std::numeric_limits<int>::max();
    const ShadowbladeActionTuning clampedExtreme =
        ShadowbladeActions::ActionTuningForProfile(extremeProfile);
    Expect(clampedExtreme.fatalStrikeDamage
                == ShadowbladeActions::FatalStrikeDamage
                    + ShadowbladeActions::MaximumLoadoutAttackDamageBonus
            && Near(clampedExtreme.dashDistance,
                ShadowbladeActions::DashDistance
                    + ShadowbladeActions::MaximumLoadoutDashDistanceBonus)
            && Near(clampedExtreme.resourceRegenerationPerSecond,
                ShadowbladeActions::ResourceRegenerationPerSecond
                    + ShadowbladeActions::MaximumLoadoutResourceRegenerationBonus)
            && clampedExtreme.guardDamageMitigation
                == ShadowbladeActions::MaximumLoadoutGuardDamageMitigation
            && clampedExtreme.readinessScore == 100
            && clampedExtreme.activeResonanceFamilies == 3,
        "pass17 extreme hypothetical profile exercises every hard tuning clamp exactly");

    ShadowbladeLoadoutProfile negativeProfile{};
    negativeProfile.attackBonus = std::numeric_limits<int>::min();
    negativeProfile.guardBonus = std::numeric_limits<int>::min();
    negativeProfile.resourceRecoveryBonus = std::numeric_limits<int>::min();
    negativeProfile.mobilityBonus = std::numeric_limits<int>::min();
    negativeProfile.readinessScore = std::numeric_limits<int>::min();
    negativeProfile.activeResonanceFamilies = std::numeric_limits<int>::min();
    const ShadowbladeActionTuning clampedNegative =
        ShadowbladeActions::ActionTuningForProfile(negativeProfile);
    Expect(clampedNegative.fatalStrikeDamage == ShadowbladeActions::FatalStrikeDamage
            && Near(clampedNegative.dashDistance, ShadowbladeActions::DashDistance)
            && Near(clampedNegative.resourceRegenerationPerSecond,
                ShadowbladeActions::ResourceRegenerationPerSecond)
            && clampedNegative.guardDamageMitigation == 0
            && clampedNegative.readinessScore == 0
            && clampedNegative.activeResonanceFamilies == 0,
        "pass17 negative hypothetical profile clamps safely to the baseline without overflow");

    ShadowbladeActions retryActions;
    ShadowbladeLoadout& retryLoadout = retryActions.Loadout();
    Expect(retryLoadout.AcquireWeapon(ShadowbladeWeapon::RiftsteelSabre)
            && retryLoadout.TryEquipWeapon(ShadowbladeWeapon::RiftsteelSabre, progression)
                == LoadoutActionResult::Success
            && retryLoadout.AcquireModule(ResonanceModule::RiftEdge)
            && retryLoadout.TryEquipModule(ResonanceSlot::Edge, ResonanceModule::RiftEdge)
                == LoadoutActionResult::Success
            && retryLoadout.SavePreset(0) == LoadoutActionResult::Success,
        "pass17 reset fixture owns, equips, and saves a non-default persistent loadout");
    const ShadowbladeActionTuning tuningBeforeReset = retryActions.CurrentLoadoutTuning();
    retryActions.SetDefenseTimingPreset(DefenseTimingPreset::Forgiving);
    Expect(retryActions.TryDash({}).result == ShadowActionResult::Activated
            && retryActions.BeginIncomingAttack({1.0f, 20, 10, true}),
        "pass17 reset fixture creates transient action state before retry reset");
    retryActions.SetGuarding(true);

    retryActions.ResetTransientStatePreservingLoadout();
    const ShadowbladeActionTuning tuningAfterReset = retryActions.CurrentLoadoutTuning();
    Expect(retryActions.Loadout().OwnsWeapon(ShadowbladeWeapon::RiftsteelSabre)
            && retryActions.Loadout().EquippedWeapon() == ShadowbladeWeapon::RiftsteelSabre
            && retryActions.Loadout().OwnsModule(ResonanceModule::RiftEdge)
            && retryActions.Loadout().ModuleSlotOccupied(ResonanceSlot::Edge)
            && retryActions.Loadout().EquippedModule(ResonanceSlot::Edge)
                == ResonanceModule::RiftEdge
            && retryActions.Loadout().PresetSaved(0)
            && tuningAfterReset.fatalStrikeDamage == tuningBeforeReset.fatalStrikeDamage
            && Near(tuningAfterReset.dashDistance, tuningBeforeReset.dashDistance)
            && Near(tuningAfterReset.resourceRegenerationPerSecond,
                tuningBeforeReset.resourceRegenerationPerSecond)
            && tuningAfterReset.guardDamageMitigation == tuningBeforeReset.guardDamageMitigation,
        "pass17 transient retry reset preserves owned, equipped, preset, and tuned loadout state");
    Expect(Near(retryActions.Resource(), ShadowbladeActions::MaximumResource)
            && Near(retryActions.DashCooldownRemaining(), 0.0f)
            && Near(retryActions.FatalStrikeCooldownRemaining(), 0.0f)
            && !retryActions.IsGuarding() && !retryActions.HasIncomingAttack()
            && retryActions.PlayerHealth() == ShadowbladeActions::MaximumPlayerHealth
            && retryActions.GuardIntegrity() == ShadowbladeActions::MaximumGuardIntegrity
            && Near(retryActions.PerfectDefenseWindowSeconds(),
                ShadowbladeActions::ForgivingPerfectDefenseWindowSeconds),
        "pass17 transient retry reset clears combat/action state while retaining timing preference");
}
