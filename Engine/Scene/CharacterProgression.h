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

enum class ShadowSkill {
    Dash,
    FatalStrike,
    Defense,
};

enum class ProgressionActionResult {
    Success,
    Invalid,
    Locked,
    InsufficientResources,
    Maxed,
    EmptyPreset,
    AlreadyClaimed,
};

struct ProgressionRewardReport {
    int experienceApplied{};
    int levelsGained{};
    int masteryPointsApplied{};
    int enhancementMaterialsApplied{};
};

struct JourneyClaimReport {
    ProgressionActionResult result{ProgressionActionResult::Invalid};
    ProgressionRewardReport reward{};
};

struct BuildPreset {
    bool saved{};
    std::array<CoreTalent, 2> talents{CoreTalent::ShadowStep, CoreTalent::EclipseEdge};
    std::array<bool, 2> occupied{};
};

struct TrainingPlanStatus {
    bool active{};
    int targetsComplete{};
    int targetsTotal{};
    int remainingLevels{};
    int remainingWeaponTiers{};
    std::array<int, 3> remainingTalentTiers{};
    std::array<int, 3> remainingSkillRanks{};

    bool Complete() const { return active && targetsComplete == targetsTotal; }
};

class CharacterProgression {
public:
    static constexpr int MaximumLevel = 20;
    static constexpr int MaximumMasteryRank = 10;
    static constexpr int MasteryPointsPerRank = 50;
    static constexpr int MaximumTalentTier = 3;
    static constexpr int MaximumWeaponTier = 5;
    static constexpr int MaximumWeaponAwakeningRank = 2;
    static constexpr int MaximumSkillRank = 5;
    static constexpr int SkillPracticePerRank = 10;
    static constexpr std::size_t EquippedTalentSlots = 2;
    static constexpr std::size_t BuildPresetSlots = 3;
    static constexpr std::size_t TalentCount = 3;
    static constexpr std::size_t SkillCount = 3;
    static constexpr std::size_t JourneyMilestoneCount = 4;

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
    int WeaponAwakeningRank() const { return weaponAwakeningRank_; }
    int EnhancementMaterials() const { return enhancementMaterials_; }

    int SkillPractice(ShadowSkill skill) const {
        return IsValidSkill(skill) ? skillPractice_[SkillIndex(skill)] : 0;
    }
    int SkillRank(ShadowSkill skill) const {
        if (!IsValidSkill(skill)) return 0;
        const int practice = skillPractice_[SkillIndex(skill)];
        return std::min(MaximumSkillRank, 1 + practice / SkillPracticePerRank);
    }

    int RecordSkillPractice(ShadowSkill skill, int amount) {
        if (!IsValidSkill(skill) || amount <= 0) return 0;
        const std::size_t index = SkillIndex(skill);
        const int maximumPractice = (MaximumSkillRank - 1) * SkillPracticePerRank;
        const int room = maximumPractice - skillPractice_[index];
        const int applied = std::min(amount, std::max(0, room));
        skillPractice_[index] += applied;
        return applied;
    }

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

    bool ShadowCryptFirstClearClaimed() const { return shadowCryptFirstClearClaimed_; }

