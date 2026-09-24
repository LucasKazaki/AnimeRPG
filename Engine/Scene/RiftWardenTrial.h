#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace Astral::Scene {

enum class RiftWardenDifficulty : std::uint8_t {
    Story,
    Standard,
    Expert,
};

enum class RiftWardenPhase : std::uint8_t {
    Opening,
    Fracture,
    Overload,
    Defeated,
};

enum class RiftWardenAttack : std::uint8_t {
    RiftSlash,
    GravityPulse,
    EchoBurst,
    StaggerOpening,
};

enum class RiftWardenResponse : std::uint8_t {
    Dodge,
    Guard,
    Strike,
};

enum class RiftWardenCue : std::uint8_t {
    None,
    Evade,
    Brace,
    Punish,
};

enum class RiftWardenCoachReason : std::uint8_t {
    None,
    WrongResponse,
    LateResponse,
    MissedPunish,
};

enum class RiftWardenResolution : std::uint8_t {
    Rejected,
    PerfectDefense,
    Hit,
    Punish,
    MissedOpening,
};

enum class RiftWardenMastery : std::uint8_t {
    Unranked,
    Bronze,
    Silver,
    Gold,
};

struct RiftWardenTelegraph {
    RiftWardenAttack attack{RiftWardenAttack::RiftSlash};
    RiftWardenResponse recommendedResponse{RiftWardenResponse::Dodge};
    RiftWardenCue cue{RiftWardenCue::None};
    double responseWindowSeconds{};
    std::uint32_t sequence{};
};

struct RiftWardenActionReport {
    bool accepted{};
    RiftWardenResolution resolution{RiftWardenResolution::Rejected};
    int bossDamage{};
    int playerDamage{};
    int postureGain{};
    bool staggerOpened{};
    bool phaseChanged{};
    bool complete{};
    bool responseMatched{};
    double timingMarginSeconds{};
};

struct RiftWardenRecord {
    bool valid{};
    int score{};
    RiftWardenMastery mastery{RiftWardenMastery::Unranked};
    int damageTaken{};
    int perfectDefenses{};
    int punishes{};
    int missedOpenings{};
    int bestStreak{};
    double clearSeconds{};
};

struct RiftWardenTrainingRecord {
    int attempts{};
    int successes{};
    int bestSuccessStreak{};
    RiftWardenPhase lastSeenPhase{RiftWardenPhase::Opening};
};

struct RiftWardenPracticeRecommendation {
    bool available{};
    RiftWardenAttack attack{RiftWardenAttack::RiftSlash};
    RiftWardenPhase phase{RiftWardenPhase::Opening};
    RiftWardenResponse response{RiftWardenResponse::Dodge};
    int attempts{};
    int successes{};
};

struct RiftWardenCoachTip {
    bool available{};
    RiftWardenAttack attack{RiftWardenAttack::RiftSlash};
    RiftWardenResponse expectedResponse{RiftWardenResponse::Dodge};
    RiftWardenCue cue{RiftWardenCue::None};
    RiftWardenCoachReason reason{RiftWardenCoachReason::None};
    double responseWindowSeconds{};
    double observedReactionSeconds{};
};

struct RiftWardenBriefing {
    bool active{};
    bool complete{};
    bool practice{};
    RiftWardenDifficulty difficulty{RiftWardenDifficulty::Story};
    RiftWardenPhase phase{RiftWardenPhase::Opening};
    int bossHealth{};
    int bossMaxHealth{};
    int posture{};
    bool staggerOpen{};
    int damageTaken{};
    int perfectDefenses{};
    int punishes{};
    int missedOpenings{};
    int currentStreak{};
    int bestStreak{};
    double elapsedSeconds{};
    RiftWardenTelegraph telegraph{};
    RiftWardenPracticeRecommendation practiceRecommendation{};
    RiftWardenCoachTip coachTip{};
};

// GAME-domain-only deterministic boss challenge. Rendering, input, animation,
// audio, physics, save backends, networking and engine timing remain outside
// this type. Callers provide explicit reaction timing and progression gates.
class RiftWardenTrial {
public:
    static constexpr int MaximumPosture = 100;
    static constexpr int MaximumPerfectDefenses = 16384;
    static constexpr int MaximumMissedOpenings = 4096;
    static constexpr int MaximumDamageTaken = 9999;
    static constexpr int MaximumTrainingAttempts = 4096;
    static constexpr double MaximumElapsedSeconds = 3600.0;
    static constexpr double StaggerResponseWindowSeconds = 2.0;

