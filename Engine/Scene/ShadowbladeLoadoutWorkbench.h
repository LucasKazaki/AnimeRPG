#pragma once

#include "Engine/Scene/ShadowbladeLoadout.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

namespace Astral::Scene {

enum class LoadoutWorkbenchFocus : std::uint8_t {
    Balanced,
    Assault,
    Guard,
    Recovery,
    Mobility,
    Count,
};

enum class LoadoutWorkbenchScope : std::uint8_t {
    CurrentSlot,
    WholeBuild,
    Count,
};

enum class LoadoutWorkbenchFamilyFilter : std::uint8_t {
    Any,
    Cooling,
    Rift,
    Civic,
    Count,
};

enum class PresetLabelResult : std::uint8_t {
    Success,
    InvalidSlot,
    EmptyPreset,
    EmptyLabel,
    TooLong,
    InvalidCharacter,
};

struct LoadoutWorkbenchComparison {
    LoadoutActionResult result{LoadoutActionResult::Invalid};
    ShadowbladeLoadoutProfile before{};
    ShadowbladeLoadoutProfile after{};
    int attackDelta{};
    int guardDelta{};
    int resourceRecoveryDelta{};
    int mobilityDelta{};
    int resonanceFamilyDelta{};
    int readinessDelta{};
    bool changesEquipment{};

    bool Valid() const { return result == LoadoutActionResult::Success; }
};

struct LoadoutWorkbenchRecommendation {
    bool available{};
    bool changesEquipment{};
    ResonanceSlot slot{ResonanceSlot::Edge};
    ResonanceModule module{ResonanceModule::CoolingEdge};
    ResonanceFamily family{ResonanceFamily::Cooling};
    int score{};
    LoadoutWorkbenchComparison comparison{};
};

class ShadowbladeLoadoutWorkbench {
public:
    static constexpr std::size_t MaximumPresetLabelLength = 24;

    LoadoutWorkbenchComparison PreviewModule(const ShadowbladeLoadout& loadout,
        ResonanceSlot slot, ResonanceModule module) const {
        LoadoutWorkbenchComparison report{};
        report.before = loadout.Profile();
        ShadowbladeLoadout candidate = loadout;
        report.result = candidate.TryEquipModule(slot, module);
        if (report.result != LoadoutActionResult::Success) {
            report.after = report.before;
            return report;
        }

        report.after = candidate.Profile();
        report.changesEquipment = !loadout.ModuleSlotOccupied(slot)
            || loadout.EquippedModule(slot) != module;
        PopulateDeltas(report);
        return report;
    }

    LoadoutWorkbenchComparison PreviewWeapon(const ShadowbladeLoadout& loadout,
        ShadowbladeWeapon weapon, const CharacterProgression& progression) const {
        LoadoutWorkbenchComparison report{};
        report.before = loadout.Profile();
        ShadowbladeLoadout candidate = loadout;
        report.result = candidate.TryEquipWeapon(weapon, progression);
        if (report.result != LoadoutActionResult::Success) {
            report.after = report.before;
            return report;
        }

        report.after = candidate.Profile();
        report.changesEquipment = loadout.EquippedWeapon() != weapon;
        PopulateDeltas(report);
        return report;
    }

    LoadoutWorkbenchRecommendation RecommendModule(const ShadowbladeLoadout& loadout,
        ResonanceSlot slot, LoadoutWorkbenchFocus focus,
        LoadoutWorkbenchFamilyFilter filter, LoadoutWorkbenchScope scope) const {
        LoadoutWorkbenchRecommendation best{};
        if (!IsValidSlot(slot) || !IsValidFocus(focus)
            || !IsValidFilter(filter) || !IsValidScope(scope)) {
            return best;
        }

        ShadowbladeLoadout withoutSlot = loadout;
        if (withoutSlot.ModuleSlotOccupied(slot)) {
            const auto removed = withoutSlot.UnequipModule(slot);
            if (removed != LoadoutActionResult::Success) return best;
        }
        const ShadowbladeLoadoutProfile slotBaseline = withoutSlot.Profile();
        int bestScore = std::numeric_limits<int>::min();

        for (std::size_t index = 0; index < ShadowbladeLoadout::ModuleCount; ++index) {
            const auto module = static_cast<ResonanceModule>(index);
            if (!MatchesFilter(module, filter)) continue;

            ShadowbladeLoadout candidate = withoutSlot;
            if (candidate.TryEquipModule(slot, module) != LoadoutActionResult::Success) {
                continue;
            }

            const ShadowbladeLoadoutProfile candidateProfile = candidate.Profile();
            const ShadowbladeLoadoutProfile scoredProfile =
                scope == LoadoutWorkbenchScope::CurrentSlot
                ? Difference(candidateProfile, slotBaseline)
                : candidateProfile;
            const int score = FocusScore(scoredProfile, focus);
            if (best.available && score < bestScore) continue;
            if (best.available && score == bestScore
                && index > static_cast<std::size_t>(best.module)) {
                continue;
            }

            best.available = true;
            best.slot = slot;
            best.module = module;
            best.family = ModuleFamily(module);
            best.score = score;
            best.comparison = PreviewModule(loadout, slot, module);
            best.changesEquipment = best.comparison.changesEquipment;
            bestScore = score;
        }
        return best;
    }

