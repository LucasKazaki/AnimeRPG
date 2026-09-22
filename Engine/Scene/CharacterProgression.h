#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace Astral::Scene {

enum class CoreTalent {
    ShadowStep,
    EclipseEdge,
    BreakerFocus,
};

enum class ProgressionActionResult {
    Success,
    Invalid,
    Locked,
    InsufficientResources,
    Maxed,
    EmptyPreset,
};

struct ProgressionRewardReport {
    int experienceApplied{};
    int levelsGained{};
    int masteryPointsApplied{};
    int enhancementMaterialsApplied{};
};

struct BuildPreset {
    bool saved{};
    std::array<CoreTalent, 2> talents{CoreTalent::ShadowStep, CoreTalent::EclipseEdge};
    std::array<bool, 2> occupied{};
};

class CharacterProgression {
public:
    static constexpr int MaximumLevel = 20;
    static constexpr int MaximumMasteryRank = 10;
    static constexpr int MasteryPointsPerRank = 50;
    static constexpr int MaximumTalentTier = 3;
    static constexpr int MaximumWeaponTier = 5;
    static constexpr std::size_t EquippedTalentSlots = 2;
    static constexpr std::size_t BuildPresetSlots = 3;

    int Level() const { return level_; }
    int ExperienceIntoLevel() const { return experienceIntoLevel_; }
    int ExperienceRequiredForNextLevel() const {
        return level_ >= MaximumLevel ? 0 : level_ * 100;
    }

    int MasteryRank() const {
        const std::int64_t earned = std::max<std::int64_t>(0, masteryPointsEarned_);
        const std::int64_t rank = 1 + earned / MasteryPointsPerRank;
        return static_cast<int>(std::min<std::int64_t>(MaximumMasteryRank, rank));
    }
    int AvailableMasteryPoints() const { return masteryPointsAvailable_; }
    std::int64_t LifetimeMasteryPoints() const { return masteryPointsEarned_; }

    int TalentTier(CoreTalent talent) const {
        return IsValidTalent(talent) ? talentTiers_[TalentIndex(talent)] : 0;
    }
    int WeaponTier() const { return weaponTier_; }
    int EnhancementMaterials() const { return enhancementMaterials_; }

    ProgressionRewardReport GrantRewards(int experience, int masteryPoints,
        int enhancementMaterials) {
        ProgressionRewardReport report{};
        if (experience > 0) {
            report.experienceApplied = GrantExperience(experience, report.levelsGained);
        }
        if (masteryPoints > 0) {
            report.masteryPointsApplied = GrantMasteryPoints(masteryPoints);
        }
        if (enhancementMaterials > 0) {
            report.enhancementMaterialsApplied =
                GrantEnhancementMaterials(enhancementMaterials);
        }
        return report;
    }

    int GrantExperience(int amount, int& levelsGained) {
        levelsGained = 0;
        if (amount <= 0 || level_ >= MaximumLevel) return 0;

        int remaining = amount;
        int applied = 0;
        while (remaining > 0 && level_ < MaximumLevel) {
            const int needed = ExperienceRequiredForNextLevel() - experienceIntoLevel_;
            const int step = std::min(remaining, needed);
            experienceIntoLevel_ += step;
            remaining -= step;
            applied += step;
            if (experienceIntoLevel_ >= ExperienceRequiredForNextLevel()) {
                experienceIntoLevel_ = 0;
                ++level_;
                ++levelsGained;
            }
        }
        if (level_ >= MaximumLevel) experienceIntoLevel_ = 0;
        return applied;
    }

    int GrantMasteryPoints(int amount) {
        if (amount <= 0) return 0;
        const int room = std::numeric_limits<int>::max() - masteryPointsAvailable_;
        const int applied = std::min(amount, room);
        masteryPointsAvailable_ += applied;
        masteryPointsEarned_ = SaturatingAdd(masteryPointsEarned_, applied);
        return applied;
    }

    int GrantEnhancementMaterials(int amount) {
        if (amount <= 0) return 0;
        const int room = std::numeric_limits<int>::max() - enhancementMaterials_;
        const int applied = std::min(amount, room);
        enhancementMaterials_ += applied;
        return applied;
    }

    ProgressionActionResult UpgradeTalent(CoreTalent talent) {
        if (!IsValidTalent(talent)) return ProgressionActionResult::Invalid;
        const std::size_t index = TalentIndex(talent);
        const int currentTier = talentTiers_[index];
        if (currentTier >= MaximumTalentTier) return ProgressionActionResult::Maxed;

        const int requiredMasteryRank = 1 + currentTier * 2;
        if (MasteryRank() < requiredMasteryRank) return ProgressionActionResult::Locked;

        const int cost = 20 * (currentTier + 1);
        if (masteryPointsAvailable_ < cost) {
            return ProgressionActionResult::InsufficientResources;
        }
        masteryPointsAvailable_ -= cost;
        talentTiers_[index] = currentTier + 1;
        return ProgressionActionResult::Success;
    }