    bool Begin(bool shadowCryptComplete, RiftWardenDifficulty difficulty) {
        if (!shadowCryptComplete || active_ || !DifficultyValid(difficulty)
            || !DifficultyUnlocked(difficulty)) {
            return false;
        }
        ResetAttempt(difficulty, false, RiftWardenPhase::Opening);
        return true;
    }

    // QOL-033: after at least one ranked clear, players may rehearse any phase
    // on an already-unlocked difficulty. Practice never updates unlocks/records.
    bool BeginPractice(bool shadowCryptComplete, RiftWardenDifficulty difficulty,
        RiftWardenPhase phase) {
        if (!shadowCryptComplete || active_ || !hasRankedClear_
            || !DifficultyValid(difficulty) || !DifficultyUnlocked(difficulty)
            || phase == RiftWardenPhase::Defeated || !PhaseValid(phase)) {
            return false;
        }
        ResetAttempt(difficulty, true, phase);
        return true;
    }

    bool AdvanceTime(double deltaSeconds) {
        if (!active_ || !std::isfinite(deltaSeconds) || deltaSeconds < 0.0) return false;
        elapsedSeconds_ = std::min(MaximumElapsedSeconds, elapsedSeconds_ + deltaSeconds);
        return true;
    }

    RiftWardenActionReport Resolve(RiftWardenResponse response, double reactionSeconds) {
        RiftWardenActionReport report{};
        if (!active_ || !ResponseValid(response) || !std::isfinite(reactionSeconds)
            || reactionSeconds < 0.0) {
            return report;
        }

        const RiftWardenPhase resolvedPhase = phase_;
        const RiftWardenTelegraph telegraph = CurrentTelegraph();
        report.accepted = true;
        report.responseMatched = response == telegraph.recommendedResponse;
        report.timingMarginSeconds = telegraph.responseWindowSeconds - reactionSeconds;

        if (staggerOpen_) {
            const bool success = report.responseMatched && report.timingMarginSeconds >= 0.0;
            if (success) {
                report.resolution = RiftWardenResolution::Punish;
                report.bossDamage = PunishDamage(difficulty_);
                bossHealth_ = std::max(0, bossHealth_ - report.bossDamage);
                ++punishes_;
                ++currentStreak_;
                bestStreak_ = std::max(bestStreak_, currentStreak_);
            } else {
                report.resolution = RiftWardenResolution::MissedOpening;
                missedOpenings_ = std::min(MaximumMissedOpenings, missedOpenings_ + 1);
                currentStreak_ = 0;
                lastCoachTip_ = BuildCoachTip(
                    telegraph, RiftWardenCoachReason::MissedPunish, reactionSeconds);
            }
            RecordTrainingAttempt(telegraph.attack, resolvedPhase, success);
            posture_ = 0;
            staggerOpen_ = false;
            const RiftWardenPhase oldPhase = phase_;
            UpdatePhaseFromHealth();
            report.phaseChanged = phase_ != oldPhase;
            report.complete = complete_;
            return report;
        }

        const bool correct = report.responseMatched && report.timingMarginSeconds >= 0.0;
        if (correct) {
            report.resolution = RiftWardenResolution::PerfectDefense;
            report.postureGain = PostureGain(difficulty_);
            posture_ = std::min(MaximumPosture, posture_ + report.postureGain);
            perfectDefenses_ = std::min(MaximumPerfectDefenses, perfectDefenses_ + 1);
            ++currentStreak_;
            bestStreak_ = std::max(bestStreak_, currentStreak_);
            if (posture_ >= MaximumPosture) {
                staggerOpen_ = true;
                report.staggerOpened = true;
            }
        } else {
            report.resolution = RiftWardenResolution::Hit;
            report.playerDamage = IncomingDamage(difficulty_, phase_);
            damageTaken_ = std::min(MaximumDamageTaken, damageTaken_ + report.playerDamage);
            posture_ = std::max(0, posture_ - 25);
            currentStreak_ = 0;
            lastCoachTip_ = BuildCoachTip(telegraph,
                report.responseMatched ? RiftWardenCoachReason::LateResponse
                                       : RiftWardenCoachReason::WrongResponse,
                reactionSeconds);
        }
        RecordTrainingAttempt(telegraph.attack, resolvedPhase, correct);
        ++attackCursor_;
        return report;
    }

