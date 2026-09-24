#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace Astral::Scene {

enum class ShadowCryptEnemyRole : std::uint8_t {
    RiftSkirmisher,
    VeilChanneler,
    GraveboundBulwark,
    Count,
};

enum class ShadowCryptThreatCue : std::uint8_t {
    None,
    Evade,
    Interrupt,
    Brace,
};

enum class ShadowCryptDefenseResponse : std::uint8_t {
    None,
    Dodge,
    Interrupt,
    Guard,
};

enum class ShadowCryptAttackStyle : std::uint8_t {
    Light,
    Heavy,
    Counter,
};

enum class ShadowCryptSkirmishTier : std::uint8_t {
    EntrySeal,
    ArchiveGallery,
    RiftNave,
    Count,
};

enum class ShadowCryptThreatVariant : std::uint8_t {
    RiftLunge,
    RiftCrosscut,
    VeilBind,
    VeilBurst,
    GraveHammer,
    GraveRush,
    Count,
};

struct ShadowCryptEnemyState {
    ShadowCryptEnemyRole role{ShadowCryptEnemyRole::RiftSkirmisher};
    int health{};
    int maxHealth{};
    int posture{};
    int maxPosture{};
    bool staggered{};
    bool defeated{};
};

struct ShadowCryptThreatTelegraph {
    bool valid{};
    std::size_t enemyIndex{};
    ShadowCryptEnemyRole role{ShadowCryptEnemyRole::RiftSkirmisher};
    ShadowCryptThreatVariant variant{ShadowCryptThreatVariant::RiftLunge};
    ShadowCryptThreatCue cue{ShadowCryptThreatCue::None};
    ShadowCryptDefenseResponse expected{ShadowCryptDefenseResponse::None};
    double responseWindowSeconds{};
    int failureDamage{};
    bool defensiveCounterAllowed{};
    bool enraged{};
    std::uint8_t patternStep{};
};

struct ShadowCryptDefenseReport {
    bool accepted{};
    bool successful{};
    bool precision{};
    bool openedCounter{};
    bool interruptedChannel{};
    bool staggerOpened{};
    bool breachOpened{};
    int postureDamageApplied{};
    int damageTaken{};
    std::size_t enemyIndex{};
    std::uint8_t shadowFlow{};
};

struct ShadowCryptAttackReport {
    bool accepted{};
    std::size_t enemyIndex{};
    int healthDamage{};
    int postureDamage{};
    bool staggerOpened{};
    bool staggerConsumed{};
    bool defeated{};
    bool encounterComplete{};
    bool breachConsumed{};
    bool weaknessExploited{};
    bool flowBreakConsumed{};
};

struct ShadowCryptTargetRecommendation {
    bool valid{};
    std::size_t enemyIndex{};
    ShadowCryptEnemyRole role{ShadowCryptEnemyRole::RiftSkirmisher};
    int priority{};
};

struct ShadowCryptThreatQueue {
    static constexpr std::size_t Capacity = 3;
    std::array<ShadowCryptThreatTelegraph, Capacity> threats{};
    std::size_t count{};
};

struct ShadowCryptRoleIntel {
    bool valid{};
    std::size_t enemyIndex{};
    ShadowCryptEnemyRole role{ShadowCryptEnemyRole::RiftSkirmisher};
    ShadowCryptAttackStyle recommendedAttack{ShadowCryptAttackStyle::Heavy};
    bool breachOpening{};
    bool enraged{};
};

// Game-domain Shadow Crypt room combat. This models original enemy roles and
// readable counterplay without taking renderer, input, animation, audio, or
// generic engine ownership.
class ShadowCryptSkirmish {
public:
    static constexpr std::size_t EnemyCapacity = 3;
    static constexpr std::uint8_t MaxShadowFlow = 3;