    LoadoutActionResult ApplyPreset(ShadowbladeLoadout& loadout, std::size_t slot,
        const CharacterProgression& progression) {
        const LoadoutActionResult result = loadout.ApplyPreset(slot, progression);
        if (result == LoadoutActionResult::Success) {
            lastAppliedPreset_ = slot;
        }
        return result;
    }

    bool HasLastAppliedPreset() const {
        return lastAppliedPreset_ < ShadowbladeLoadout::PresetSlots;
    }

    std::size_t LastAppliedPreset() const {
        return HasLastAppliedPreset() ? lastAppliedPreset_ : ShadowbladeLoadout::PresetSlots;
    }

    LoadoutActionResult ReapplyLastPreset(ShadowbladeLoadout& loadout,
        const CharacterProgression& progression) {
        if (!HasLastAppliedPreset()) return LoadoutActionResult::EmptyPreset;
        return ApplyPreset(loadout, lastAppliedPreset_, progression);
    }

    PresetLabelResult SetPresetLabel(const ShadowbladeLoadout& loadout,
        std::size_t slot, const std::string& requestedLabel) {
        if (slot >= ShadowbladeLoadout::PresetSlots) return PresetLabelResult::InvalidSlot;
        if (!loadout.PresetSaved(slot)) return PresetLabelResult::EmptyPreset;

        const std::string label = TrimSpaces(requestedLabel);
        if (label.empty()) return PresetLabelResult::EmptyLabel;
        if (label.size() > MaximumPresetLabelLength) return PresetLabelResult::TooLong;
        for (unsigned char character : label) {
            if (character < 0x20 || character == 0x7f) {
                return PresetLabelResult::InvalidCharacter;
            }
        }
        presetLabels_[slot] = label;
        return PresetLabelResult::Success;
    }

    PresetLabelResult ClearPresetLabel(const ShadowbladeLoadout& loadout,
        std::size_t slot) {
        if (slot >= ShadowbladeLoadout::PresetSlots) return PresetLabelResult::InvalidSlot;
        if (!loadout.PresetSaved(slot)) return PresetLabelResult::EmptyPreset;
        if (presetLabels_[slot].empty()) return PresetLabelResult::EmptyLabel;
        presetLabels_[slot].clear();
        return PresetLabelResult::Success;
    }

    bool HasPresetLabel(std::size_t slot) const {
        return slot < ShadowbladeLoadout::PresetSlots && !presetLabels_[slot].empty();
    }

    std::string PresetLabel(std::size_t slot) const {
        return slot < ShadowbladeLoadout::PresetSlots ? presetLabels_[slot] : std::string{};
    }

private:
    static bool IsValidSlot(ResonanceSlot slot) {
        return static_cast<std::uint8_t>(slot)
            < static_cast<std::uint8_t>(ResonanceSlot::Count);
    }

    static bool IsValidFocus(LoadoutWorkbenchFocus focus) {
        return static_cast<std::uint8_t>(focus)
            < static_cast<std::uint8_t>(LoadoutWorkbenchFocus::Count);
    }

    static bool IsValidFilter(LoadoutWorkbenchFamilyFilter filter) {
        return static_cast<std::uint8_t>(filter)
            < static_cast<std::uint8_t>(LoadoutWorkbenchFamilyFilter::Count);
    }

    static bool IsValidScope(LoadoutWorkbenchScope scope) {
        return static_cast<std::uint8_t>(scope)
            < static_cast<std::uint8_t>(LoadoutWorkbenchScope::Count);
    }

