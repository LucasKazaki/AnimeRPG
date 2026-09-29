#pragma once

// Behaviour trees with a typed blackboard (the role of UE Behavior Trees):
// Sequence/Selector (with running-child memory), Parallel, Inverter,
// Succeeder, Repeat, Cooldown, TimeLimit, Wait, Condition and Action leaves.
// Deterministic, allocation-free while ticking, and resettable so encounters
// can restart cleanly. Also: sight-cone perception with physics line of sight.

#include "Engine/Math/VectorMath.h"

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Astral::Physics {
class PhysicsWorld;
}

namespace Astral::AI {

enum class Status : std::uint8_t { Success, Failure, Running };

class Blackboard {
public:
    void SetFloat(const std::string& key, float value) { floats_[key] = value; }
    void SetBool(const std::string& key, bool value) { bools_[key] = value; }
    void SetVector(const std::string& key, Math::Vec3 value) { vectors_[key] = value; }
    float GetFloat(const std::string& key, float fallback = 0.0f) const;
    bool GetBool(const std::string& key, bool fallback = false) const;
    Math::Vec3 GetVector(const std::string& key, Math::Vec3 fallback = {}) const;
    bool HasVector(const std::string& key) const { return vectors_.count(key) != 0; }
    void Clear();

private:
    std::map<std::string, float> floats_;
    std::map<std::string, bool> bools_;
    std::map<std::string, Math::Vec3> vectors_;
};

class Node {
public:
    virtual ~Node() = default;
    virtual Status Tick(Blackboard& blackboard, float dt) = 0;
    // Full reset for encounter restarts (clears timers such as cooldowns).
    virtual void Reset() {}
    // Cancels in-flight work when a parent aborts this branch; timers that must
    // outlive an abort (cooldowns) are kept.
    virtual void Abort() { Reset(); }
    std::string name;
};

using NodePtr = std::unique_ptr<Node>;

class Composite : public Node {
public:
    Composite& Add(NodePtr child) {
        children_.push_back(std::move(child));
        return *this;
    }
    void Reset() override;
    void Abort() override;

protected:
    std::vector<NodePtr> children_;
    std::size_t running_{};
};

// Runs children in order until one fails; resumes a running child.
class Sequence final : public Composite {
public:
    Status Tick(Blackboard& blackboard, float dt) override;
};

// Reactive priority selector: re-evaluates from the first child every tick and
// aborts (resets) a running lower-priority child when a higher one takes over,
// like UE's "abort lower priority" observers.
class Selector final : public Composite {
public:
    Status Tick(Blackboard& blackboard, float dt) override;
    void Reset() override;
    void Abort() override;

private:
    int active_{-1};
};

// Ticks all children; succeeds when `successThreshold` succeed, fails when success becomes impossible.
class Parallel final : public Composite {
public:
    explicit Parallel(int successThreshold) : threshold_(successThreshold) {}
    Status Tick(Blackboard& blackboard, float dt) override;

private:
    int threshold_;
};

class Decorator : public Node {
public:
    explicit Decorator(NodePtr child) : child_(std::move(child)) {}
    void Reset() override {
        if (child_) child_->Reset();
    }
    void Abort() override {
        if (child_) child_->Abort();
    }

protected:
    NodePtr child_;
};

class Inverter final : public Decorator {
public:
    using Decorator::Decorator;
    Status Tick(Blackboard& blackboard, float dt) override;
};

class Succeeder final : public Decorator {
public:
    using Decorator::Decorator;
    Status Tick(Blackboard& blackboard, float dt) override;
};

// Repeats the child `count` times (count < 0 = forever); fails if the child fails.
class Repeat final : public Decorator {
public:
    Repeat(NodePtr child, int count) : Decorator(std::move(child)), count_(count) {}
    Status Tick(Blackboard& blackboard, float dt) override;
    void Reset() override;
    void Abort() override;

private:
    int count_;
    int done_{};
};

// Fails while cooling down after the child finished (success or failure).
class Cooldown final : public Decorator {
public:
    Cooldown(NodePtr child, float seconds) : Decorator(std::move(child)), seconds_(seconds) {}
    Status Tick(Blackboard& blackboard, float dt) override;
    void Reset() override;
    void Abort() override;

private:
    float seconds_;
    float remaining_{};
    bool childRunning_{};
};

// Fails the child if it keeps running longer than `seconds`.
class TimeLimit final : public Decorator {
public:
    TimeLimit(NodePtr child, float seconds) : Decorator(std::move(child)), seconds_(seconds) {}
    Status Tick(Blackboard& blackboard, float dt) override;
    void Reset() override;

private:
    float seconds_;
    float elapsed_{};
};

class Wait final : public Node {
public:
    explicit Wait(float seconds) : seconds_(seconds) {}
    Status Tick(Blackboard& blackboard, float dt) override;
    void Reset() override { elapsed_ = 0.0f; }

private:
    float seconds_;
    float elapsed_{};
};

class Condition final : public Node {
public:
    explicit Condition(std::function<bool(const Blackboard&)> predicate) : predicate_(std::move(predicate)) {}
    Status Tick(Blackboard& blackboard, float) override {
        return predicate_ && predicate_(blackboard) ? Status::Success : Status::Failure;
    }

private:
    std::function<bool(const Blackboard&)> predicate_;
};

class Action final : public Node {
public:
    explicit Action(std::function<Status(Blackboard&, float)> action) : action_(std::move(action)) {}
    Status Tick(Blackboard& blackboard, float dt) override { return action_ ? action_(blackboard, dt) : Status::Failure; }

private:
    std::function<Status(Blackboard&, float)> action_;
};

class BehaviorTree {
public:
    explicit BehaviorTree(NodePtr root) : root_(std::move(root)) {}
    Status Tick(Blackboard& blackboard, float dt);
    void Reset();

private:
    NodePtr root_;
    Status last_{Status::Success};
};

// Builder helpers.
template <typename T, typename... Args>
NodePtr Make(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}
template <typename... Children>
NodePtr MakeSequence(Children&&... children) {
    auto node = std::make_unique<Sequence>();
    node->name = "Sequence";
    (node->Add(std::forward<Children>(children)), ...);
    return node;
}
template <typename... Children>
NodePtr MakeSelector(Children&&... children) {
    auto node = std::make_unique<Selector>();
    node->name = "Selector";
    (node->Add(std::forward<Children>(children)), ...);
    return node;
}

// True when `target` is inside the eye's view cone and range and no static
// collider blocks the line (dynamic bodies and the observer's own body ignored).
bool CanSee(const Physics::PhysicsWorld& world, Math::Vec3 eye, Math::Vec3 forward, Math::Vec3 target,
    float fovDegrees, float range, std::uint32_t occluderMask = 0xFFFFFFFFu);

} // namespace Astral::AI
