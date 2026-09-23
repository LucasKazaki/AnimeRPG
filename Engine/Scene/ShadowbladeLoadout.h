#pragma once

#include "Engine/Scene/CharacterProgression.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

namespace Astral::Scene {

enum class ShadowbladeWeapon : std::uint8_t {
    TrainingBlade,
    RiftsteelSabre,
    CryoEdge,
    Count,
};

enum class ResonanceSlot : std::uint8_t {
    Edge,
    Ward,
    Flow,
    Insight,
    Count,
};

enum class ResonanceFamily : std::uint8_t {
    Cooling,
    Rift,
    Civic,
};

enum class ResonanceModule : std::uint8_t {
    CoolingEdge,
    RiftEdge,
    CoolingWard,
    CivicWard,
    RiftFlow,
    CoolingFlow,
    CivicInsight,
    RiftInsight,
    Count,
};

enum class LoadoutActionResult : std::uint8_t {
    Success,
    Invalid,
    Locked,
    NotOwned,
    Equipped,
    EmptyPreset,
    MissingPresetItem,
    InsufficientParts,
    MaximumRank,
    Protected,
};

enum class LoadoutUpgradeKind : std::uint8_t {
    None,
    Weapon,
    Module,
};

struct ShadowbladeLoadoutProfile {
    int attackBonus{};
    int guardBonus{};
    int resourceRecoveryBonus{};
    int mobilityBonus{};
    int activeResonanceFamilies{};
    int readinessScore{};
};

struct ShadowbladeLoadoutPreset {
    bool saved{};
    ShadowbladeWeapon weapon{ShadowbladeWeapon::TrainingBlade};
    std::array<ResonanceModule, 4> modules{
        ResonanceModule::CoolingEdge,
        ResonanceModule::CoolingWard,
        ResonanceModule::CoolingFlow,
        ResonanceModule::CivicInsight,
    };
    std::array<bool, 4> occupied{};
};

struct ShadowbladeUpgradeRecommendation {
    bool available{};
    LoadoutUpgradeKind kind{LoadoutUpgradeKind::None};
    ShadowbladeWeapon weapon{ShadowbladeWeapon::TrainingBlade};
    ResonanceModule module{ResonanceModule::CoolingEdge};
    int currentRank{};
    int nextCost{};
    bool affordable{};
};

class ShadowbladeLoadoutWorkbench;

class ShadowbladeLoadout {
public:
    static constexpr std::size_t WeaponCount =
        static_cast<std::size_t>(ShadowbladeWeapon::Count);
    static constexpr std::size_t ModuleCount =
        static_cast<std::size_t>(ResonanceModule::Count);
    static constexpr std::size_t ModuleSlotCount =
        static_cast<std::size_t>(ResonanceSlot::Count);
    static constexpr std::size_t PresetSlots = 3;
    static constexpr int MaximumTuningParts = 999;
    static constexpr int MaximumTuneRank = 3;

    ShadowbladeLoadout() {
        ownedWeapons_[WeaponIndex(ShadowbladeWeapon::TrainingBlade)] = true;
    }

    bool OwnsWeapon(ShadowbladeWeapon weapon) const {
        return IsValidWeapon(weapon) && ownedWeapons_[WeaponIndex(weapon)];
    }

    bool AcquireWeapon(ShadowbladeWeapon weapon) {
        if (!IsValidWeapon(weapon)) return false;
        const std::size_t index = WeaponIndex(weapon);
        if (ownedWeapons_[index]) return false;
        ownedWeapons_[index] = true;
        return true;
    }

    ShadowbladeWeapon EquippedWeapon() const { return equippedWeapon_; }