    static ResonanceFamily ModuleFamily(ResonanceModule module) {
        switch (module) {
        case ResonanceModule::CoolingEdge:
        case ResonanceModule::CoolingWard:
        case ResonanceModule::CoolingFlow:
            return ResonanceFamily::Cooling;
        case ResonanceModule::RiftEdge:
        case ResonanceModule::RiftFlow:
        case ResonanceModule::RiftInsight:
            return ResonanceFamily::Rift;
        case ResonanceModule::CivicWard:
        case ResonanceModule::CivicInsight:
            return ResonanceFamily::Civic;
        case ResonanceModule::Count:
            return ResonanceFamily::Cooling;
        }
        return ResonanceFamily::Cooling;
    }

    static bool MatchesFilter(ResonanceModule module,
        LoadoutWorkbenchFamilyFilter filter) {
        switch (filter) {
        case LoadoutWorkbenchFamilyFilter::Any:
            return true;
        case LoadoutWorkbenchFamilyFilter::Cooling:
            return ModuleFamily(module) == ResonanceFamily::Cooling;
        case LoadoutWorkbenchFamilyFilter::Rift:
            return ModuleFamily(module) == ResonanceFamily::Rift;
        case LoadoutWorkbenchFamilyFilter::Civic:
            return ModuleFamily(module) == ResonanceFamily::Civic;
        case LoadoutWorkbenchFamilyFilter::Count:
            return false;
        }
        return false;
    }

    static ShadowbladeLoadoutProfile Difference(const ShadowbladeLoadoutProfile& after,
        const ShadowbladeLoadoutProfile& before) {
        ShadowbladeLoadoutProfile difference{};
        difference.attackBonus = after.attackBonus - before.attackBonus;
        difference.guardBonus = after.guardBonus - before.guardBonus;
        difference.resourceRecoveryBonus =
            after.resourceRecoveryBonus - before.resourceRecoveryBonus;
        difference.mobilityBonus = after.mobilityBonus - before.mobilityBonus;
        difference.activeResonanceFamilies =
            after.activeResonanceFamilies - before.activeResonanceFamilies;
        difference.readinessScore = after.readinessScore - before.readinessScore;
        return difference;
    }

    static int FocusScore(const ShadowbladeLoadoutProfile& profile,
        LoadoutWorkbenchFocus focus) {
        switch (focus) {
        case LoadoutWorkbenchFocus::Balanced:
            return profile.attackBonus * 2 + profile.guardBonus * 2
                + profile.resourceRecoveryBonus + profile.mobilityBonus
                + profile.activeResonanceFamilies * 5;
        case LoadoutWorkbenchFocus::Assault:
            return profile.attackBonus * 4 + profile.mobilityBonus * 2
                + profile.resourceRecoveryBonus;
        case LoadoutWorkbenchFocus::Guard:
            return profile.guardBonus * 4 + profile.resourceRecoveryBonus * 2
                + profile.attackBonus;
        case LoadoutWorkbenchFocus::Recovery:
            return profile.resourceRecoveryBonus * 4 + profile.guardBonus * 2
                + profile.mobilityBonus;
        case LoadoutWorkbenchFocus::Mobility:
            return profile.mobilityBonus * 4 + profile.attackBonus * 2
                + profile.resourceRecoveryBonus;
        case LoadoutWorkbenchFocus::Count:
            return std::numeric_limits<int>::min();
        }
        return std::numeric_limits<int>::min();
    }

    static void PopulateDeltas(LoadoutWorkbenchComparison& report) {
        report.attackDelta = report.after.attackBonus - report.before.attackBonus;
        report.guardDelta = report.after.guardBonus - report.before.guardBonus;
        report.resourceRecoveryDelta =
            report.after.resourceRecoveryBonus - report.before.resourceRecoveryBonus;
        report.mobilityDelta = report.after.mobilityBonus - report.before.mobilityBonus;
        report.resonanceFamilyDelta =
            report.after.activeResonanceFamilies - report.before.activeResonanceFamilies;
        report.readinessDelta = report.after.readinessScore - report.before.readinessScore;
    }

    static std::string TrimSpaces(const std::string& value) {
        std::size_t first = 0;
        while (first < value.size() && value[first] == ' ') ++first;
        std::size_t last = value.size();
        while (last > first && value[last - 1] == ' ') --last;
        return value.substr(first, last - first);
    }

    std::array<std::string, ShadowbladeLoadout::PresetSlots> presetLabels_{};
    std::size_t lastAppliedPreset_{ShadowbladeLoadout::PresetSlots};
};

} // namespace Astral::Scene
