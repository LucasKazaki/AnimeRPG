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

    void Configure(EncounterChallengeDifficulty difficulty,
        EncounterTacticalFocus focus,
        EncounterScoringMode scoringMode = EncounterScoringMode::Balanced) {
        enabled_ = true;
        difficulty_ = difficulty;
        focus_ = focus;
        scoringMode_ = scoringMode;
    }

    bool Enabled() const { return enabled_; }
    EncounterChallengeDifficulty Difficulty() const { return difficulty_; }
    EncounterTacticalFocus Focus() const { return focus_; }
    EncounterScoringMode ScoringMode() const { return scoringMode_; }

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
        return firstClearGranted_[DifficultyIndex(difficulty)];
    }

private:
    static std::int64_t SaturatingAdd(std::int64_t left, std::int64_t right) {
        if (right <= 0) return left;
        const std::int64_t maximum = std::numeric_limits<std::int64_t>::max();
        return left > maximum - right ? maximum : left + right;
    }

    static std::int64_t SaturatingMultiply(std::int64_t left, std::int64_t right) {
        if (left <= 0 || right <= 0) return 0;
        const std::int64_t maximum = std::numeric_limits<std::int64_t>::max();
        return left > maximum / right ? maximum : left * right;
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
            bronze = 2;
            silver = 3;
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
    bool firstClearGranted_[3]{};
};

} // namespace Astral::Scene