    LoadoutActionResult TryEquipWeapon(
        ShadowbladeWeapon weapon, const CharacterProgression& progression) {
        if (!IsValidWeapon(weapon)) return LoadoutActionResult::Invalid;
        if (!OwnsWeapon(weapon)) return LoadoutActionResult::NotOwned;
        if (progression.Level() < RequiredLevel(weapon)) return LoadoutActionResult::Locked;
        equippedWeapon_ = weapon;
        return LoadoutActionResult::Success;
    }

    bool OwnsModule(ResonanceModule module) const {
        return IsValidModule(module) && ownedModules_[ModuleIndex(module)];
    }

    bool AcquireModule(ResonanceModule module) {
        if (!IsValidModule(module)) return false;
        const std::size_t index = ModuleIndex(module);
        if (ownedModules_[index] || salvagedModules_[index]) return false;
        ownedModules_[index] = true;
        return true;
    }

    bool ModuleSlotOccupied(ResonanceSlot slot) const {
        return IsValidSlot(slot) && moduleOccupied_[SlotIndex(slot)];
    }

    ResonanceModule EquippedModule(ResonanceSlot slot) const {
        return IsValidSlot(slot)
            ? equippedModules_[SlotIndex(slot)]
            : ResonanceModule::CoolingEdge;
    }

    LoadoutActionResult TryEquipModule(ResonanceSlot slot, ResonanceModule module) {
        if (!IsValidSlot(slot) || !IsValidModule(module)) return LoadoutActionResult::Invalid;
        if (!OwnsModule(module)) return LoadoutActionResult::NotOwned;
        if (SlotFor(module) != slot) return LoadoutActionResult::Invalid;
        const std::size_t index = SlotIndex(slot);
        equippedModules_[index] = module;
        moduleOccupied_[index] = true;
        return LoadoutActionResult::Success;
    }

    LoadoutActionResult UnequipModule(ResonanceSlot slot) {
        if (!IsValidSlot(slot)) return LoadoutActionResult::Invalid;
        const std::size_t index = SlotIndex(slot);
        if (!moduleOccupied_[index]) return LoadoutActionResult::Invalid;
        moduleOccupied_[index] = false;
        return LoadoutActionResult::Success;
    }

    int WeaponTuneRank(ShadowbladeWeapon weapon) const {
        return IsValidWeapon(weapon) ? weaponTuneRanks_[WeaponIndex(weapon)] : -1;
    }

    int ModuleTuneRank(ResonanceModule module) const {
        return IsValidModule(module) ? moduleTuneRanks_[ModuleIndex(module)] : -1;
    }

    int RemainingWeaponTuneCost(ShadowbladeWeapon weapon) const {
        if (!IsValidWeapon(weapon) || !OwnsWeapon(weapon)) return 0;
        return RemainingTuneCost(weaponTuneRanks_[WeaponIndex(weapon)], true);
    }

    int RemainingModuleTuneCost(ResonanceModule module) const {
        if (!IsValidModule(module) || !OwnsModule(module)) return 0;
        return RemainingTuneCost(moduleTuneRanks_[ModuleIndex(module)], false);
    }

    LoadoutActionResult TryTuneWeapon(
        ShadowbladeWeapon weapon, const CharacterProgression& progression) {
        if (!IsValidWeapon(weapon)) return LoadoutActionResult::Invalid;
        if (!OwnsWeapon(weapon)) return LoadoutActionResult::NotOwned;
        if (progression.Level() < RequiredLevel(weapon)) return LoadoutActionResult::Locked;
        const std::size_t index = WeaponIndex(weapon);
        const int rank = weaponTuneRanks_[index];
        if (rank >= MaximumTuneRank) return LoadoutActionResult::MaximumRank;
        const int cost = WeaponTuneCost(rank);
        if (tuningParts_ < cost) return LoadoutActionResult::InsufficientParts;
        tuningParts_ -= cost;
        ++weaponTuneRanks_[index];
        return LoadoutActionResult::Success;
    }

