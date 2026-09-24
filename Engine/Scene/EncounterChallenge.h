#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>

namespace Astral::Scene {

enum class EncounterChallengeDifficulty {
    Standard,
    Expert,
    Apex,
};

enum class EncounterTacticalFocus {
    Balanced,
    Reaction,
    Stagger,
    Finisher,
};

enum class EncounterScoringMode {
    Balanced,
    TechniqueFirst,
    SpecialOffensive,
};

enum class EncounterTacticalBuff {
    None,
    ShadowTempo,
    RiftPressure,
    BalancedFlow,
};

enum class EncounterScoreCoachFocus {
    None,
    Damage,
    Technique,
    Combo,
    Time,
};

enum class EncounterChallengeRank {
    None,
    Bronze,
    Silver,
    Gold,
};

enum class EncounterTimeGrade {
    Bronze,
    Silver,
    Gold,
};

struct EncounterChallengeSnapshot {
    std::int64_t baseCombatScore{};
    int reactionCount{};
    int staggerCount{};
    int finisherCount{};
    bool flawless{};
    EncounterTimeGrade timeGrade{EncounterTimeGrade::Bronze};

    // GAME pass 36: activation-local raw score evidence for the optional
    // SpecialOffensive evaluation. Legacy scoring continues to use
    // baseCombatScore exactly as before.
    std::int64_t damageScore{};
    std::int64_t techniqueScore{};
    int activeTechniqueChain{};
};

struct EncounterScoreBreakdown {
    std::int64_t damageScore{};
    std::int64_t techniqueScore{};
    std::int64_t techniqueComboBonus{};
    std::int64_t tacticalBuffBonus{};
    int activeTechniqueChain{};
    int timeCoefficientPercent{};
    std::int64_t scoreBeforeDifficulty{};
    std::int64_t finalScore{};
    EncounterTacticalBuff tacticalBuff{EncounterTacticalBuff::None};
};

struct EncounterChallengeResult {
    std::int64_t score{};
    EncounterChallengeRank rank{EncounterChallengeRank::None};
    int sideGoalsCompleted{};
    bool flawlessGoal{};
    bool techniqueVarietyGoal{};
    bool speedGoal{};
    bool tacticalGoal{};
    bool firstClear{};
    float firstClearRewardRequested{};
    float firstClearRewardApplied{};

    EncounterScoreBreakdown breakdown{};
    std::int64_t previousBestScore{};
    bool newPersonalBest{};
    EncounterScoreCoachFocus coachFocus{EncounterScoreCoachFocus::None};
};

class EncounterChallengeTracker {
public:
    static constexpr int SideGoalScore = 150;
    static constexpr int TechniqueFirstSideGoalScore = 300;
    static constexpr int TacticalEventScore = 40;
    static constexpr int TechniqueFirstTacticalEventScore = 100;
    static constexpr float StandardFirstClearReward = 10.0f;
    static constexpr float ExpertFirstClearReward = 15.0f;
    static constexpr float ApexFirstClearReward = 20.0f;

    // SpecialOffensive is an original Astral adaptation of the current
    // damage + technique + combo + time-coefficient challenge pattern. The
    // percentages are deliberately simple, bounded game rules rather than
    // copied values from a reference title.
    static constexpr int MaximumTechniqueCombo = 4;
    static constexpr int TechniqueComboStepPercent = 10;
    static constexpr int GoldTimeCoefficientPercent = 125;
    static constexpr int SilverTimeCoefficientPercent = 100;
    static constexpr int BronzeTimeCoefficientPercent = 75;

    void Configure(EncounterChallengeDifficulty difficulty,
        EncounterTacticalFocus focus,
        EncounterScoringMode scoringMode = EncounterScoringMode::Balanced,
        EncounterTacticalBuff tacticalBuff = EncounterTacticalBuff::None) {
        if (!ValidDifficulty(difficulty) || !ValidFocus(focus)
            || !ValidScoringMode(scoringMode) || !ValidTacticalBuff(tacticalBuff)) {
            enabled_ = false;
            difficulty_ = EncounterChallengeDifficulty::Standard;
            focus_ = EncounterTacticalFocus::Balanced;
            scoringMode_ = EncounterScoringMode::Balanced;
            tacticalBuff_ = EncounterTacticalBuff::None;
            return;
        }
        enabled_ = true;
        difficulty_ = difficulty;
        focus_ = focus;
        scoringMode_ = scoringMode;
        tacticalBuff_ = tacticalBuff;
    }