    bool Begin(ShadowCryptSkirmishTier tier) {
        if (active_ || !ValidTier(tier)) return false;
        Reset();
        tier_ = tier;
        active_ = true;
        switch (tier) {
        case ShadowCryptSkirmishTier::EntrySeal:
            enemyCount_ = 2;
            enemies_[0] = MakeEnemy(ShadowCryptEnemyRole::RiftSkirmisher, 80, 40);
            enemies_[1] = MakeEnemy(ShadowCryptEnemyRole::GraveboundBulwark, 120, 70);
            break;
        case ShadowCryptSkirmishTier::ArchiveGallery:
            enemyCount_ = 2;
            enemies_[0] = MakeEnemy(ShadowCryptEnemyRole::VeilChanneler, 90, 45);
            enemies_[1] = MakeEnemy(ShadowCryptEnemyRole::RiftSkirmisher, 95, 45);
            break;
        case ShadowCryptSkirmishTier::RiftNave:
            enemyCount_ = 3;
            enemies_[0] = MakeEnemy(ShadowCryptEnemyRole::VeilChanneler, 110, 55);
            enemies_[1] = MakeEnemy(ShadowCryptEnemyRole::GraveboundBulwark, 150, 85);
            enemies_[2] = MakeEnemy(ShadowCryptEnemyRole::RiftSkirmisher, 110, 50);
            break;
        case ShadowCryptSkirmishTier::Count:
            return false;
        }
        RefreshThreat();
        return true;
    }

    bool Replay() {
        if (active_ || !complete_ || !ValidTier(tier_)) return false;
        const ShadowCryptSkirmishTier replayTier = tier_;
        if (!Begin(replayTier)) return false;
        replayMode_ = true;
        return true;
    }

    bool Cancel() {
        if (!active_) return false;
        Reset();
        return true;
    }

    bool Active() const { return active_; }
    bool Complete() const { return complete_; }
    bool ReplayMode() const { return replayMode_; }
    ShadowCryptSkirmishTier Tier() const { return tier_; }
    std::size_t EnemyCount() const { return enemyCount_; }
    std::uint8_t ShadowFlow() const { return shadowFlow_; }
    bool ShadowFlowReady() const { return shadowFlow_ == MaxShadowFlow; }

    ShadowCryptEnemyState Enemy(std::size_t index) const {
        return index < enemyCount_ ? enemies_[index] : ShadowCryptEnemyState{};
    }

    ShadowCryptRoleIntel EnemyIntel(std::size_t index) const {
        ShadowCryptRoleIntel intel{};
        if (index >= enemyCount_) return intel;
        intel.valid = true;
        intel.enemyIndex = index;
        intel.role = enemies_[index].role;
        intel.recommendedAttack = RecommendedAttack(enemies_[index].role);
        intel.breachOpening = breachOpenings_[index] && !enemies_[index].defeated;
        intel.enraged = enemies_[index].health > 0
            && enemies_[index].health * 2 <= enemies_[index].maxHealth;
        return intel;
    }

    ShadowCryptThreatTelegraph CurrentThreat() const {
        if (!active_ || complete_ || threatIndex_ >= enemyCount_
            || enemies_[threatIndex_].defeated) return {};
        return ThreatForIndex(threatIndex_);
    }

    ShadowCryptThreatQueue ThreatQueue(
        std::size_t maxCount = ShadowCryptThreatQueue::Capacity) const {
        ShadowCryptThreatQueue queue{};
        if (!active_ || complete_ || enemyCount_ == 0 || maxCount == 0) return queue;
        const std::size_t wanted = std::min(maxCount, ShadowCryptThreatQueue::Capacity);
        for (std::size_t offset = 0; offset < enemyCount_ && queue.count < wanted; ++offset) {
            const std::size_t index = (threatIndex_ + offset) % enemyCount_;
            if (enemies_[index].defeated) continue;
            const ShadowCryptThreatTelegraph threat = ThreatForIndex(index);
            if (threat.valid) queue.threats[queue.count++] = threat;
        }
        return queue;
    }