    LoadoutActionResult TryTuneModule(ResonanceModule module) {
        if (!IsValidModule(module)) return LoadoutActionResult::Invalid;
        if (!OwnsModule(module)) return LoadoutActionResult::NotOwned;
        const std::size_t index = ModuleIndex(module);
        const int rank = moduleTuneRanks_[index];
        if (rank >= MaximumTuneRank) return LoadoutActionResult::MaximumRank;
        const int cost = ModuleTuneCost(rank);
        if (tuningParts_ < cost) return LoadoutActionResult::InsufficientParts;
        tuningParts_ -= cost;
        ++moduleTuneRanks_[index];
        return LoadoutActionResult::Success;
    }

    bool ModuleProtected(ResonanceModule module) const {
        return IsValidModule(module) && moduleProtected_[ModuleIndex(module)];
    }

    LoadoutActionResult SetModuleProtected(ResonanceModule module, bool protect) {
        if (!IsValidModule(module)) return LoadoutActionResult::Invalid;
        if (!OwnsModule(module)) return LoadoutActionResult::NotOwned;
        moduleProtected_[ModuleIndex(module)] = protect;
        return LoadoutActionResult::Success;
    }

    ShadowbladeUpgradeRecommendation RecommendedUpgrade() const {
        ShadowbladeUpgradeRecommendation best{};

        auto considerWeapon = [&](ShadowbladeWeapon weapon) {
            if (!OwnsWeapon(weapon)) return;
            const int rank = WeaponTuneRank(weapon);
            if (rank < 0 || rank >= MaximumTuneRank) return;
            const int cost = WeaponTuneCost(rank);
            if (!best.available || cost < best.nextCost) {
                best.available = true;
                best.kind = LoadoutUpgradeKind::Weapon;
                best.weapon = weapon;
                best.currentRank = rank;
                best.nextCost = cost;
            }
        };

        auto considerModule = [&](ResonanceModule module) {
            if (!OwnsModule(module)) return;
            const int rank = ModuleTuneRank(module);
            if (rank < 0 || rank >= MaximumTuneRank) return;
            const int cost = ModuleTuneCost(rank);
            if (!best.available || cost < best.nextCost) {
                best.available = true;
                best.kind = LoadoutUpgradeKind::Module;
                best.module = module;
                best.currentRank = rank;
                best.nextCost = cost;
            }
        };

        considerWeapon(equippedWeapon_);
        for (std::size_t slot = 0; slot < ModuleSlotCount; ++slot) {
            if (moduleOccupied_[slot]) considerModule(equippedModules_[slot]);
        }
        if (best.available) best.affordable = tuningParts_ >= best.nextCost;
        return best;
    }

    ShadowbladeLoadoutProfile Profile() const {
        ShadowbladeLoadoutProfile profile{};
        const WeaponStats weapon = StatsFor(equippedWeapon_);
        profile.attackBonus += weapon.attack;
        profile.guardBonus += weapon.guard;
        profile.resourceRecoveryBonus += weapon.resource;
        profile.mobilityBonus += weapon.mobility;
        ApplyWeaponTuneBonus(weaponTuneRanks_[WeaponIndex(equippedWeapon_)], profile);

        std::array<int, 3> familyCounts{};
        for (std::size_t slot = 0; slot < ModuleSlotCount; ++slot) {
            if (!moduleOccupied_[slot]) continue;
            const ResonanceModule module = equippedModules_[slot];
            const ModuleStats stats = StatsFor(module);
            profile.attackBonus += stats.attack;
            profile.guardBonus += stats.guard;
            profile.resourceRecoveryBonus += stats.resource;
            profile.mobilityBonus += stats.mobility;
            ApplyModuleTuneBonus(static_cast<ResonanceSlot>(slot),
                moduleTuneRanks_[ModuleIndex(module)], profile);
            ++familyCounts[FamilyIndex(FamilyFor(module))];
        }

        for (std::size_t family = 0; family < familyCounts.size(); ++family) {
            if (familyCounts[family] < 2) continue;
            ++profile.activeResonanceFamilies;
            ApplyFamilyBonus(static_cast<ResonanceFamily>(family), profile);
        }

        const int score = 20 + profile.attackBonus * 2 + profile.guardBonus * 2
            + profile.resourceRecoveryBonus + profile.mobilityBonus
            + profile.activeResonanceFamilies * 5;
        profile.readinessScore = std::clamp(score, 0, 100);
        return profile;
    }