    RiftWardenTelegraph CurrentTelegraph() const {
        RiftWardenTelegraph result{};
        result.sequence = attackCursor_;
        if (staggerOpen_) {
            result.attack = RiftWardenAttack::StaggerOpening;
            result.recommendedResponse = RiftWardenResponse::Strike;
            result.cue = RiftWardenCue::Punish;
            result.responseWindowSeconds = StaggerResponseWindowSeconds;
            return result;
        }
        result.attack = PatternFor(phase_, attackCursor_);
        result.recommendedResponse = RecommendedResponse(result.attack);
        result.cue = CueFor(result.attack);
        result.responseWindowSeconds = DefenseWindow(difficulty_, phase_);
        return result;
    }

    RiftWardenBriefing Briefing() const {
        RiftWardenBriefing result{};
        result.active = active_;
        result.complete = complete_;
        result.practice = practice_;
        result.difficulty = difficulty_;
        result.phase = phase_;
        result.bossHealth = bossHealth_;
        result.bossMaxHealth = bossMaxHealth_;
        result.posture = posture_;
        result.staggerOpen = staggerOpen_;
        result.damageTaken = damageTaken_;
        result.perfectDefenses = perfectDefenses_;
        result.punishes = punishes_;
        result.missedOpenings = missedOpenings_;
        result.currentStreak = currentStreak_;
        result.bestStreak = bestStreak_;
        result.elapsedSeconds = elapsedSeconds_;
        result.telegraph = CurrentTelegraph();
        result.practiceRecommendation = PracticeRecommendation();
        result.coachTip = lastCoachTip_;
        return result;
    }

    RiftWardenRecord BestRecord(RiftWardenDifficulty difficulty) const {
        if (!DifficultyValid(difficulty)) return {};
        return bestRecords_[DifficultyIndex(difficulty)];
    }

    bool DifficultyUnlocked(RiftWardenDifficulty difficulty) const {
        if (!DifficultyValid(difficulty)) return false;
        return difficultyUnlocked_[DifficultyIndex(difficulty)];
    }

    bool HasRankedClear() const { return hasRankedClear_; }

    bool AttackLearned(RiftWardenAttack attack) const {
        return AttackValid(attack) && TrainingRecord(attack).attempts > 0;
    }

    RiftWardenTrainingRecord TrainingRecord(RiftWardenAttack attack) const {
        if (!AttackValid(attack)) return {};
        return trainingRecords_[AttackIndex(attack)];
    }

    RiftWardenPracticeRecommendation PracticeRecommendation() const {
        RiftWardenPracticeRecommendation result{};
        bool found = false;
        int bestFailures = -1;
        for (std::size_t index = 0; index < trainingRecords_.size(); ++index) {
            const RiftWardenTrainingRecord& record = trainingRecords_[index];
            if (record.attempts <= 0) continue;
            const int failures = std::max(0, record.attempts - record.successes);
            bool better = !found || failures > bestFailures;
            if (!better && failures == bestFailures) {
                const RiftWardenTrainingRecord& current =
                    trainingRecords_[AttackIndex(result.attack)];
                const long long lhs = static_cast<long long>(record.successes) * current.attempts;
                const long long rhs = static_cast<long long>(current.successes) * record.attempts;
                better = lhs < rhs;
            }
            if (!better) continue;
            found = true;
            bestFailures = failures;
            result.available = true;
            result.attack = static_cast<RiftWardenAttack>(index);
            result.phase = record.lastSeenPhase;
            result.response = RecommendedResponse(result.attack);
            result.attempts = record.attempts;
            result.successes = record.successes;
        }
        return result;
    }

    RiftWardenCoachTip LastCoachTip() const { return lastCoachTip_; }