    ShadowCryptDefenseReport ResolveThreat(ShadowCryptDefenseResponse response,
        double reactionSeconds) {
        ShadowCryptDefenseReport report{};
        const auto threat = CurrentThreat();
        if (!threat.valid || !ValidResponse(response)
            || !std::isfinite(reactionSeconds) || reactionSeconds < 0.0) return report;
        report.accepted = true;
        report.enemyIndex = threat.enemyIndex;
        report.successful = response == threat.expected
            && reactionSeconds <= threat.responseWindowSeconds;
        if (report.successful) {
            report.precision = reactionSeconds
                <= threat.responseWindowSeconds * PrecisionResponseFraction;
            report.openedCounter = threat.defensiveCounterAllowed;
            counterTarget_ = report.openedCounter ? threat.enemyIndex : EnemyCapacity;
            precisionCounterTarget_ =
                report.openedCounter && report.precision ? threat.enemyIndex : EnemyCapacity;
            report.interruptedChannel =
                threat.role == ShadowCryptEnemyRole::VeilChanneler
                && response == ShadowCryptDefenseResponse::Interrupt;
            breachOpenings_[threat.enemyIndex] = true;
            report.breachOpened = true;
            shadowFlow_ = std::min<std::uint8_t>(MaxShadowFlow,
                static_cast<std::uint8_t>(shadowFlow_ + 1));
            if (threat.role == ShadowCryptEnemyRole::GraveboundBulwark
                && response == ShadowCryptDefenseResponse::Guard) {
                auto& enemy = enemies_[threat.enemyIndex];
                if (!enemy.staggered && !enemy.defeated) {
                    const int postureBefore = enemy.posture;
                    enemy.posture = std::max(0, enemy.posture - BracePostureDamage);
                    report.postureDamageApplied = postureBefore - enemy.posture;
                    if (enemy.posture == 0) {
                        enemy.staggered = true;
                        report.staggerOpened = true;
                    }
                }
            }
        } else {
            const int damageBefore = damageTaken_;
            damageTaken_ = std::min(MaxTrackedDamage, damageTaken_ + threat.failureDamage);
            report.damageTaken = damageTaken_ - damageBefore;
            counterTarget_ = EnemyCapacity;
            precisionCounterTarget_ = EnemyCapacity;
            breachOpenings_[threat.enemyIndex] = false;
            shadowFlow_ = 0;
        }
        report.shadowFlow = shadowFlow_;
        AdvanceThreatPattern(threat.enemyIndex);
        AdvanceThreatCursor();
        return report;
    }