    LoadoutActionResult SavePreset(std::size_t slot) {
        if (slot >= PresetSlots) return LoadoutActionResult::Invalid;
        ShadowbladeLoadoutPreset preset{};
        preset.saved = true;
        preset.weapon = equippedWeapon_;
        preset.modules = equippedModules_;
        preset.occupied = moduleOccupied_;
        presets_[slot] = preset;
        return LoadoutActionResult::Success;
    }

    bool PresetSaved(std::size_t slot) const {
        return slot < PresetSlots && presets_[slot].saved;
    }

    const ShadowbladeLoadoutPreset* Preset(std::size_t slot) const {
        return slot < PresetSlots && presets_[slot].saved ? &presets_[slot] : nullptr;
    }

    LoadoutActionResult ApplyPreset(
        std::size_t slot, const CharacterProgression& progression) {
        if (slot >= PresetSlots) return LoadoutActionResult::Invalid;
        const ShadowbladeLoadoutPreset& preset = presets_[slot];
        if (!preset.saved) return LoadoutActionResult::EmptyPreset;
        if (!IsValidWeapon(preset.weapon) || !OwnsWeapon(preset.weapon)) {
            return LoadoutActionResult::MissingPresetItem;
        }
        if (progression.Level() < RequiredLevel(preset.weapon)) {
            return LoadoutActionResult::Locked;
        }
        for (std::size_t index = 0; index < ModuleSlotCount; ++index) {
            if (!preset.occupied[index]) continue;
            const ResonanceModule module = preset.modules[index];
            const ResonanceSlot slotId = static_cast<ResonanceSlot>(index);
            if (!IsValidModule(module) || !OwnsModule(module) || SlotFor(module) != slotId) {
                return LoadoutActionResult::MissingPresetItem;
            }
        }

        equippedWeapon_ = preset.weapon;
        equippedModules_ = preset.modules;
        moduleOccupied_ = preset.occupied;
        return LoadoutActionResult::Success;
    }

    int TuningParts() const { return tuningParts_; }

    LoadoutActionResult TrySalvageModule(ResonanceModule module) {
        if (!IsValidModule(module)) return LoadoutActionResult::Invalid;
        if (!OwnsModule(module)) return LoadoutActionResult::NotOwned;
        if (IsEquipped(module)) return LoadoutActionResult::Equipped;
        const std::size_t index = ModuleIndex(module);
        if (moduleProtected_[index]) return LoadoutActionResult::Protected;

        const int recovered = SalvageValue(module)
            + ModuleInvestment(moduleTuneRanks_[index]) / 2;
        ownedModules_[index] = false;
        salvagedModules_[index] = true;
        moduleProtected_[index] = false;
        moduleTuneRanks_[index] = 0;
        const int room = MaximumTuningParts - tuningParts_;
        tuningParts_ += std::min(recovered, std::max(0, room));
        return LoadoutActionResult::Success;
    }

private:
    friend class ShadowbladeLoadoutWorkbench;

    struct WeaponStats {
        int attack{};
        int guard{};
        int resource{};
        int mobility{};
    };

    struct ModuleStats {
        int attack{};
        int guard{};
        int resource{};
        int mobility{};
    };

    static bool IsValidWeapon(ShadowbladeWeapon weapon) {
        return static_cast<std::uint8_t>(weapon)
            < static_cast<std::uint8_t>(ShadowbladeWeapon::Count);
    }