    bool Enabled() const { return enabled_; }
    EncounterChallengeDifficulty Difficulty() const { return difficulty_; }
    EncounterTacticalFocus Focus() const { return focus_; }
    EncounterScoringMode ScoringMode() const { return scoringMode_; }
    EncounterTacticalBuff TacticalBuff() const { return tacticalBuff_; }

    EncounterChallengeResult Resolve(const EncounterChallengeSnapshot& snapshot) {
        EncounterChallengeResult result{};
        if (!enabled_) return result;

        result.flawlessGoal = snapshot.flawless;
        const int techniqueKinds = (snapshot.reactionCount > 0 ? 1 : 0)
            + (snapshot.staggerCount > 0 ? 1 : 0)
            + (snapshot.finisherCount > 0 ? 1 : 0);
        result.techniqueVarietyGoal = techniqueKinds >= 2;
        result.speedGoal = snapshot.timeGrade != EncounterTimeGrade::Bronze;
        result.tacticalGoal = TacticalEventCount(snapshot) > 0;
        result.sideGoalsCompleted = (result.flawlessGoal ? 1 : 0)
            + (result.techniqueVarietyGoal ? 1 : 0)
            + (result.speedGoal ? 1 : 0)
            + (result.tacticalGoal ? 1 : 0);

        if (scoringMode_ == EncounterScoringMode::SpecialOffensive) {
            ResolveSpecialOffensive(snapshot, result);
        } else {
            ResolveLegacyScore(snapshot, result);
        }
        result.rank = RankFor(result.sideGoalsCompleted, snapshot.timeGrade);

        const int index = DifficultyIndex(difficulty_);
        if (!firstClearGranted_[index]) {
            firstClearGranted_[index] = true;
            result.firstClear = true;
            result.firstClearRewardRequested = FirstClearRewardFor(difficulty_);
        }
        return result;
    }

    bool FirstClearGranted(EncounterChallengeDifficulty difficulty) const {
        return ValidDifficulty(difficulty) && firstClearGranted_[DifficultyIndex(difficulty)];
    }

    std::int64_t BestSpecialOffensiveScore(EncounterChallengeDifficulty difficulty) const {
        if (!ValidDifficulty(difficulty)) return 0;
        const int index = DifficultyIndex(difficulty);
        return specialBestValid_[index] ? specialBestScore_[index] : 0;
    }

private:
    static bool ValidDifficulty(EncounterChallengeDifficulty difficulty) {
        return difficulty == EncounterChallengeDifficulty::Standard
            || difficulty == EncounterChallengeDifficulty::Expert
            || difficulty == EncounterChallengeDifficulty::Apex;
    }

    static bool ValidFocus(EncounterTacticalFocus focus) {
        return focus == EncounterTacticalFocus::Balanced
            || focus == EncounterTacticalFocus::Reaction
            || focus == EncounterTacticalFocus::Stagger
            || focus == EncounterTacticalFocus::Finisher;
    }

    static bool ValidScoringMode(EncounterScoringMode mode) {
        return mode == EncounterScoringMode::Balanced
            || mode == EncounterScoringMode::TechniqueFirst
            || mode == EncounterScoringMode::SpecialOffensive;
    }

    static bool ValidTacticalBuff(EncounterTacticalBuff buff) {
        return buff == EncounterTacticalBuff::None
            || buff == EncounterTacticalBuff::ShadowTempo
            || buff == EncounterTacticalBuff::RiftPressure
            || buff == EncounterTacticalBuff::BalancedFlow;
    }

    static std::int64_t SaturatingAdd(std::int64_t left, std::int64_t right) {
        if (right <= 0) return std::max<std::int64_t>(0, left);
        left = std::max<std::int64_t>(0, left);
        const std::int64_t maximum = std::numeric_limits<std::int64_t>::max();
        return left > maximum - right ? maximum : left + right;
    }

    static std::int64_t SaturatingMultiply(std::int64_t left, std::int64_t right) {
        if (left <= 0 || right <= 0) return 0;
        const std::int64_t maximum = std::numeric_limits<std::int64_t>::max();
        return left > maximum / right ? maximum : left * right;
    }

    static std::int64_t ScalePercent(std::int64_t value, int percent) {
        if (value <= 0 || percent <= 0) return 0;
        const std::int64_t quotient = value / 100;
        const std::int64_t remainder = value % 100;
        return SaturatingAdd(SaturatingMultiply(quotient, percent),
            remainder * static_cast<std::int64_t>(percent) / 100);
    }