    static constexpr RiftWardenCue CueFor(RiftWardenAttack attack) {
        switch (attack) {
        case RiftWardenAttack::RiftSlash: return RiftWardenCue::Evade;
        case RiftWardenAttack::GravityPulse: return RiftWardenCue::Brace;
        case RiftWardenAttack::EchoBurst: return RiftWardenCue::Evade;
        case RiftWardenAttack::StaggerOpening: return RiftWardenCue::Punish;
        }
        return RiftWardenCue::None;
    }

private:
    static constexpr bool DifficultyValid(RiftWardenDifficulty difficulty) {
        switch (difficulty) {
        case RiftWardenDifficulty::Story:
        case RiftWardenDifficulty::Standard:
        case RiftWardenDifficulty::Expert:
            return true;
        }
        return false;
    }

    static constexpr bool PhaseValid(RiftWardenPhase phase) {
        switch (phase) {
        case RiftWardenPhase::Opening:
        case RiftWardenPhase::Fracture:
        case RiftWardenPhase::Overload:
        case RiftWardenPhase::Defeated:
            return true;
        }
        return false;
    }

    static constexpr bool AttackValid(RiftWardenAttack attack) {
        switch (attack) {
        case RiftWardenAttack::RiftSlash:
        case RiftWardenAttack::GravityPulse:
        case RiftWardenAttack::EchoBurst:
        case RiftWardenAttack::StaggerOpening:
            return true;
        }
        return false;
    }

    static constexpr bool ResponseValid(RiftWardenResponse response) {
        switch (response) {
        case RiftWardenResponse::Dodge:
        case RiftWardenResponse::Guard:
        case RiftWardenResponse::Strike:
            return true;
        }
        return false;
    }

    static constexpr std::size_t DifficultyIndex(RiftWardenDifficulty difficulty) {
        return static_cast<std::size_t>(difficulty);
    }

    static constexpr std::size_t AttackIndex(RiftWardenAttack attack) {
        return static_cast<std::size_t>(attack);
    }

    static constexpr int BossHealthFor(RiftWardenDifficulty difficulty) {
        switch (difficulty) {
        case RiftWardenDifficulty::Story: return 720;
        case RiftWardenDifficulty::Standard: return 900;
        case RiftWardenDifficulty::Expert: return 1080;
        }
        return 0;
    }

    static constexpr int PunishDamage(RiftWardenDifficulty difficulty) {
        switch (difficulty) {
        case RiftWardenDifficulty::Story: return 180;
        case RiftWardenDifficulty::Standard: return 180;
        case RiftWardenDifficulty::Expert: return 180;
        }
        return 0;
    }

    static constexpr int PostureGain(RiftWardenDifficulty difficulty) {
        switch (difficulty) {
        case RiftWardenDifficulty::Story: return 50;
        case RiftWardenDifficulty::Standard: return 40;
        case RiftWardenDifficulty::Expert: return 34;
        }
        return 0;
    }

    static constexpr int IncomingDamage(RiftWardenDifficulty difficulty,
        RiftWardenPhase phase) {
        int base = 10;
        switch (difficulty) {
        case RiftWardenDifficulty::Story: base = 8; break;
        case RiftWardenDifficulty::Standard: base = 14; break;
        case RiftWardenDifficulty::Expert: base = 20; break;
        }
        if (phase == RiftWardenPhase::Fracture) base += 4;
        if (phase == RiftWardenPhase::Overload) base += 8;
        return base;
    }

    static constexpr double DefenseWindow(RiftWardenDifficulty difficulty,
        RiftWardenPhase phase) {
        double seconds = 1.10;
        switch (difficulty) {
        case RiftWardenDifficulty::Story: seconds = 1.20; break;
        case RiftWardenDifficulty::Standard: seconds = 0.90; break;
        case RiftWardenDifficulty::Expert: seconds = 0.65; break;
        }
        if (phase == RiftWardenPhase::Fracture) seconds -= 0.05;
        if (phase == RiftWardenPhase::Overload) seconds -= 0.10;
        return seconds;
    }