    static bool IsValidModule(ResonanceModule module) {
        return static_cast<std::uint8_t>(module)
            < static_cast<std::uint8_t>(ResonanceModule::Count);
    }

    static bool IsValidSlot(ResonanceSlot slot) {
        return static_cast<std::uint8_t>(slot)
            < static_cast<std::uint8_t>(ResonanceSlot::Count);
    }

    static std::size_t WeaponIndex(ShadowbladeWeapon weapon) {
        return static_cast<std::size_t>(weapon);
    }

    static std::size_t ModuleIndex(ResonanceModule module) {
        return static_cast<std::size_t>(module);
    }

    static std::size_t SlotIndex(ResonanceSlot slot) {
        return static_cast<std::size_t>(slot);
    }

    static std::size_t FamilyIndex(ResonanceFamily family) {
        return static_cast<std::size_t>(family);
    }

    static int RequiredLevel(ShadowbladeWeapon weapon) {
        switch (weapon) {
        case ShadowbladeWeapon::TrainingBlade: return 1;
        case ShadowbladeWeapon::RiftsteelSabre: return 8;
        case ShadowbladeWeapon::CryoEdge: return 14;
        case ShadowbladeWeapon::Count: return std::numeric_limits<int>::max();
        }
        return std::numeric_limits<int>::max();
    }

    static WeaponStats StatsFor(ShadowbladeWeapon weapon) {
        switch (weapon) {
        case ShadowbladeWeapon::TrainingBlade: return {4, 1, 0, 0};
        case ShadowbladeWeapon::RiftsteelSabre: return {8, 2, 1, 1};
        case ShadowbladeWeapon::CryoEdge: return {6, 4, 3, 0};
        case ShadowbladeWeapon::Count: return {};
        }
        return {};
    }

    static ResonanceSlot SlotFor(ResonanceModule module) {
        switch (module) {
        case ResonanceModule::CoolingEdge:
        case ResonanceModule::RiftEdge:
            return ResonanceSlot::Edge;
        case ResonanceModule::CoolingWard:
        case ResonanceModule::CivicWard:
            return ResonanceSlot::Ward;
        case ResonanceModule::RiftFlow:
        case ResonanceModule::CoolingFlow:
            return ResonanceSlot::Flow;
        case ResonanceModule::CivicInsight:
        case ResonanceModule::RiftInsight:
            return ResonanceSlot::Insight;
        case ResonanceModule::Count:
            return ResonanceSlot::Count;
        }
        return ResonanceSlot::Count;
    }