    ProgressionRewardReport ClaimShadowCryptFirstClearReward(int experience,
        int masteryPoints, int enhancementMaterials, bool& granted) {
        granted = false;
        if (shadowCryptFirstClearClaimed_
            || experience < 0 || masteryPoints < 0 || enhancementMaterials < 0) {
            return {};
        }
        shadowCryptFirstClearClaimed_ = true;
        granted = true;
        return GrantRewards(experience, masteryPoints, enhancementMaterials);
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

    ProgressionActionResult AwakenWeapon() {
        if (weaponAwakeningRank_ >= MaximumWeaponAwakeningRank) {
            return ProgressionActionResult::Maxed;
        }
        if (weaponTier_ < MaximumWeaponTier) return ProgressionActionResult::Locked;
        const int requiredLevel = weaponAwakeningRank_ == 0 ? 15 : 20;
        if (level_ < requiredLevel) return ProgressionActionResult::Locked;
        const int cost = weaponAwakeningRank_ == 0 ? 100 : 200;
        if (enhancementMaterials_ < cost) {
            return ProgressionActionResult::InsufficientResources;
        }
        enhancementMaterials_ -= cost;
        ++weaponAwakeningRank_;
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

    ProgressionActionResult SetTrainingPlan(int targetLevel, int targetWeaponTier,
        const std::array<int, TalentCount>& targetTalentTiers,
        const std::array<int, SkillCount>& targetSkillRanks) {
        if (targetLevel < 1 || targetLevel > MaximumLevel
            || targetWeaponTier < 0 || targetWeaponTier > MaximumWeaponTier) {
            return ProgressionActionResult::Invalid;
        }
        for (int tier : targetTalentTiers) {
            if (tier < 0 || tier > MaximumTalentTier) return ProgressionActionResult::Invalid;
        }
        for (int rank : targetSkillRanks) {
            if (rank < 1 || rank > MaximumSkillRank) return ProgressionActionResult::Invalid;
        }
        trainingPlanActive_ = true;
        targetLevel_ = targetLevel;
        targetWeaponTier_ = targetWeaponTier;
        targetTalentTiers_ = targetTalentTiers;
        targetSkillRanks_ = targetSkillRanks;
        return ProgressionActionResult::Success;
    }

    void ClearTrainingPlan() {
        trainingPlanActive_ = false;
    }

    TrainingPlanStatus CurrentTrainingPlanStatus() const {
        TrainingPlanStatus status{};
        status.active = trainingPlanActive_;
        if (!trainingPlanActive_) return status;

        status.targetsTotal = 2 + static_cast<int>(TalentCount + SkillCount);
        status.remainingLevels = std::max(0, targetLevel_ - level_);
        status.remainingWeaponTiers = std::max(0, targetWeaponTier_ - weaponTier_);
        if (status.remainingLevels == 0) ++status.targetsComplete;
        if (status.remainingWeaponTiers == 0) ++status.targetsComplete;

        for (std::size_t index = 0; index < TalentCount; ++index) {
            status.remainingTalentTiers[index] =
                std::max(0, targetTalentTiers_[index] - talentTiers_[index]);
            if (status.remainingTalentTiers[index] == 0) ++status.targetsComplete;
        }
        for (std::size_t index = 0; index < SkillCount; ++index) {
            status.remainingSkillRanks[index] =
                std::max(0, targetSkillRanks_[index] - SkillRank(SkillFromIndex(index)));
            if (status.remainingSkillRanks[index] == 0) ++status.targetsComplete;
        }
        return status;
    }

    int CombatReadinessScore() const {
        int score = level_ * 2;
        score += weaponTier_ * 4;
        score += weaponAwakeningRank_ * 5;
        for (int tier : talentTiers_) score += tier * 2;
        for (std::size_t index = 0; index < SkillCount; ++index) {
            score += std::max(0, SkillRank(SkillFromIndex(index)) - 1);
        }
        return std::min(100, std::max(0, score));
    }

    static int JourneyMilestoneRequiredLevel(std::size_t milestone) {
        static constexpr std::array<int, JourneyMilestoneCount> levels{5, 10, 15, 20};
        return milestone < levels.size() ? levels[milestone] : 0;
    }

    bool JourneyMilestoneClaimed(std::size_t milestone) const {
        return milestone < JourneyMilestoneCount && journeyMilestoneClaimed_[milestone];
    }

    JourneyClaimReport ClaimJourneyMilestone(std::size_t milestone) {
        JourneyClaimReport report{};
        if (milestone >= JourneyMilestoneCount) {
            report.result = ProgressionActionResult::Invalid;
            return report;
        }
        if (journeyMilestoneClaimed_[milestone]) {
            report.result = ProgressionActionResult::AlreadyClaimed;
            return report;
        }
        if (level_ < JourneyMilestoneRequiredLevel(milestone)) {
            report.result = ProgressionActionResult::Locked;
            return report;
        }

        journeyMilestoneClaimed_[milestone] = true;
        const int masteryReward = 20 + static_cast<int>(milestone) * 10;
        const int materialReward = 10 + static_cast<int>(milestone) * 5;
        report.reward = GrantRewards(0, masteryReward, materialReward);
        report.result = ProgressionActionResult::Success;
        return report;
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

    static bool IsValidSkill(ShadowSkill skill) {
        return skill == ShadowSkill::Dash
            || skill == ShadowSkill::FatalStrike
            || skill == ShadowSkill::Defense;
    }

    static std::size_t SkillIndex(ShadowSkill skill) {
        switch (skill) {
        case ShadowSkill::FatalStrike: return 1;
        case ShadowSkill::Defense: return 2;
        case ShadowSkill::Dash:
        default: return 0;
        }
    }

    static ShadowSkill SkillFromIndex(std::size_t index) {
        switch (index) {
        case 1: return ShadowSkill::FatalStrike;
        case 2: return ShadowSkill::Defense;
        case 0:
        default: return ShadowSkill::Dash;
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
    std::array<int, TalentCount> talentTiers_{};
    int weaponTier_{};
    int weaponAwakeningRank_{};
    std::array<int, SkillCount> skillPractice_{};
    std::array<CoreTalent, EquippedTalentSlots> equippedTalents_{
        CoreTalent::ShadowStep, CoreTalent::EclipseEdge};
    std::array<bool, EquippedTalentSlots> equippedOccupied_{};
    std::array<BuildPreset, BuildPresetSlots> presets_{};

    bool trainingPlanActive_{};
    int targetLevel_{1};
    int targetWeaponTier_{};
    std::array<int, TalentCount> targetTalentTiers_{};
    std::array<int, SkillCount> targetSkillRanks_{1, 1, 1};
    std::array<bool, JourneyMilestoneCount> journeyMilestoneClaimed_{};
    bool shadowCryptFirstClearClaimed_{};
};

} // namespace Astral::Scene
