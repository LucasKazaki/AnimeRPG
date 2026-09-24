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
    ShadowCryptThreatCue cue{ShadowCryptThreatCue::None};
    ShadowCryptDefenseResponse expected{ShadowCryptDefenseResponse::None};
    double responseWindowSeconds{};
    int failureDamage{};
    bool defensiveCounterAllowed{};
};

struct ShadowCryptDefenseReport {
    bool accepted{};
    bool successful{};
    bool openedCounter{};
    bool interruptedChannel{};
    int damageTaken{};
    std::size_t enemyIndex{};
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
};

struct ShadowCryptTargetRecommendation {
    bool valid{};
    std::size_t enemyIndex{};
    ShadowCryptEnemyRole role{ShadowCryptEnemyRole::RiftSkirmisher};
    int priority{};
};

// Game-domain Shadow Crypt room combat. This models original enemy roles and
// readable counterplay without taking renderer, input, animation, audio, or
// generic engine ownership.
class ShadowCryptSkirmish {
public:
    static constexpr std::size_t EnemyCapacity = 3;

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

    bool Cancel() {
        if (!active_) return false;
        Reset();
        return true;
    }

    bool Active() const { return active_; }
    bool Complete() const { return complete_; }
    ShadowCryptSkirmishTier Tier() const { return tier_; }
    std::size_t EnemyCount() const { return enemyCount_; }

    ShadowCryptEnemyState Enemy(std::size_t index) const {
        return index < enemyCount_ ? enemies_[index] : ShadowCryptEnemyState{};
    }

    ShadowCryptThreatTelegraph CurrentThreat() const {
        if (!active_ || complete_ || threatIndex_ >= enemyCount_
            || enemies_[threatIndex_].defeated) return {};
        const auto role = enemies_[threatIndex_].role;
        ShadowCryptThreatTelegraph telegraph{};
        telegraph.valid = true;
        telegraph.enemyIndex = threatIndex_;
        telegraph.role = role;
        switch (role) {
        case ShadowCryptEnemyRole::RiftSkirmisher:
            telegraph.cue = ShadowCryptThreatCue::Evade;
            telegraph.expected = ShadowCryptDefenseResponse::Dodge;
            telegraph.responseWindowSeconds = 0.42;
            telegraph.failureDamage = 18;
            telegraph.defensiveCounterAllowed = true;
            break;
        case ShadowCryptEnemyRole::VeilChanneler:
            telegraph.cue = ShadowCryptThreatCue::Interrupt;
            telegraph.expected = ShadowCryptDefenseResponse::Interrupt;
            telegraph.responseWindowSeconds = 0.70;
            telegraph.failureDamage = 24;
            telegraph.defensiveCounterAllowed = true;
            break;
        case ShadowCryptEnemyRole::GraveboundBulwark:
            telegraph.cue = ShadowCryptThreatCue::Brace;
            telegraph.expected = ShadowCryptDefenseResponse::Guard;
            telegraph.responseWindowSeconds = 0.55;
            telegraph.failureDamage = 30;
            telegraph.defensiveCounterAllowed = false;
            break;
        case ShadowCryptEnemyRole::Count:
            return {};
        }
        return telegraph;
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
            report.openedCounter = threat.defensiveCounterAllowed;
            counterTarget_ = report.openedCounter ? threat.enemyIndex : EnemyCapacity;
            report.interruptedChannel =
                threat.role == ShadowCryptEnemyRole::VeilChanneler
                && response == ShadowCryptDefenseResponse::Interrupt;
        } else {
            const int damageBefore = damageTaken_;
            damageTaken_ = std::min(MaxTrackedDamage, damageTaken_ + threat.failureDamage);
            report.damageTaken = damageTaken_ - damageBefore;
            counterTarget_ = EnemyCapacity;
        }
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
            counterTarget_ = EnemyCapacity;
            break;
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
            if (lockedTarget_ == index) lockedTarget_ = EnemyCapacity;
            if (counterTarget_ == index) counterTarget_ = EnemyCapacity;
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

    void Reset() {
        enemies_ = {};
        enemyCount_ = 0;
        threatIndex_ = 0;
        lockedTarget_ = EnemyCapacity;
        counterTarget_ = EnemyCapacity;
        damageTaken_ = 0;
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

    void AdvanceThreatCursor() {
        if (enemyCount_ == 0) return;
        threatIndex_ = (threatIndex_ + 1) % enemyCount_;
        RefreshThreat();
    }

    std::array<ShadowCryptEnemyState, EnemyCapacity> enemies_{};
    std::size_t enemyCount_{};
    std::size_t threatIndex_{};
    std::size_t lockedTarget_{EnemyCapacity};
    std::size_t counterTarget_{EnemyCapacity};
    int damageTaken_{};
    ShadowCryptSkirmishTier tier_{ShadowCryptSkirmishTier::EntrySeal};
    bool active_{};
    bool complete_{};
};

} // namespace Astral::Scene
