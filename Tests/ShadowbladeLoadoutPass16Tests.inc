void TestShadowbladeLoadoutPass16() {
    using namespace Astral::Scene;

    CharacterProgression progression;
    ShadowbladeLoadout loadout;

    Expect(loadout.OwnsWeapon(ShadowbladeWeapon::TrainingBlade)
            && loadout.EquippedWeapon() == ShadowbladeWeapon::TrainingBlade,
        "pass16 starts with the original Training Blade owned and equipped");
    const ShadowbladeLoadoutProfile initial = loadout.Profile();
    Expect(initial.attackBonus == 4 && initial.guardBonus == 1
            && initial.activeResonanceFamilies == 0 && initial.readinessScore == 30,
        "pass16 default weapon produces deterministic bounded baseline profile");

    const auto invalidWeapon = static_cast<ShadowbladeWeapon>(255);
    const auto invalidModule = static_cast<ResonanceModule>(255);
    const auto invalidSlot = static_cast<ResonanceSlot>(255);
    Expect(!loadout.AcquireWeapon(invalidWeapon)
            && loadout.TryEquipWeapon(invalidWeapon, progression) == LoadoutActionResult::Invalid,
        "pass16 malformed weapon ids fail closed");
    Expect(!loadout.AcquireModule(invalidModule)
            && loadout.TryEquipModule(ResonanceSlot::Edge, invalidModule)
                == LoadoutActionResult::Invalid
            && loadout.TryEquipModule(invalidSlot, ResonanceModule::CoolingEdge)
                == LoadoutActionResult::Invalid
            && loadout.UnequipModule(invalidSlot) == LoadoutActionResult::Invalid,
        "pass16 malformed module and slot ids fail closed");

    Expect(loadout.TryEquipWeapon(ShadowbladeWeapon::RiftsteelSabre, progression)
            == LoadoutActionResult::NotOwned,
        "pass16 unowned weapon cannot be equipped");
    Expect(loadout.AcquireWeapon(ShadowbladeWeapon::RiftsteelSabre)
            && loadout.AcquireWeapon(ShadowbladeWeapon::CryoEdge),
        "pass16 original advanced weapon frames can be acquired once");
    Expect(loadout.TryEquipWeapon(ShadowbladeWeapon::RiftsteelSabre, progression)
            == LoadoutActionResult::Locked
            && loadout.TryEquipWeapon(ShadowbladeWeapon::CryoEdge, progression)
                == LoadoutActionResult::Locked
            && loadout.EquippedWeapon() == ShadowbladeWeapon::TrainingBlade,
        "pass16 level-gated weapon failures preserve current equipment");

    CharacterProgression boundaryProgression;
    ShadowbladeLoadout boundaryLoadout;
    Expect(boundaryLoadout.AcquireWeapon(ShadowbladeWeapon::RiftsteelSabre)
            && boundaryLoadout.AcquireWeapon(ShadowbladeWeapon::CryoEdge),
        "pass16 boundary fixture owns both gated weapon frames");
    int boundaryLevels = 0;
    boundaryProgression.GrantExperience(2799, boundaryLevels);
    Expect(boundaryProgression.Level() == 7
            && boundaryLoadout.TryEquipWeapon(
                   ShadowbladeWeapon::RiftsteelSabre, boundaryProgression)
                == LoadoutActionResult::Locked,
        "pass16 Riftsteel remains locked one experience point below level 8");
    boundaryProgression.GrantExperience(1, boundaryLevels);
    Expect(boundaryProgression.Level() == 8
            && boundaryLoadout.TryEquipWeapon(
                   ShadowbladeWeapon::RiftsteelSabre, boundaryProgression)
                == LoadoutActionResult::Success
            && boundaryLoadout.TryEquipWeapon(ShadowbladeWeapon::CryoEdge, boundaryProgression)
                == LoadoutActionResult::Locked
            && boundaryLoadout.EquippedWeapon() == ShadowbladeWeapon::RiftsteelSabre,
        "pass16 Riftsteel unlocks exactly at level 8 while Cryo Edge remains locked");
    boundaryProgression.GrantExperience(6299, boundaryLevels);
    Expect(boundaryProgression.Level() == 13
            && boundaryLoadout.TryEquipWeapon(ShadowbladeWeapon::CryoEdge, boundaryProgression)
                == LoadoutActionResult::Locked
            && boundaryLoadout.EquippedWeapon() == ShadowbladeWeapon::RiftsteelSabre,
        "pass16 Cryo Edge remains locked one experience point below level 14");
    boundaryProgression.GrantExperience(1, boundaryLevels);
    Expect(boundaryProgression.Level() == 14
            && boundaryLoadout.TryEquipWeapon(ShadowbladeWeapon::CryoEdge, boundaryProgression)
                == LoadoutActionResult::Success
            && boundaryLoadout.EquippedWeapon() == ShadowbladeWeapon::CryoEdge,
        "pass16 Cryo Edge unlocks exactly at level 14");

    int levelsGained = 0;
    progression.GrantExperience(100000, levelsGained);
    Expect(progression.Level() == CharacterProgression::MaximumLevel,
        "pass16 test fixture advances real protagonist progression through production API");
    Expect(loadout.TryEquipWeapon(ShadowbladeWeapon::RiftsteelSabre, progression)
            == LoadoutActionResult::Success
            && loadout.EquippedWeapon() == ShadowbladeWeapon::RiftsteelSabre,
        "pass16 eligible owned Riftsteel Sabre can be equipped");
    Expect(loadout.TryEquipWeapon(ShadowbladeWeapon::CryoEdge, progression)
            == LoadoutActionResult::Success
            && loadout.EquippedWeapon() == ShadowbladeWeapon::CryoEdge,
        "pass16 eligible owned Cryo Edge can be equipped");

    Expect(loadout.TryEquipModule(ResonanceSlot::Edge, ResonanceModule::CoolingEdge)
            == LoadoutActionResult::NotOwned,
        "pass16 unowned module cannot be equipped");
    Expect(loadout.AcquireModule(ResonanceModule::CoolingEdge)
            && loadout.AcquireModule(ResonanceModule::CoolingWard)
            && loadout.AcquireModule(ResonanceModule::CoolingFlow)
            && loadout.AcquireModule(ResonanceModule::RiftEdge)
            && loadout.AcquireModule(ResonanceModule::RiftFlow)
            && loadout.AcquireModule(ResonanceModule::RiftInsight)
            && loadout.AcquireModule(ResonanceModule::CivicWard)
            && loadout.AcquireModule(ResonanceModule::CivicInsight),
        "pass16 bounded module catalogue can be acquired without duplicates");
    Expect(!loadout.AcquireModule(ResonanceModule::CoolingEdge),
        "pass16 duplicate module acquisition is idempotent");
    Expect(loadout.TryEquipModule(ResonanceSlot::Ward, ResonanceModule::CoolingEdge)
            == LoadoutActionResult::Invalid
            && !loadout.ModuleSlotOccupied(ResonanceSlot::Ward),
        "pass16 module slot typing rejects a wrong-slot equip without mutation");

    ShadowbladeLoadout replacementLoadout;
    Expect(replacementLoadout.AcquireModule(ResonanceModule::CoolingEdge)
            && replacementLoadout.AcquireModule(ResonanceModule::RiftEdge)
            && replacementLoadout.AcquireModule(ResonanceModule::CoolingWard)
            && replacementLoadout.TryEquipModule(
                   ResonanceSlot::Edge, ResonanceModule::CoolingEdge)
                == LoadoutActionResult::Success
            && replacementLoadout.TryEquipModule(
                   ResonanceSlot::Ward, ResonanceModule::CoolingWard)
                == LoadoutActionResult::Success
            && replacementLoadout.TryEquipModule(ResonanceSlot::Edge, ResonanceModule::RiftEdge)
                == LoadoutActionResult::Success
            && replacementLoadout.EquippedModule(ResonanceSlot::Edge)
                == ResonanceModule::RiftEdge
            && replacementLoadout.EquippedModule(ResonanceSlot::Ward)
                == ResonanceModule::CoolingWard,
        "pass16 replacing one typed slot preserves every other equipped slot");
    Expect(replacementLoadout.UnequipModule(ResonanceSlot::Ward)
            == LoadoutActionResult::Success
            && !replacementLoadout.ModuleSlotOccupied(ResonanceSlot::Ward)
            && replacementLoadout.UnequipModule(ResonanceSlot::Ward)
                == LoadoutActionResult::Invalid
            && replacementLoadout.EquippedModule(ResonanceSlot::Edge)
                == ResonanceModule::RiftEdge,
        "pass16 explicit unequip only clears the requested occupied slot");

    Expect(loadout.TryEquipModule(ResonanceSlot::Edge, ResonanceModule::CoolingEdge)
            == LoadoutActionResult::Success,
        "pass16 matching Edge module equips successfully");
    ShadowbladeLoadoutProfile oneCooling = loadout.Profile();
    Expect(oneCooling.activeResonanceFamilies == 0,
        "pass16 one family module does not prematurely activate resonance");
    Expect(loadout.TryEquipModule(ResonanceSlot::Ward, ResonanceModule::CoolingWard)
            == LoadoutActionResult::Success,
        "pass16 matching Ward module equips successfully");
    ShadowbladeLoadoutProfile twoCooling = loadout.Profile();
    Expect(twoCooling.activeResonanceFamilies == 1
            && twoCooling.attackBonus == 8 && twoCooling.guardBonus == 9
            && twoCooling.resourceRecoveryBonus == 7
            && twoCooling.readinessScore >= 0 && twoCooling.readinessScore <= 100,
        "pass16 two Cooling modules activate one deterministic bounded family bonus");

    Expect(loadout.TryEquipModule(ResonanceSlot::Flow, ResonanceModule::RiftFlow)
            == LoadoutActionResult::Success
            && loadout.TryEquipModule(ResonanceSlot::Insight, ResonanceModule::RiftInsight)
                == LoadoutActionResult::Success,
        "pass16 separate typed Flow and Insight modules equip independently");
    const ShadowbladeLoadoutProfile mixedProfile = loadout.Profile();
    Expect(mixedProfile.activeResonanceFamilies == 2
            && mixedProfile.readinessScore >= 0 && mixedProfile.readinessScore <= 100,
        "pass16 two distinct two-module families each activate once without stacking duplicates");

    Expect(loadout.SavePreset(0) == LoadoutActionResult::Success
            && loadout.PresetSaved(0),
        "pass16 preset zero stores exact current equipment");
    const ShadowbladeLoadoutPreset* firstPreset = loadout.Preset(0);
    Expect(firstPreset != nullptr
            && firstPreset->weapon == ShadowbladeWeapon::CryoEdge
            && firstPreset->occupied[0] && firstPreset->modules[0] == ResonanceModule::CoolingEdge
            && firstPreset->occupied[1] && firstPreset->modules[1] == ResonanceModule::CoolingWard
            && firstPreset->occupied[2] && firstPreset->modules[2] == ResonanceModule::RiftFlow
            && firstPreset->occupied[3] && firstPreset->modules[3] == ResonanceModule::RiftInsight,
        "pass16 preset retains exact weapon/module identity rather than only derived stats");

    Expect(loadout.TryEquipWeapon(ShadowbladeWeapon::TrainingBlade, progression)
            == LoadoutActionResult::Success
            && loadout.TryEquipModule(ResonanceSlot::Edge, ResonanceModule::RiftEdge)
                == LoadoutActionResult::Success
            && loadout.TryEquipModule(ResonanceSlot::Ward, ResonanceModule::CivicWard)
                == LoadoutActionResult::Success
            && loadout.TryEquipModule(ResonanceSlot::Insight, ResonanceModule::CivicInsight)
                == LoadoutActionResult::Success
            && loadout.SavePreset(1) == LoadoutActionResult::Success,
        "pass16 second preset captures a distinct original build");
    Expect(loadout.ApplyPreset(0, progression) == LoadoutActionResult::Success
            && loadout.EquippedWeapon() == ShadowbladeWeapon::CryoEdge
            && loadout.EquippedModule(ResonanceSlot::Edge) == ResonanceModule::CoolingEdge
            && loadout.EquippedModule(ResonanceSlot::Ward) == ResonanceModule::CoolingWard
            && loadout.EquippedModule(ResonanceSlot::Flow) == ResonanceModule::RiftFlow
            && loadout.EquippedModule(ResonanceSlot::Insight) == ResonanceModule::RiftInsight,
        "pass16 valid preset switch restores the exact saved build atomically");

    const ShadowbladeWeapon beforeEmptyWeapon = loadout.EquippedWeapon();
    const ResonanceModule beforeEmptyEdge = loadout.EquippedModule(ResonanceSlot::Edge);
    const ShadowbladeLoadoutProfile beforeEmptyProfile = loadout.Profile();
    Expect(loadout.ApplyPreset(2, progression) == LoadoutActionResult::EmptyPreset
            && loadout.ApplyPreset(ShadowbladeLoadout::PresetSlots, progression)
                == LoadoutActionResult::Invalid
            && loadout.EquippedWeapon() == beforeEmptyWeapon
            && loadout.EquippedModule(ResonanceSlot::Edge) == beforeEmptyEdge
            && loadout.Profile().readinessScore == beforeEmptyProfile.readinessScore,
        "pass16 empty and out-of-range preset requests are non-mutating");

    const int partsBeforeEquipped = loadout.TuningParts();
    Expect(loadout.TrySalvageModule(ResonanceModule::CoolingEdge)
            == LoadoutActionResult::Equipped
            && loadout.TuningParts() == partsBeforeEquipped
            && loadout.OwnsModule(ResonanceModule::CoolingEdge),
        "pass16 equipped modules are protected from salvage");

    Expect(loadout.ApplyPreset(1, progression) == LoadoutActionResult::Success,
        "pass16 switches away from the exact module before salvage");
    const ShadowbladeLoadoutProfile stateBeforeStaleApply = loadout.Profile();
    const ShadowbladeWeapon weaponBeforeStaleApply = loadout.EquippedWeapon();
    const ResonanceModule edgeBeforeStaleApply = loadout.EquippedModule(ResonanceSlot::Edge);
    const ResonanceModule wardBeforeStaleApply = loadout.EquippedModule(ResonanceSlot::Ward);
    const ResonanceModule flowBeforeStaleApply = loadout.EquippedModule(ResonanceSlot::Flow);
    const ResonanceModule insightBeforeStaleApply = loadout.EquippedModule(ResonanceSlot::Insight);
    const std::array<bool, 4> occupiedBeforeStaleApply{
        loadout.ModuleSlotOccupied(ResonanceSlot::Edge),
        loadout.ModuleSlotOccupied(ResonanceSlot::Ward),
        loadout.ModuleSlotOccupied(ResonanceSlot::Flow),
        loadout.ModuleSlotOccupied(ResonanceSlot::Insight),
    };

    Expect(loadout.TrySalvageModule(ResonanceModule::CoolingEdge)
            == LoadoutActionResult::Success
            && !loadout.OwnsModule(ResonanceModule::CoolingEdge)
            && loadout.TuningParts() == partsBeforeEquipped + 12,
        "pass16 unequipped module salvages exactly once into bounded tuning parts");
    Expect(loadout.TrySalvageModule(ResonanceModule::CoolingEdge)
            == LoadoutActionResult::NotOwned
            && !loadout.AcquireModule(ResonanceModule::CoolingEdge)
            && loadout.TuningParts() == partsBeforeEquipped + 12
            && loadout.TuningParts() <= ShadowbladeLoadout::MaximumTuningParts,
        "pass16 salvaged unique module cannot be reacquired for resource farming");

    const int partsBeforeStaleApply = loadout.TuningParts();
    Expect(loadout.ApplyPreset(0, progression) == LoadoutActionResult::MissingPresetItem,
        "pass16 exact-item preset refuses to substitute for a salvaged saved module");
    const ShadowbladeLoadoutProfile afterStaleApply = loadout.Profile();
    Expect(loadout.EquippedWeapon() == weaponBeforeStaleApply
            && loadout.EquippedModule(ResonanceSlot::Edge) == edgeBeforeStaleApply
            && loadout.EquippedModule(ResonanceSlot::Ward) == wardBeforeStaleApply
            && loadout.EquippedModule(ResonanceSlot::Flow) == flowBeforeStaleApply
            && loadout.EquippedModule(ResonanceSlot::Insight) == insightBeforeStaleApply
            && loadout.ModuleSlotOccupied(ResonanceSlot::Edge) == occupiedBeforeStaleApply[0]
            && loadout.ModuleSlotOccupied(ResonanceSlot::Ward) == occupiedBeforeStaleApply[1]
            && loadout.ModuleSlotOccupied(ResonanceSlot::Flow) == occupiedBeforeStaleApply[2]
            && loadout.ModuleSlotOccupied(ResonanceSlot::Insight) == occupiedBeforeStaleApply[3]
            && loadout.TuningParts() == partsBeforeStaleApply
            && afterStaleApply.attackBonus == stateBeforeStaleApply.attackBonus
            && afterStaleApply.guardBonus == stateBeforeStaleApply.guardBonus
            && afterStaleApply.resourceRecoveryBonus
                == stateBeforeStaleApply.resourceRecoveryBonus
            && afterStaleApply.mobilityBonus == stateBeforeStaleApply.mobilityBonus
            && afterStaleApply.activeResonanceFamilies
                == stateBeforeStaleApply.activeResonanceFamilies
            && afterStaleApply.readinessScore == stateBeforeStaleApply.readinessScore,
        "pass16 stale exact-item preset failure is fully atomic across equipment, parts, and derived profile");

    ShadowbladeLoadout salvageCatalogue;
    const std::array<ResonanceModule, 8> allModules{
        ResonanceModule::CoolingEdge,
        ResonanceModule::RiftEdge,
        ResonanceModule::CoolingWard,
        ResonanceModule::CivicWard,
        ResonanceModule::RiftFlow,
        ResonanceModule::CoolingFlow,
        ResonanceModule::CivicInsight,
        ResonanceModule::RiftInsight,
    };
    bool catalogueAcquired = true;
    bool catalogueSalvaged = true;
    for (ResonanceModule module : allModules) {
        catalogueAcquired = salvageCatalogue.AcquireModule(module) && catalogueAcquired;
    }
    for (ResonanceModule module : allModules) {
        catalogueSalvaged = salvageCatalogue.TrySalvageModule(module)
                == LoadoutActionResult::Success
            && catalogueSalvaged;
    }
    Expect(catalogueAcquired && catalogueSalvaged
            && salvageCatalogue.TuningParts() == 108
            && salvageCatalogue.TuningParts() <= ShadowbladeLoadout::MaximumTuningParts,
        "pass16 salvaging the complete bounded unique catalogue remains deterministic and under the tuning-parts cap");
}
