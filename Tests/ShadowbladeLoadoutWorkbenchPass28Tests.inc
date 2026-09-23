void TestShadowbladeLoadoutWorkbenchPass28() {
    using namespace Astral::Scene;

    CharacterProgression progression;
    ShadowbladeActions actions;
    ShadowbladeLoadout& loadout = actions.Loadout();
    ShadowbladeLoadoutWorkbench& workbench = actions.LoadoutWorkbench();

    Expect(loadout.AcquireModule(ResonanceModule::CoolingEdge),
        "pass28 setup acquires Cooling Edge");
    Expect(loadout.AcquireModule(ResonanceModule::RiftEdge),
        "pass28 setup acquires Rift Edge");
    Expect(loadout.AcquireModule(ResonanceModule::CoolingWard),
        "pass28 setup acquires Cooling Ward");
    Expect(loadout.AcquireModule(ResonanceModule::CivicWard),
        "pass28 setup acquires Civic Ward");
    Expect(loadout.AcquireModule(ResonanceModule::CivicInsight),
        "pass28 setup acquires Civic Insight");

    // GAME-137: candidate previews are exact, non-mutating comparisons for both
    // modules and weapons. Failed previews also preserve the authoritative loadout.
    Expect(loadout.TryEquipModule(ResonanceSlot::Edge, ResonanceModule::CoolingEdge)
            == LoadoutActionResult::Success,
        "pass28 equips baseline Cooling Edge");
    const ShadowbladeLoadoutProfile beforePreview = loadout.Profile();
    const auto riftPreview = workbench.PreviewModule(
        loadout, ResonanceSlot::Edge, ResonanceModule::RiftEdge);
    Expect(riftPreview.Valid() && riftPreview.changesEquipment,
        "pass28 module preview accepts an owned compatible alternative");
    Expect(riftPreview.attackDelta == 1 && riftPreview.guardDelta == 0
            && riftPreview.resourceRecoveryDelta == -1
            && riftPreview.mobilityDelta == 1 && riftPreview.readinessDelta == 2,
        "pass28 module preview reports exact profile deltas");
    Expect(loadout.EquippedModule(ResonanceSlot::Edge) == ResonanceModule::CoolingEdge
            && loadout.Profile().readinessScore == beforePreview.readinessScore,
        "pass28 module preview never mutates live equipment");

    const auto unavailableWeapon = workbench.PreviewWeapon(
        loadout, ShadowbladeWeapon::CryoEdge, progression);
    Expect(unavailableWeapon.result == LoadoutActionResult::NotOwned
            && !unavailableWeapon.changesEquipment
            && unavailableWeapon.readinessDelta == 0
            && loadout.EquippedWeapon() == ShadowbladeWeapon::TrainingBlade,
        "pass28 rejected weapon preview is non-mutating");

    // GAME-138: explicit build focus changes the recommendation while using only
    // owned, slot-compatible modules.
    const auto assault = workbench.RecommendModule(loadout, ResonanceSlot::Edge,
        LoadoutWorkbenchFocus::Assault, LoadoutWorkbenchFamilyFilter::Any,
        LoadoutWorkbenchScope::CurrentSlot);
    const auto recovery = workbench.RecommendModule(loadout, ResonanceSlot::Edge,
        LoadoutWorkbenchFocus::Recovery, LoadoutWorkbenchFamilyFilter::Any,
        LoadoutWorkbenchScope::CurrentSlot);
    Expect(assault.available && assault.module == ResonanceModule::RiftEdge,
        "pass28 assault focus prefers the stronger Rift Edge contribution");
    Expect(recovery.available && recovery.module == ResonanceModule::CoolingEdge,
        "pass28 recovery focus prefers Cooling Edge resource recovery");
    Expect(loadout.EquippedModule(ResonanceSlot::Edge) == ResonanceModule::CoolingEdge,
        "pass28 recommendations do not auto-equip candidates");

    // GAME-139: a family filter constrains suggestions without fabricating a
    // candidate when the requested family has no owned compatible module.
    const auto coolingWard = workbench.RecommendModule(loadout, ResonanceSlot::Ward,
        LoadoutWorkbenchFocus::Balanced, LoadoutWorkbenchFamilyFilter::Cooling,
        LoadoutWorkbenchScope::CurrentSlot);
    const auto civicWard = workbench.RecommendModule(loadout, ResonanceSlot::Ward,
        LoadoutWorkbenchFocus::Balanced, LoadoutWorkbenchFamilyFilter::Civic,
        LoadoutWorkbenchScope::CurrentSlot);
    const auto missingRiftWard = workbench.RecommendModule(loadout, ResonanceSlot::Ward,
        LoadoutWorkbenchFocus::Balanced, LoadoutWorkbenchFamilyFilter::Rift,
        LoadoutWorkbenchScope::CurrentSlot);
    Expect(coolingWard.available && coolingWard.module == ResonanceModule::CoolingWard,
        "pass28 Cooling filter returns the owned Cooling Ward");
    Expect(civicWard.available && civicWard.module == ResonanceModule::CivicWard,
        "pass28 Civic filter returns the owned Civic Ward");
    Expect(!missingRiftWard.available,
        "pass28 unavailable family filter fails closed instead of inventing gear");
    Expect(!workbench.RecommendModule(loadout, ResonanceSlot::Ward,
            LoadoutWorkbenchFocus::Balanced, LoadoutWorkbenchFamilyFilter::Count,
            LoadoutWorkbenchScope::CurrentSlot).available,
        "pass28 malformed family filter fails closed");

    // GAME-140: CurrentSlot evaluates the candidate's bounded slot contribution,
    // while WholeBuild includes the resulting family synergy across equipped gear.
    Expect(loadout.TryEquipModule(ResonanceSlot::Ward, ResonanceModule::CoolingWard)
            == LoadoutActionResult::Success,
        "pass28 scope setup equips Cooling Ward");
    Expect(loadout.TryEquipModule(ResonanceSlot::Insight, ResonanceModule::CivicInsight)
            == LoadoutActionResult::Success,
        "pass28 scope setup equips Civic Insight");
    const auto currentSlot = workbench.RecommendModule(loadout, ResonanceSlot::Edge,
        LoadoutWorkbenchFocus::Balanced, LoadoutWorkbenchFamilyFilter::Any,
        LoadoutWorkbenchScope::CurrentSlot);
    const auto wholeBuild = workbench.RecommendModule(loadout, ResonanceSlot::Edge,
        LoadoutWorkbenchFocus::Balanced, LoadoutWorkbenchFamilyFilter::Any,
        LoadoutWorkbenchScope::WholeBuild);
    Expect(currentSlot.available && currentSlot.module == ResonanceModule::RiftEdge,
        "pass28 current-slot scope prefers the stronger isolated Edge module");
    Expect(wholeBuild.available && wholeBuild.module == ResonanceModule::CoolingEdge,
        "pass28 whole-build scope recognizes Cooling family synergy");
    Expect(loadout.EquippedModule(ResonanceSlot::Edge) == ResonanceModule::CoolingEdge,
        "pass28 scope comparison remains preview-only");

    // GAME-141: the live Shadowblade owner remembers only a successful preset
    // application and can reapply it after later manual equipment changes.
    Expect(loadout.SavePreset(0) == LoadoutActionResult::Success,
        "pass28 saves baseline preset zero");
    Expect(loadout.TryEquipModule(ResonanceSlot::Edge, ResonanceModule::RiftEdge)
            == LoadoutActionResult::Success,
        "pass28 changes live equipment before second preset");
    Expect(loadout.SavePreset(1) == LoadoutActionResult::Success,
        "pass28 saves alternate preset one");
    Expect(!workbench.HasLastAppliedPreset(),
        "pass28 saving alone does not fabricate a last-applied preset");
    Expect(workbench.ApplyPreset(loadout, 0, progression) == LoadoutActionResult::Success
            && workbench.HasLastAppliedPreset() && workbench.LastAppliedPreset() == 0,
        "pass28 successful preset application becomes the remembered selection");
    Expect(workbench.ApplyPreset(loadout, 2, progression) == LoadoutActionResult::EmptyPreset
            && workbench.LastAppliedPreset() == 0,
        "pass28 failed preset application cannot replace remembered selection");
    Expect(loadout.TryEquipModule(ResonanceSlot::Edge, ResonanceModule::RiftEdge)
            == LoadoutActionResult::Success,
        "pass28 changes equipment before one-action reapply");
    Expect(workbench.ReapplyLastPreset(loadout, progression) == LoadoutActionResult::Success
            && loadout.EquippedModule(ResonanceSlot::Edge) == ResonanceModule::CoolingEdge,
        "pass28 one-action reapply restores the exact remembered preset");

    // QOL-029: saved presets can carry short human-readable labels with spaces,
    // and labels can be cleared without modifying the saved build or live gear.
    const ShadowbladeLoadoutProfile beforeLabel = loadout.Profile();
    Expect(workbench.SetPresetLabel(loadout, 0, "  Mall Patrol  ")
            == PresetLabelResult::Success
            && workbench.PresetLabel(0) == "Mall Patrol",
        "pass28 preset labels trim outer spaces while preserving internal spaces");
    Expect(workbench.SetPresetLabel(loadout, 0, std::string(25, 'x'))
            == PresetLabelResult::TooLong
            && workbench.PresetLabel(0) == "Mall Patrol",
        "pass28 overlong label rejection preserves the previous label");
    Expect(workbench.SetPresetLabel(loadout, 0, "bad\tlabel")
            == PresetLabelResult::InvalidCharacter
            && workbench.PresetLabel(0) == "Mall Patrol",
        "pass28 control-character label rejection is non-mutating");
    Expect(workbench.SetPresetLabel(loadout, 2, "Unused")
            == PresetLabelResult::EmptyPreset,
        "pass28 cannot label an unsaved preset slot");
    Expect(workbench.ClearPresetLabel(loadout, 0) == PresetLabelResult::Success
            && !workbench.HasPresetLabel(0)
            && loadout.PresetSaved(0)
            && loadout.Profile().readinessScore == beforeLabel.readinessScore,
        "pass28 clearing a label preserves saved and equipped gameplay state");
    Expect(workbench.ClearPresetLabel(loadout, 0) == PresetLabelResult::EmptyLabel,
        "pass28 repeated label clear is deterministic");
}
