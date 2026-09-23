void TestShadowbladeTuningPass18() {
    using namespace Astral::Scene;

    CharacterProgression progression;
    int levelsGained = 0;
    progression.GrantExperience(100000, levelsGained);
    Expect(progression.Level() == CharacterProgression::MaximumLevel,
        "pass18 fixture reaches the production progression cap");

    const auto invalidWeapon = static_cast<ShadowbladeWeapon>(255);
    const auto invalidModule = static_cast<ResonanceModule>(255);

    ShadowbladeLoadout validation;
    Expect(validation.WeaponTuneRank(invalidWeapon) == -1
            && validation.ModuleTuneRank(invalidModule) == -1
            && validation.RemainingWeaponTuneCost(invalidWeapon) == 0
            && validation.RemainingModuleTuneCost(invalidModule) == 0
            && validation.TryTuneWeapon(invalidWeapon, progression)
                == LoadoutActionResult::Invalid
            && validation.TryTuneModule(invalidModule) == LoadoutActionResult::Invalid
            && validation.SetModuleProtected(invalidModule, true)
                == LoadoutActionResult::Invalid,
        "pass18 malformed tuning ids fail closed without resource mutation");

    ShadowbladeActions effectsActions;
    ShadowbladeLoadout& effects = effectsActions.Loadout();
    Expect(effects.AcquireModule(ResonanceModule::CoolingEdge)
            && effects.AcquireModule(ResonanceModule::CoolingWard)
            && effects.AcquireModule(ResonanceModule::CoolingFlow)
            && effects.AcquireModule(ResonanceModule::CivicInsight)
            && effects.AcquireModule(ResonanceModule::RiftEdge)
            && effects.AcquireModule(ResonanceModule::CivicWard)
            && effects.AcquireModule(ResonanceModule::RiftFlow)
            && effects.AcquireModule(ResonanceModule::RiftInsight),
        "pass18 effect fixture acquires the bounded module catalogue");
    Expect(effects.TrySalvageModule(ResonanceModule::RiftEdge)
                == LoadoutActionResult::Success
            && effects.TrySalvageModule(ResonanceModule::CivicWard)
                == LoadoutActionResult::Success
            && effects.TrySalvageModule(ResonanceModule::RiftFlow)
                == LoadoutActionResult::Success
            && effects.TrySalvageModule(ResonanceModule::RiftInsight)
                == LoadoutActionResult::Success
            && effects.TuningParts() == 54,
        "pass18 deterministic donor salvage funds bounded tuning");
    Expect(effects.TryEquipModule(ResonanceSlot::Edge, ResonanceModule::CoolingEdge)
                == LoadoutActionResult::Success
            && effects.TryEquipModule(ResonanceSlot::Ward, ResonanceModule::CoolingWard)
                == LoadoutActionResult::Success
            && effects.TryEquipModule(ResonanceSlot::Flow, ResonanceModule::CoolingFlow)
                == LoadoutActionResult::Success
            && effects.TryEquipModule(ResonanceSlot::Insight, ResonanceModule::CivicInsight)
                == LoadoutActionResult::Success,
        "pass18 target modules equip through the existing typed slots");

    const ShadowbladeLoadoutProfile beforeProfile = effects.Profile();
    const ShadowbladeActionTuning beforeTuning = effectsActions.CurrentLoadoutTuning();
    Expect(effects.RemainingModuleTuneCost(ResonanceModule::CoolingEdge) == 24
            && effects.RemainingModuleTuneCost(ResonanceModule::CoolingWard) == 24
            && effects.RemainingModuleTuneCost(ResonanceModule::CoolingFlow) == 24
            && effects.RemainingModuleTuneCost(ResonanceModule::CivicInsight) == 24,
        "pass18 module material plans report the exact remaining rank-zero budget");

    Expect(effects.TryTuneModule(ResonanceModule::CoolingEdge)
                == LoadoutActionResult::Success
            && effects.TryTuneModule(ResonanceModule::CoolingWard)
                == LoadoutActionResult::Success
            && effects.TryTuneModule(ResonanceModule::CoolingFlow)
                == LoadoutActionResult::Success
            && effects.TryTuneModule(ResonanceModule::CivicInsight)
                == LoadoutActionResult::Success
            && effects.TuningParts() == 38,
        "pass18 one rank across four module slots spends the exact bounded costs");
    const ShadowbladeLoadoutProfile afterProfile = effects.Profile();
    const ShadowbladeActionTuning afterTuning = effectsActions.CurrentLoadoutTuning();
    Expect(afterProfile.attackBonus == beforeProfile.attackBonus + 1
            && afterProfile.guardBonus == beforeProfile.guardBonus + 1
            && afterProfile.resourceRecoveryBonus
                == beforeProfile.resourceRecoveryBonus + 1
            && afterProfile.mobilityBonus == beforeProfile.mobilityBonus + 1,
        "pass18 module ranks affect the intended Edge Ward Flow and Insight stats");
    Expect(afterTuning.fatalStrikeDamage == beforeTuning.fatalStrikeDamage + 2
            && afterTuning.guardDamageMitigation == beforeTuning.guardDamageMitigation + 2
            && Near(afterTuning.resourceRegenerationPerSecond,
                beforeTuning.resourceRegenerationPerSecond + 1.0f)
            && Near(afterTuning.dashDistance, beforeTuning.dashDistance + 0.25f),
        "pass18 tuned modules flow through the already-live Shadowblade action tuning");

    Expect(effects.ModuleTuneRank(ResonanceModule::CoolingEdge) == 1
            && effects.RemainingModuleTuneCost(ResonanceModule::CoolingEdge) == 20
            && effects.TryTuneModule(ResonanceModule::CoolingEdge)
                == LoadoutActionResult::Success
            && effects.TryTuneModule(ResonanceModule::CoolingEdge)
                == LoadoutActionResult::Success
            && effects.ModuleTuneRank(ResonanceModule::CoolingEdge)
                == ShadowbladeLoadout::MaximumTuneRank
            && effects.RemainingModuleTuneCost(ResonanceModule::CoolingEdge) == 0,
        "pass18 module tuning reaches the exact three-rank cap and material plan terminates");
    const int partsAtModuleCap = effects.TuningParts();
    Expect(effects.TryTuneModule(ResonanceModule::CoolingEdge)
                == LoadoutActionResult::MaximumRank
            && effects.TuningParts() == partsAtModuleCap,
        "pass18 max-rank module attempts are non-mutating");

    ShadowbladeLoadout insufficient;
    Expect(insufficient.AcquireModule(ResonanceModule::CoolingEdge)
            && insufficient.TryTuneModule(ResonanceModule::CoolingEdge)
                == LoadoutActionResult::InsufficientParts
            && insufficient.ModuleTuneRank(ResonanceModule::CoolingEdge) == 0
            && insufficient.TuningParts() == 0,
        "pass18 insufficient tuning parts fail atomically");

    ShadowbladeActions weaponActions;
    ShadowbladeLoadout& weaponLoadout = weaponActions.Loadout();
    Expect(weaponLoadout.AcquireWeapon(ShadowbladeWeapon::RiftsteelSabre)
            && weaponLoadout.AcquireModule(ResonanceModule::CoolingEdge)
            && weaponLoadout.AcquireModule(ResonanceModule::CoolingWard)
            && weaponLoadout.AcquireModule(ResonanceModule::RiftFlow)
            && weaponLoadout.TrySalvageModule(ResonanceModule::CoolingEdge)
                == LoadoutActionResult::Success
            && weaponLoadout.TrySalvageModule(ResonanceModule::CoolingWard)
                == LoadoutActionResult::Success
            && weaponLoadout.TrySalvageModule(ResonanceModule::RiftFlow)
                == LoadoutActionResult::Success
            && weaponLoadout.TuningParts() == 39,
        "pass18 weapon fixture funds all three calibration ranks without hidden grants");
    CharacterProgression lowProgression;
    Expect(weaponLoadout.TryTuneWeapon(
               ShadowbladeWeapon::RiftsteelSabre, lowProgression)
                == LoadoutActionResult::Locked
            && weaponLoadout.WeaponTuneRank(ShadowbladeWeapon::RiftsteelSabre) == 0
            && weaponLoadout.TuningParts() == 39,
        "pass18 weapon calibration preserves the existing protagonist level gate");
    Expect(weaponLoadout.TryEquipWeapon(
               ShadowbladeWeapon::RiftsteelSabre, progression)
                == LoadoutActionResult::Success
            && weaponLoadout.RemainingWeaponTuneCost(
                ShadowbladeWeapon::RiftsteelSabre) == 30,
        "pass18 eligible owned weapon exposes the exact full calibration budget");
    const ShadowbladeActionTuning weaponBefore = weaponActions.CurrentLoadoutTuning();
    Expect(weaponLoadout.TryTuneWeapon(
               ShadowbladeWeapon::RiftsteelSabre, progression)
                == LoadoutActionResult::Success
            && weaponLoadout.TryTuneWeapon(
               ShadowbladeWeapon::RiftsteelSabre, progression)
                == LoadoutActionResult::Success
            && weaponLoadout.TryTuneWeapon(
               ShadowbladeWeapon::RiftsteelSabre, progression)
                == LoadoutActionResult::Success
            && weaponLoadout.TuningParts() == 9
            && weaponLoadout.WeaponTuneRank(ShadowbladeWeapon::RiftsteelSabre)
                == ShadowbladeLoadout::MaximumTuneRank
            && weaponLoadout.RemainingWeaponTuneCost(
                ShadowbladeWeapon::RiftsteelSabre) == 0,
        "pass18 weapon calibration spends 6 10 14 parts and stops at rank three");
    const ShadowbladeActionTuning weaponAfter = weaponActions.CurrentLoadoutTuning();
    Expect(weaponAfter.fatalStrikeDamage == weaponBefore.fatalStrikeDamage + 6
            && weaponAfter.guardDamageMitigation == weaponBefore.guardDamageMitigation + 2,
        "pass18 weapon calibration feeds live attack and guard tuning");
    const int partsAtWeaponCap = weaponLoadout.TuningParts();
    Expect(weaponLoadout.TryTuneWeapon(
               ShadowbladeWeapon::RiftsteelSabre, progression)
                == LoadoutActionResult::MaximumRank
            && weaponLoadout.TuningParts() == partsAtWeaponCap,
        "pass18 max-rank weapon attempts do not consume tuning parts");

    ShadowbladeLoadout recommendation;
    Expect(recommendation.AcquireModule(ResonanceModule::CoolingEdge)
            && recommendation.AcquireModule(ResonanceModule::RiftFlow)
            && recommendation.TryEquipModule(
                ResonanceSlot::Edge, ResonanceModule::CoolingEdge)
                == LoadoutActionResult::Success,
        "pass18 recommendation fixture equips one tunable module");
    const ShadowbladeUpgradeRecommendation unfunded =
        recommendation.RecommendedUpgrade();
    Expect(unfunded.available
            && unfunded.kind == LoadoutUpgradeKind::Module
            && unfunded.module == ResonanceModule::CoolingEdge
            && unfunded.currentRank == 0
            && unfunded.nextCost == 4
            && !unfunded.affordable,
        "pass18 recommendation is deterministic and reports an unaffordable cheapest upgrade");
    Expect(recommendation.TrySalvageModule(ResonanceModule::RiftFlow)
                == LoadoutActionResult::Success,
        "pass18 recommendation fixture gains real tuning parts through salvage");
    const ShadowbladeUpgradeRecommendation funded =
        recommendation.RecommendedUpgrade();
    Expect(funded.available && funded.kind == LoadoutUpgradeKind::Module
            && funded.module == ResonanceModule::CoolingEdge
            && funded.nextCost == 4 && funded.affordable,
        "pass18 recommendation updates affordability from authoritative tuning parts");
    Expect(recommendation.TryTuneModule(ResonanceModule::CoolingEdge)
                == LoadoutActionResult::Success,
        "pass18 recommendation target can be upgraded through the same production API");
    const ShadowbladeUpgradeRecommendation afterFirstModuleRank =
        recommendation.RecommendedUpgrade();
    Expect(afterFirstModuleRank.available
            && afterFirstModuleRank.kind == LoadoutUpgradeKind::Weapon
            && afterFirstModuleRank.weapon == ShadowbladeWeapon::TrainingBlade
            && afterFirstModuleRank.nextCost == 6
            && afterFirstModuleRank.affordable,
        "pass18 recommendation re-evaluates and prefers the cheaper next equipped upgrade");

    ShadowbladeLoadout recovery;
    Expect(recovery.AcquireModule(ResonanceModule::CivicWard)
            && recovery.AcquireModule(ResonanceModule::RiftFlow)
            && recovery.TrySalvageModule(ResonanceModule::RiftFlow)
                == LoadoutActionResult::Success
            && recovery.TryTuneModule(ResonanceModule::CivicWard)
                == LoadoutActionResult::Success
            && recovery.TryTuneModule(ResonanceModule::CivicWard)
                == LoadoutActionResult::Success
            && recovery.ModuleTuneRank(ResonanceModule::CivicWard) == 2
            && recovery.TuningParts() == 3,
        "pass18 enhanced-salvage fixture invests exactly twelve tuning parts");
    Expect(recovery.TrySalvageModule(ResonanceModule::CivicWard)
                == LoadoutActionResult::Success
            && recovery.TuningParts() == 21
            && recovery.ModuleTuneRank(ResonanceModule::CivicWard) == 0
            && !recovery.OwnsModule(ResonanceModule::CivicWard),
        "pass18 enhanced salvage returns base value plus half invested parts and clears rank");
    Expect(recovery.TrySalvageModule(ResonanceModule::CivicWard)
                == LoadoutActionResult::NotOwned
            && !recovery.AcquireModule(ResonanceModule::CivicWard)
            && recovery.TuningParts() == 21,
        "pass18 enhanced salvage remains one-shot and cannot be farmed by reacquisition");

    ShadowbladeLoadout protection;
    Expect(protection.AcquireModule(ResonanceModule::CivicInsight)
            && protection.SetModuleProtected(
                ResonanceModule::CivicInsight, true) == LoadoutActionResult::Success
            && protection.ModuleProtected(ResonanceModule::CivicInsight),
        "pass18 player protection marks an owned module without changing equipment");
    const int protectedParts = protection.TuningParts();
    Expect(protection.TrySalvageModule(ResonanceModule::CivicInsight)
                == LoadoutActionResult::Protected
            && protection.OwnsModule(ResonanceModule::CivicInsight)
            && protection.TuningParts() == protectedParts,
        "pass18 protected modules cannot be accidentally salvaged");
    Expect(protection.SetModuleProtected(
               ResonanceModule::CivicInsight, false) == LoadoutActionResult::Success
            && !protection.ModuleProtected(ResonanceModule::CivicInsight)
            && protection.TrySalvageModule(ResonanceModule::CivicInsight)
                == LoadoutActionResult::Success
            && protection.TuningParts() == 15,
        "pass18 explicit unprotect restores the ordinary deterministic salvage path");
}