    static int DifficultyIndex(EncounterChallengeDifficulty difficulty) {
        switch (difficulty) {
        case EncounterChallengeDifficulty::Expert: return 1;
        case EncounterChallengeDifficulty::Apex: return 2;
        case EncounterChallengeDifficulty::Standard:
        default: return 0;
        }
    }

    static float FirstClearRewardFor(EncounterChallengeDifficulty difficulty) {
        switch (difficulty) {
        case EncounterChallengeDifficulty::Expert: return ExpertFirstClearReward;
        case EncounterChallengeDifficulty::Apex: return ApexFirstClearReward;
        case EncounterChallengeDifficulty::Standard:
        default: return StandardFirstClearReward;
        }
    }

    std::int64_t TacticalEventCount(const EncounterChallengeSnapshot& snapshot) const {
        switch (focus_) {
        case EncounterTacticalFocus::Reaction: return std::max(0, snapshot.reactionCount);
        case EncounterTacticalFocus::Stagger: return std::max(0, snapshot.staggerCount);
        case EncounterTacticalFocus::Finisher: return std::max(0, snapshot.finisherCount);
        case EncounterTacticalFocus::Balanced:
        default:
            return static_cast<std::int64_t>(std::max(0, snapshot.reactionCount))
                + static_cast<std::int64_t>(std::max(0, snapshot.staggerCount))
                + static_cast<std::int64_t>(std::max(0, snapshot.finisherCount));
        }
    }

    void ResolveLegacyScore(const EncounterChallengeSnapshot& snapshot,
        EncounterChallengeResult& result) const {
        const std::int64_t base = std::max<std::int64_t>(0, snapshot.baseCombatScore);
        const int eventPoints = scoringMode_ == EncounterScoringMode::TechniqueFirst
            ? TechniqueFirstTacticalEventScore
            : TacticalEventScore;
        const int goalPoints = scoringMode_ == EncounterScoringMode::TechniqueFirst
            ? TechniqueFirstSideGoalScore
            : SideGoalScore;
        const std::int64_t tacticalScore = SaturatingMultiply(
            TacticalEventCount(snapshot), eventPoints);
        const std::int64_t sideGoalScore = SaturatingMultiply(
            result.sideGoalsCompleted, goalPoints);
        const std::int64_t weightedBase = scoringMode_ == EncounterScoringMode::TechniqueFirst
            ? base / 2
            : base;
        result.score = ApplyDifficultyMultiplier(SaturatingAdd(
            SaturatingAdd(weightedBase, tacticalScore), sideGoalScore));
    }

    void ResolveSpecialOffensive(const EncounterChallengeSnapshot& snapshot,
        EncounterChallengeResult& result) {
        EncounterScoreBreakdown breakdown{};
        breakdown.damageScore = std::max<std::int64_t>(0, snapshot.damageScore);
        breakdown.techniqueScore = std::max<std::int64_t>(0, snapshot.techniqueScore);
        breakdown.activeTechniqueChain = std::min(MaximumTechniqueCombo,
            std::max(0, snapshot.activeTechniqueChain));
        breakdown.tacticalBuff = tacticalBuff_;

        if (breakdown.activeTechniqueChain > 1) {
            breakdown.techniqueComboBonus = ScalePercent(breakdown.techniqueScore,
                (breakdown.activeTechniqueChain - 1) * TechniqueComboStepPercent);
        }

        switch (tacticalBuff_) {
        case EncounterTacticalBuff::ShadowTempo:
            if (breakdown.activeTechniqueChain >= 2) {
                breakdown.tacticalBuffBonus = breakdown.techniqueScore / 5;
            }
            break;
        case EncounterTacticalBuff::RiftPressure:
            if (snapshot.timeGrade == EncounterTimeGrade::Gold) {
                breakdown.tacticalBuffBonus = breakdown.damageScore / 10;
            } else if (snapshot.timeGrade == EncounterTimeGrade::Silver) {
                breakdown.tacticalBuffBonus = breakdown.damageScore / 20;
            }
            break;
        case EncounterTacticalBuff::BalancedFlow:
            breakdown.tacticalBuffBonus =
                std::min(breakdown.damageScore, breakdown.techniqueScore) / 4;
            break;
        case EncounterTacticalBuff::None:
        default:
            break;
        }

        switch (snapshot.timeGrade) {
        case EncounterTimeGrade::Gold:
            breakdown.timeCoefficientPercent = GoldTimeCoefficientPercent;
            break;
        case EncounterTimeGrade::Silver:
            breakdown.timeCoefficientPercent = SilverTimeCoefficientPercent;
            break;
        case EncounterTimeGrade::Bronze:
        default:
            breakdown.timeCoefficientPercent = BronzeTimeCoefficientPercent;
            break;
        }

        std::int64_t subtotal = SaturatingAdd(
            breakdown.damageScore, breakdown.techniqueScore);
        subtotal = SaturatingAdd(subtotal, breakdown.techniqueComboBonus);
        subtotal = SaturatingAdd(subtotal, breakdown.tacticalBuffBonus);
        breakdown.scoreBeforeDifficulty = ScalePercent(
            subtotal, breakdown.timeCoefficientPercent);
        breakdown.finalScore = ApplyDifficultyMultiplier(breakdown.scoreBeforeDifficulty);
        result.score = breakdown.finalScore;
        result.breakdown = breakdown;

        const int index = DifficultyIndex(difficulty_);
        result.previousBestScore = specialBestValid_[index] ? specialBestScore_[index] : 0;
        if (result.score > 0
            && (!specialBestValid_[index] || result.score > specialBestScore_[index])) {
            specialBestValid_[index] = true;
            specialBestScore_[index] = result.score;
            result.newPersonalBest = true;
        }
        result.coachFocus = CoachFocus(snapshot, breakdown);
    }