    ProgressionActionResult UpgradeWeapon() {
        if (weaponTier_ >= MaximumWeaponTier) return ProgressionActionResult::Maxed;
        const int requiredLevel = 1 + weaponTier_ * 3;
        if (level_ < requiredLevel) return ProgressionActionResult::Locked;
        const int cost = 10 * (weaponTier_ + 1);
        if (enhancementMaterials_ < cost) {
            return ProgressionActionResult::InsufficientResources;
        }
        enhancementMaterials_ -= cost;
        ++weaponTier_;
        return ProgressionActionResult::Success;
    }

    ProgressionActionResult EquipTalent(std::size_t slot, CoreTalent talent) {
        if (slot >= EquippedTalentSlots || !IsValidTalent(talent)) {
            return ProgressionActionResult::Invalid;
        }
        if (TalentTier(talent) <= 0) return ProgressionActionResult::Locked;
        for (std::size_t index = 0; index < EquippedTalentSlots; ++index) {
            if (index != slot && equippedOccupied_[index] && equippedTalents_[index] == talent) {
                return ProgressionActionResult::Invalid;
            }
        }
        equippedTalents_[slot] = talent;
        equippedOccupied_[slot] = true;
        return ProgressionActionResult::Success;
    }

    bool TalentSlotOccupied(std::size_t slot) const {
        return slot < EquippedTalentSlots && equippedOccupied_[slot];
    }
    CoreTalent EquippedTalent(std::size_t slot) const {
        return slot < EquippedTalentSlots ? equippedTalents_[slot] : CoreTalent::ShadowStep;
    }

    ProgressionActionResult SaveBuildPreset(std::size_t slot) {
        if (slot >= BuildPresetSlots) return ProgressionActionResult::Invalid;
        presets_[slot].saved = true;
        presets_[slot].talents = equippedTalents_;
        presets_[slot].occupied = equippedOccupied_;
        return ProgressionActionResult::Success;
    }

    ProgressionActionResult LoadBuildPreset(std::size_t slot) {
        if (slot >= BuildPresetSlots) return ProgressionActionResult::Invalid;
        const BuildPreset& preset = presets_[slot];
        if (!preset.saved) return ProgressionActionResult::EmptyPreset;
        for (std::size_t index = 0; index < EquippedTalentSlots; ++index) {
            if (preset.occupied[index]
                && (!IsValidTalent(preset.talents[index])
                    || TalentTier(preset.talents[index]) <= 0)) {
                return ProgressionActionResult::Locked;
            }
            if (preset.occupied[index]) {
                for (std::size_t other = index + 1; other < EquippedTalentSlots; ++other) {
                    if (preset.occupied[other] && preset.talents[index] == preset.talents[other]) {
                        return ProgressionActionResult::Invalid;
                    }
                }
            }
        }
        equippedTalents_ = preset.talents;
        equippedOccupied_ = preset.occupied;
        return ProgressionActionResult::Success;
    }

    bool BuildPresetSaved(std::size_t slot) const {
        return slot < BuildPresetSlots && presets_[slot].saved;
    }

private:
    static bool IsValidTalent(CoreTalent talent) {
        return talent == CoreTalent::ShadowStep
            || talent == CoreTalent::EclipseEdge
            || talent == CoreTalent::BreakerFocus;
    }

    static std::size_t TalentIndex(CoreTalent talent) {
        switch (talent) {
        case CoreTalent::EclipseEdge: return 1;
        case CoreTalent::BreakerFocus: return 2;
        case CoreTalent::ShadowStep:
        default: return 0;
        }
    }

    static std::int64_t SaturatingAdd(std::int64_t left, std::int64_t right) {
        if (right <= 0) return left;
        const std::int64_t maximum = std::numeric_limits<std::int64_t>::max();
        return left > maximum - right ? maximum : left + right;
    }

    int level_{1};
    int experienceIntoLevel_{};
    int masteryPointsAvailable_{};
    std::int64_t masteryPointsEarned_{};
    int enhancementMaterials_{};
    std::array<int, 3> talentTiers_{};
    int weaponTier_{};
    std::array<CoreTalent, EquippedTalentSlots> equippedTalents_{
        CoreTalent::ShadowStep, CoreTalent::EclipseEdge};
    std::array<bool, EquippedTalentSlots> equippedOccupied_{};
    std::array<BuildPreset, BuildPresetSlots> presets_{};
};

} // namespace Astral::Scene