    ShadowCryptAttackReport AttackTarget(std::size_t index, ShadowCryptAttackStyle style) {
        ShadowCryptAttackReport report{};
        if (!active_ || complete_ || index >= enemyCount_ || !ValidAttackStyle(style)
            || enemies_[index].defeated) return report;
        auto& enemy = enemies_[index];
        report.accepted = true;
        report.enemyIndex = index;

        int healthDamage = 0;
        int postureDamage = 0;
        switch (style) {
        case ShadowCryptAttackStyle::Light:
            healthDamage = enemy.role == ShadowCryptEnemyRole::GraveboundBulwark
                && !enemy.staggered ? 8 : 20;
            postureDamage = 12;
            break;
        case ShadowCryptAttackStyle::Heavy:
            healthDamage = enemy.role == ShadowCryptEnemyRole::GraveboundBulwark
                && !enemy.staggered ? 14 : 28;
            postureDamage = 28;
            break;
        case ShadowCryptAttackStyle::Counter:
            if (counterTarget_ != index) {
                report.accepted = false;
                return report;
            }
            healthDamage = 36;
            postureDamage = 24;
            if (precisionCounterTarget_ == index) {
                healthDamage += PrecisionCounterHealthBonus;
                postureDamage += PrecisionCounterPostureBonus;
            }
            counterTarget_ = EnemyCapacity;
            precisionCounterTarget_ = EnemyCapacity;
            break;
        }

        if (breachOpenings_[index]) {
            report.breachConsumed = true;
            breachOpenings_[index] = false;
            if (style == RecommendedAttack(enemy.role)) {
                report.weaknessExploited = true;
                healthDamage += WeaknessHealthBonus;
                postureDamage += WeaknessPostureBonus;
            }
        }
        if (style == ShadowCryptAttackStyle::Heavy && ShadowFlowReady()) {
            report.flowBreakConsumed = true;
            shadowFlow_ = 0;
            healthDamage += FlowBreakHealthBonus;
            postureDamage += FlowBreakPostureBonus;
        }

        if (enemy.staggered) {
            healthDamage += 20;
            enemy.staggered = false;
            enemy.posture = enemy.maxPosture;
            report.staggerConsumed = true;
        } else {
            const int postureBefore = enemy.posture;
            enemy.posture = std::max(0, enemy.posture - postureDamage);
            report.postureDamage = postureBefore - enemy.posture;
            if (enemy.posture == 0) {
                enemy.staggered = true;
                report.staggerOpened = true;
            }
        }

        const int beforeHealth = enemy.health;
        enemy.health = std::max(0, enemy.health - healthDamage);
        report.healthDamage = beforeHealth - enemy.health;
        if (enemy.health == 0) {
            enemy.defeated = true;
            enemy.staggered = false;
            report.staggerOpened = false;
            report.defeated = true;
            breachOpenings_[index] = false;
            if (lockedTarget_ == index) lockedTarget_ = EnemyCapacity;
            if (counterTarget_ == index) counterTarget_ = EnemyCapacity;
            if (precisionCounterTarget_ == index) precisionCounterTarget_ = EnemyCapacity;
        }
        complete_ = AllDefeated();
        if (complete_) {
            active_ = false;
        } else if (threatIndex_ >= enemyCount_ || enemies_[threatIndex_].defeated) {
            RefreshThreat();
        }
        report.encounterComplete = complete_;
        return report;
    }

    ShadowCryptTargetRecommendation RecommendedTarget() const {
        ShadowCryptTargetRecommendation best{};
        for (std::size_t i = 0; i < enemyCount_; ++i) {
            if (enemies_[i].defeated) continue;
            int priority = BasePriority(enemies_[i].role);
            if (enemies_[i].staggered) priority += 100;
            if (counterTarget_ == i) priority += 80;
            if (threatIndex_ == i && enemies_[i].role == ShadowCryptEnemyRole::VeilChanneler)
                priority += 40;
            if (!best.valid || priority > best.priority
                || (priority == best.priority && i < best.enemyIndex)) {
                best.valid = true;
                best.enemyIndex = i;
                best.role = enemies_[i].role;
                best.priority = priority;
            }
        }
        return best;
    }

    bool LockTarget(std::size_t index) {
        if (!active_ || complete_ || index >= enemyCount_ || enemies_[index].defeated
            || lockedTarget_ == index) return false;
        lockedTarget_ = index;
        return true;
    }

    bool ClearTargetLock() {
        if (lockedTarget_ >= enemyCount_) return false;
        lockedTarget_ = EnemyCapacity;
        return true;
    }

    bool HasTargetLock() const {
        return lockedTarget_ < enemyCount_ && !enemies_[lockedTarget_].defeated;
    }

    std::size_t SelectedTarget() const {
        if (HasTargetLock()) return lockedTarget_;
        const auto recommendation = RecommendedTarget();
        return recommendation.valid ? recommendation.enemyIndex : EnemyCapacity;
    }

    int DamageTaken() const { return damageTaken_; }

private:
    static constexpr int MaxTrackedDamage = 9999;
    static constexpr int BracePostureDamage = 10;
    static constexpr int PrecisionCounterHealthBonus = 8;
    static constexpr int PrecisionCounterPostureBonus = 6;
    static constexpr int WeaknessHealthBonus = 10;
    static constexpr int WeaknessPostureBonus = 8;
    static constexpr int FlowBreakHealthBonus = 12;
    static constexpr int FlowBreakPostureBonus = 10;
    static constexpr double PrecisionResponseFraction = 0.30;