    static ResonanceFamily FamilyFor(ResonanceModule module) {
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

    static ModuleStats StatsFor(ResonanceModule module) {
        switch (module) {
        case ResonanceModule::CoolingEdge: return {2, 0, 1, 0};
        case ResonanceModule::RiftEdge: return {3, 0, 0, 1};
        case ResonanceModule::CoolingWard: return {0, 3, 1, 0};
        case ResonanceModule::CivicWard: return {0, 4, 0, 0};
        case ResonanceModule::RiftFlow: return {1, 0, 1, 2};
        case ResonanceModule::CoolingFlow: return {0, 1, 3, 0};
        case ResonanceModule::CivicInsight: return {1, 2, 1, 0};
        case ResonanceModule::RiftInsight: return {2, 0, 1, 1};
        case ResonanceModule::Count: return {};
        }
        return {};
    }

    static int WeaponTuneCost(int currentRank) {
        switch (currentRank) {
        case 0: return 6;
        case 1: return 10;
        case 2: return 14;
        default: return 0;
        }
    }

    static int ModuleTuneCost(int currentRank) {
        switch (currentRank) {
        case 0: return 4;
        case 1: return 8;
        case 2: return 12;
        default: return 0;
        }
    }

    static int RemainingTuneCost(int currentRank, bool weapon) {
        if (currentRank < 0 || currentRank >= MaximumTuneRank) return 0;
        int total = 0;
        for (int rank = currentRank; rank < MaximumTuneRank; ++rank) {
            total += weapon ? WeaponTuneCost(rank) : ModuleTuneCost(rank);
        }
        return total;
    }

    static int ModuleInvestment(int rank) {
        int total = 0;
        for (int current = 0; current < std::clamp(rank, 0, MaximumTuneRank); ++current) {
            total += ModuleTuneCost(current);
        }
        return total;
    }

    static int SalvageValue(ResonanceModule module) {
        switch (module) {
        case ResonanceModule::CoolingEdge:
        case ResonanceModule::RiftEdge:
        case ResonanceModule::CoolingWard:
        case ResonanceModule::CivicWard:
            return 12;
        case ResonanceModule::RiftFlow:
        case ResonanceModule::CoolingFlow:
        case ResonanceModule::CivicInsight:
        case ResonanceModule::RiftInsight:
            return 15;
        case ResonanceModule::Count:
            return 0;
        }
        return 0;
    }

    static void ApplyWeaponTuneBonus(int rank, ShadowbladeLoadoutProfile& profile) {
        const int bounded = std::clamp(rank, 0, MaximumTuneRank);
        profile.attackBonus += bounded;
        profile.guardBonus += bounded / 2;
    }

    static void ApplyModuleTuneBonus(
        ResonanceSlot slot, int rank, ShadowbladeLoadoutProfile& profile) {
        const int bounded = std::clamp(rank, 0, MaximumTuneRank);
        switch (slot) {
        case ResonanceSlot::Edge:
            profile.attackBonus += bounded;
            break;
        case ResonanceSlot::Ward:
            profile.guardBonus += bounded;
            break;
        case ResonanceSlot::Flow:
            profile.resourceRecoveryBonus += bounded;
            break;
        case ResonanceSlot::Insight:
            profile.mobilityBonus += bounded;
            break;
        case ResonanceSlot::Count:
            break;
        }
    }

    static void ApplyFamilyBonus(
        ResonanceFamily family, ShadowbladeLoadoutProfile& profile) {
        switch (family) {
        case ResonanceFamily::Cooling:
            profile.guardBonus += 2;
            profile.resourceRecoveryBonus += 2;
            break;
        case ResonanceFamily::Rift:
            profile.attackBonus += 3;
            profile.mobilityBonus += 2;
            break;
        case ResonanceFamily::Civic:
            profile.guardBonus += 3;
            profile.resourceRecoveryBonus += 1;
            break;
        }
    }

    bool IsEquipped(ResonanceModule module) const {
        for (std::size_t index = 0; index < ModuleSlotCount; ++index) {
            if (moduleOccupied_[index] && equippedModules_[index] == module) return true;
        }
        return false;
    }

    std::array<bool, WeaponCount> ownedWeapons_{};
    std::array<bool, ModuleCount> ownedModules_{};
    std::array<bool, ModuleCount> salvagedModules_{};
    ShadowbladeWeapon equippedWeapon_{ShadowbladeWeapon::TrainingBlade};
    std::array<ResonanceModule, ModuleSlotCount> equippedModules_{
        ResonanceModule::CoolingEdge,
        ResonanceModule::CoolingWard,
        ResonanceModule::CoolingFlow,
        ResonanceModule::CivicInsight,
    };
    std::array<bool, ModuleSlotCount> moduleOccupied_{};
    std::array<ShadowbladeLoadoutPreset, PresetSlots> presets_{};
    std::array<std::string, PresetSlots> presetLabels_{};
    std::size_t lastAppliedPreset_{PresetSlots};
    std::array<std::uint8_t, WeaponCount> weaponTuneRanks_{};
    std::array<std::uint8_t, ModuleCount> moduleTuneRanks_{};
    std::array<bool, ModuleCount> moduleProtected_{};
    int tuningParts_{};
};

} // namespace Astral::Scene