    static EncounterScoreCoachFocus CoachFocus(
        const EncounterChallengeSnapshot& snapshot,
        const EncounterScoreBreakdown& breakdown) {
        if (breakdown.damageScore <= 0) return EncounterScoreCoachFocus::Damage;
        if (breakdown.techniqueScore <= 0) return EncounterScoreCoachFocus::Technique;
        if (breakdown.activeTechniqueChain < 2) return EncounterScoreCoachFocus::Combo;
        if (snapshot.timeGrade == EncounterTimeGrade::Bronze) {
            return EncounterScoreCoachFocus::Time;
        }
        if (breakdown.activeTechniqueChain < MaximumTechniqueCombo) {
            return EncounterScoreCoachFocus::Combo;
        }
        if (snapshot.timeGrade != EncounterTimeGrade::Gold) {
            return EncounterScoreCoachFocus::Time;
        }
        if (breakdown.techniqueScore < breakdown.damageScore) {
            return EncounterScoreCoachFocus::Technique;
        }
        if (breakdown.damageScore < breakdown.techniqueScore) {
            return EncounterScoreCoachFocus::Damage;
        }
        return EncounterScoreCoachFocus::None;
    }

    std::int64_t ApplyDifficultyMultiplier(std::int64_t score) const {
        switch (difficulty_) {
        case EncounterChallengeDifficulty::Expert:
            return SaturatingAdd(score, score / 10);
        case EncounterChallengeDifficulty::Apex:
            return SaturatingAdd(score, score / 4);
        case EncounterChallengeDifficulty::Standard:
        default:
            return score;
        }
    }

    EncounterChallengeRank RankFor(int sideGoals, EncounterTimeGrade timeGrade) const {
        int bronze = 1;
        int silver = 2;
        int gold = 3;
        if (difficulty_ == EncounterChallengeDifficulty::Expert) {
            bronze = 2;
            silver = 3;
            gold = 4;
        } else if (difficulty_ == EncounterChallengeDifficulty::Apex) {
            bronze = 3;
            silver = 4;
            gold = 4;
            if (sideGoals >= gold && timeGrade != EncounterTimeGrade::Gold) {
                return EncounterChallengeRank::Silver;
            }
        }
        if (sideGoals >= gold) return EncounterChallengeRank::Gold;
        if (sideGoals >= silver) return EncounterChallengeRank::Silver;
        if (sideGoals >= bronze) return EncounterChallengeRank::Bronze;
        return EncounterChallengeRank::None;
    }

    bool enabled_{};
    EncounterChallengeDifficulty difficulty_{EncounterChallengeDifficulty::Standard};
    EncounterTacticalFocus focus_{EncounterTacticalFocus::Balanced};
    EncounterScoringMode scoringMode_{EncounterScoringMode::Balanced};
    EncounterTacticalBuff tacticalBuff_{EncounterTacticalBuff::None};
    bool firstClearGranted_[3]{};
    std::int64_t specialBestScore_[3]{};
    bool specialBestValid_[3]{};
};

} // namespace Astral::Scene