    static constexpr RiftWardenAttack PatternFor(RiftWardenPhase phase,
        std::uint32_t cursor) {
        const std::uint32_t index = cursor % 3U;
        if (phase == RiftWardenPhase::Fracture) {
            return index == 0 ? RiftWardenAttack::GravityPulse
                : index == 1 ? RiftWardenAttack::EchoBurst
                             : RiftWardenAttack::RiftSlash;
        }
        if (phase == RiftWardenPhase::Overload) {
            return index == 0 ? RiftWardenAttack::EchoBurst
                : index == 1 ? RiftWardenAttack::RiftSlash
                             : RiftWardenAttack::GravityPulse;
        }
        return index == 0 ? RiftWardenAttack::RiftSlash
            : index == 1 ? RiftWardenAttack::GravityPulse
                         : RiftWardenAttack::EchoBurst;
    }

    static constexpr RiftWardenResponse RecommendedResponse(RiftWardenAttack attack) {
        switch (attack) {
        case RiftWardenAttack::RiftSlash: return RiftWardenResponse::Dodge;
        case RiftWardenAttack::GravityPulse: return RiftWardenResponse::Guard;
        case RiftWardenAttack::EchoBurst: return RiftWardenResponse::Dodge;
        case RiftWardenAttack::StaggerOpening: return RiftWardenResponse::Strike;
        }
        return RiftWardenResponse::Dodge;
    }

    static constexpr int MasteryRank(RiftWardenMastery mastery) {
        switch (mastery) {
        case RiftWardenMastery::Unranked: return 0;
        case RiftWardenMastery::Bronze: return 1;
        case RiftWardenMastery::Silver: return 2;
        case RiftWardenMastery::Gold: return 3;
        }
        return 0;
    }

    static RiftWardenMastery MasteryFor(const RiftWardenRecord& record) {
        if (record.damageTaken == 0 && record.missedOpenings == 0
            && record.perfectDefenses >= 8 && record.clearSeconds <= 180.0) {
            return RiftWardenMastery::Gold;
        }
        if (record.damageTaken <= 100 && record.missedOpenings <= 1
            && record.perfectDefenses >= 4 && record.clearSeconds <= 300.0) {
            return RiftWardenMastery::Silver;
        }
        return RiftWardenMastery::Bronze;
    }

    static bool BetterRecord(const RiftWardenRecord& candidate,
        const RiftWardenRecord& current) {
        if (!current.valid) return true;
        if (MasteryRank(candidate.mastery) != MasteryRank(current.mastery)) {
            return MasteryRank(candidate.mastery) > MasteryRank(current.mastery);
        }
        if (candidate.score != current.score) return candidate.score > current.score;
        if (candidate.missedOpenings != current.missedOpenings) {
            return candidate.missedOpenings < current.missedOpenings;
        }
        if (candidate.missedOpenings >= MaximumMissedOpenings) return false;
        if (candidate.damageTaken != current.damageTaken) {
            return candidate.damageTaken < current.damageTaken;
        }
        if (candidate.damageTaken >= MaximumDamageTaken) return false;
        return candidate.clearSeconds < current.clearSeconds;
    }

    static RiftWardenCoachTip BuildCoachTip(const RiftWardenTelegraph& telegraph,
        RiftWardenCoachReason reason, double reactionSeconds) {
        RiftWardenCoachTip tip{};
        tip.available = true;
        tip.attack = telegraph.attack;
        tip.expectedResponse = telegraph.recommendedResponse;
        tip.cue = telegraph.cue;
        tip.reason = reason;
        tip.responseWindowSeconds = telegraph.responseWindowSeconds;
        tip.observedReactionSeconds = reactionSeconds;
        return tip;
    }

    void RecordTrainingAttempt(RiftWardenAttack attack, RiftWardenPhase phase, bool success) {
        if (!AttackValid(attack) || !PhaseValid(phase)) return;
        const std::size_t index = AttackIndex(attack);
        RiftWardenTrainingRecord& record = trainingRecords_[index];
        record.lastSeenPhase = phase;
        if (record.attempts >= MaximumTrainingAttempts) return;
        ++record.attempts;
        if (success) ++record.successes;
        if (success) {
            currentTrainingStreak_[index] = std::min(
                MaximumTrainingAttempts, currentTrainingStreak_[index] + 1);
            record.bestSuccessStreak = std::max(
                record.bestSuccessStreak, currentTrainingStreak_[index]);
        } else {
            currentTrainingStreak_[index] = 0;
        }
    }