    static constexpr bool ValidTier(ShadowCryptSkirmishTier tier) {
        return static_cast<std::uint8_t>(tier)
            < static_cast<std::uint8_t>(ShadowCryptSkirmishTier::Count);
    }
    static constexpr bool ValidResponse(ShadowCryptDefenseResponse response) {
        return static_cast<std::uint8_t>(response)
            <= static_cast<std::uint8_t>(ShadowCryptDefenseResponse::Guard);
    }
    static constexpr bool ValidAttackStyle(ShadowCryptAttackStyle style) {
        return static_cast<std::uint8_t>(style)
            <= static_cast<std::uint8_t>(ShadowCryptAttackStyle::Counter);
    }
    static constexpr ShadowCryptEnemyState MakeEnemy(ShadowCryptEnemyRole role,
        int health, int posture) {
        return {role, health, health, posture, posture, false, false};
    }
    static constexpr int BasePriority(ShadowCryptEnemyRole role) {
        switch (role) {
        case ShadowCryptEnemyRole::VeilChanneler: return 30;
        case ShadowCryptEnemyRole::RiftSkirmisher: return 20;
        case ShadowCryptEnemyRole::GraveboundBulwark: return 10;
        case ShadowCryptEnemyRole::Count: return 0;
        }
        return 0;
    }
    static constexpr ShadowCryptAttackStyle RecommendedAttack(ShadowCryptEnemyRole role) {
        switch (role) {
        case ShadowCryptEnemyRole::VeilChanneler: return ShadowCryptAttackStyle::Light;
        case ShadowCryptEnemyRole::RiftSkirmisher: return ShadowCryptAttackStyle::Heavy;
        case ShadowCryptEnemyRole::GraveboundBulwark: return ShadowCryptAttackStyle::Heavy;
        case ShadowCryptEnemyRole::Count: return ShadowCryptAttackStyle::Heavy;
        }
        return ShadowCryptAttackStyle::Heavy;
    }

    ShadowCryptThreatTelegraph ThreatForIndex(std::size_t index) const {
        if (!active_ || complete_ || index >= enemyCount_ || enemies_[index].defeated) return {};
        const auto role = enemies_[index].role;
        const std::uint8_t patternStep = static_cast<std::uint8_t>(patternSteps_[index] % 2);
        ShadowCryptThreatTelegraph telegraph{};
        telegraph.valid = true;
        telegraph.enemyIndex = index;
        telegraph.role = role;
        telegraph.patternStep = patternStep;
        telegraph.enraged = enemies_[index].health > 0
            && enemies_[index].health * 2 <= enemies_[index].maxHealth;
        switch (role) {
        case ShadowCryptEnemyRole::RiftSkirmisher:
            if (patternStep == 0) {
                telegraph.variant = ShadowCryptThreatVariant::RiftLunge;
                telegraph.cue = ShadowCryptThreatCue::Evade;
                telegraph.expected = ShadowCryptDefenseResponse::Dodge;
                telegraph.responseWindowSeconds = 0.42;
                telegraph.failureDamage = 18;
                telegraph.defensiveCounterAllowed = true;
            } else {
                telegraph.variant = ShadowCryptThreatVariant::RiftCrosscut;
                telegraph.cue = ShadowCryptThreatCue::Evade;
                telegraph.expected = ShadowCryptDefenseResponse::Dodge;
                telegraph.responseWindowSeconds = 0.34;
                telegraph.failureDamage = 22;
                telegraph.defensiveCounterAllowed = true;
            }
            break;
        case ShadowCryptEnemyRole::VeilChanneler:
            if (patternStep == 0) {
                telegraph.variant = ShadowCryptThreatVariant::VeilBind;
                telegraph.cue = ShadowCryptThreatCue::Interrupt;
                telegraph.expected = ShadowCryptDefenseResponse::Interrupt;
                telegraph.responseWindowSeconds = 0.70;
                telegraph.failureDamage = 24;
                telegraph.defensiveCounterAllowed = true;
            } else {
                telegraph.variant = ShadowCryptThreatVariant::VeilBurst;
                telegraph.cue = ShadowCryptThreatCue::Interrupt;
                telegraph.expected = ShadowCryptDefenseResponse::Interrupt;
                telegraph.responseWindowSeconds = 0.52;
                telegraph.failureDamage = 28;
                telegraph.defensiveCounterAllowed = true;
            }
            break;
        case ShadowCryptEnemyRole::GraveboundBulwark:
            if (patternStep == 0) {
                telegraph.variant = ShadowCryptThreatVariant::GraveHammer;
                telegraph.cue = ShadowCryptThreatCue::Brace;
                telegraph.expected = ShadowCryptDefenseResponse::Guard;
                telegraph.responseWindowSeconds = 0.55;
                telegraph.failureDamage = 30;
                telegraph.defensiveCounterAllowed = false;
            } else {
                telegraph.variant = ShadowCryptThreatVariant::GraveRush;
                telegraph.cue = ShadowCryptThreatCue::Brace;
                telegraph.expected = ShadowCryptDefenseResponse::Guard;
                telegraph.responseWindowSeconds = 0.42;
                telegraph.failureDamage = 34;
                telegraph.defensiveCounterAllowed = false;
            }
            break;
        case ShadowCryptEnemyRole::Count:
            return {};
        }
        if (telegraph.enraged) {
            telegraph.responseWindowSeconds =
                std::max(0.20, telegraph.responseWindowSeconds * 0.80);
            telegraph.failureDamage = std::min(40, telegraph.failureDamage + 6);
        }
        return telegraph;
    }