    void ResetAttempt(RiftWardenDifficulty difficulty, bool practice,
        RiftWardenPhase startPhase) {
        difficulty_ = difficulty;
        practice_ = practice;
        active_ = true;
        complete_ = false;
        bossMaxHealth_ = BossHealthFor(difficulty);
        bossHealth_ = bossMaxHealth_;
        if (startPhase == RiftWardenPhase::Fracture) bossHealth_ = bossMaxHealth_ * 69 / 100;
        if (startPhase == RiftWardenPhase::Overload) bossHealth_ = bossMaxHealth_ * 34 / 100;
        phase_ = startPhase;
        posture_ = 0;
        staggerOpen_ = false;
        attackCursor_ = 0;
        elapsedSeconds_ = 0.0;
        damageTaken_ = 0;
        perfectDefenses_ = 0;
        punishes_ = 0;
        missedOpenings_ = 0;
        currentStreak_ = 0;
        bestStreak_ = 0;
        currentTrainingStreak_.fill(0);
        lastCoachTip_ = {};
    }

    void UpdatePhaseFromHealth() {
        if (bossHealth_ <= 0) {
            bossHealth_ = 0;
            phase_ = RiftWardenPhase::Defeated;
            active_ = false;
            complete_ = true;
            RecordCompletion();
            return;
        }
        if (bossHealth_ * 100 <= bossMaxHealth_ * 35) {
            phase_ = RiftWardenPhase::Overload;
        } else if (bossHealth_ * 100 <= bossMaxHealth_ * 70) {
            phase_ = RiftWardenPhase::Fracture;
        } else {
            phase_ = RiftWardenPhase::Opening;
        }
    }

    void RecordCompletion() {
        if (practice_) return;
        hasRankedClear_ = true;
        RiftWardenRecord candidate{};
        candidate.valid = true;
        candidate.damageTaken = damageTaken_;
        candidate.perfectDefenses = perfectDefenses_;
        candidate.punishes = punishes_;
        candidate.missedOpenings = missedOpenings_;
        candidate.bestStreak = bestStreak_;
        candidate.clearSeconds = elapsedSeconds_;
        const int scoredPerfectDefenses = std::min(perfectDefenses_, 30);
        candidate.score = std::max(0, 10000 + scoredPerfectDefenses * 100
            + punishes_ * 250 + bestStreak_ * 25 - damageTaken_ * 20
            - missedOpenings_ * 600 - static_cast<int>(elapsedSeconds_ * 5.0));
        candidate.mastery = MasteryFor(candidate);
        RiftWardenRecord& best = bestRecords_[DifficultyIndex(difficulty_)];
        if (BetterRecord(candidate, best)) best = candidate;

        if (difficulty_ == RiftWardenDifficulty::Story) {
            difficultyUnlocked_[DifficultyIndex(RiftWardenDifficulty::Standard)] = true;
        } else if (difficulty_ == RiftWardenDifficulty::Standard) {
            difficultyUnlocked_[DifficultyIndex(RiftWardenDifficulty::Expert)] = true;
        }
    }

    std::array<bool, 3> difficultyUnlocked_{{true, false, false}};
    std::array<RiftWardenRecord, 3> bestRecords_{};
    std::array<RiftWardenTrainingRecord, 4> trainingRecords_{};
    std::array<int, 4> currentTrainingStreak_{};
    RiftWardenCoachTip lastCoachTip_{};
    RiftWardenDifficulty difficulty_{RiftWardenDifficulty::Story};
    RiftWardenPhase phase_{RiftWardenPhase::Opening};
    int bossHealth_{};
    int bossMaxHealth_{};
    int posture_{};
    bool staggerOpen_{};
    std::uint32_t attackCursor_{};
    double elapsedSeconds_{};
    int damageTaken_{};
    int perfectDefenses_{};
    int punishes_{};
    int missedOpenings_{};
    int currentStreak_{};
    int bestStreak_{};
    bool active_{};
    bool complete_{};
    bool practice_{};
    bool hasRankedClear_{};
};

} // namespace Astral::Scene