    void Reset() {
        enemies_ = {};
        patternSteps_ = {};
        breachOpenings_ = {};
        enemyCount_ = 0;
        threatIndex_ = 0;
        lockedTarget_ = EnemyCapacity;
        counterTarget_ = EnemyCapacity;
        precisionCounterTarget_ = EnemyCapacity;
        damageTaken_ = 0;
        shadowFlow_ = 0;
        replayMode_ = false;
        complete_ = false;
        active_ = false;
    }

    bool AllDefeated() const {
        if (enemyCount_ == 0) return false;
        for (std::size_t i = 0; i < enemyCount_; ++i) {
            if (!enemies_[i].defeated) return false;
        }
        return true;
    }

    void RefreshThreat() {
        if (!active_ || complete_ || enemyCount_ == 0) return;
        for (std::size_t offset = 0; offset < enemyCount_; ++offset) {
            const std::size_t index = (threatIndex_ + offset) % enemyCount_;
            if (!enemies_[index].defeated) {
                threatIndex_ = index;
                return;
            }
        }
    }

    void AdvanceThreatPattern(std::size_t index) {
        if (index >= enemyCount_) return;
        patternSteps_[index] = static_cast<std::uint8_t>((patternSteps_[index] + 1) % 2);
    }

    void AdvanceThreatCursor() {
        if (enemyCount_ == 0) return;
        threatIndex_ = (threatIndex_ + 1) % enemyCount_;
        RefreshThreat();
    }

    std::array<ShadowCryptEnemyState, EnemyCapacity> enemies_{};
    std::array<std::uint8_t, EnemyCapacity> patternSteps_{};
    std::array<bool, EnemyCapacity> breachOpenings_{};
    std::size_t enemyCount_{};
    std::size_t threatIndex_{};
    std::size_t lockedTarget_{EnemyCapacity};
    std::size_t counterTarget_{EnemyCapacity};
    std::size_t precisionCounterTarget_{EnemyCapacity};
    int damageTaken_{};
    std::uint8_t shadowFlow_{};
    ShadowCryptSkirmishTier tier_{ShadowCryptSkirmishTier::EntrySeal};
    bool replayMode_{};
    bool active_{};
    bool complete_{};
};

} // namespace Astral::Scene
