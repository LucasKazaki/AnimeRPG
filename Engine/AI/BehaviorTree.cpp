#include "Engine/AI/BehaviorTree.h"

#include "Engine/Physics/PhysicsWorld.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace Astral::AI {

float Blackboard::GetFloat(const std::string& key, float fallback) const {
    const auto it = floats_.find(key);
    return it == floats_.end() ? fallback : it->second;
}
bool Blackboard::GetBool(const std::string& key, bool fallback) const {
    const auto it = bools_.find(key);
    return it == bools_.end() ? fallback : it->second;
}
Math::Vec3 Blackboard::GetVector(const std::string& key, Math::Vec3 fallback) const {
    const auto it = vectors_.find(key);
    return it == vectors_.end() ? fallback : it->second;
}
void Blackboard::Clear() {
    floats_.clear();
    bools_.clear();
    vectors_.clear();
}

void Composite::Reset() {
    running_ = 0;
    for (NodePtr& child : children_) child->Reset();
}

void Composite::Abort() {
    running_ = 0;
    for (NodePtr& child : children_) child->Abort();
}

Status Sequence::Tick(Blackboard& blackboard, float dt) {
    for (std::size_t i = running_; i < children_.size(); ++i) {
        const Status status = children_[i]->Tick(blackboard, dt);
        if (status == Status::Running) {
            running_ = i;
            return Status::Running;
        }
        if (status == Status::Failure) {
            Abort();
            return Status::Failure;
        }
    }
    Abort();
    return Status::Success;
}

Status Selector::Tick(Blackboard& blackboard, float dt) {
    for (std::size_t i = 0; i < children_.size(); ++i) {
        const Status status = children_[i]->Tick(blackboard, dt);
        if (status == Status::Failure) continue;
        if (active_ >= 0 && static_cast<std::size_t>(active_) != i) children_[static_cast<std::size_t>(active_)]->Abort();
        active_ = status == Status::Running ? static_cast<int>(i) : -1;
        return status;
    }
    if (active_ >= 0) children_[static_cast<std::size_t>(active_)]->Abort();
    active_ = -1;
    return Status::Failure;
}

void Selector::Reset() {
    active_ = -1;
    Composite::Reset();
}

void Selector::Abort() {
    active_ = -1;
    Composite::Abort();
}

Status Parallel::Tick(Blackboard& blackboard, float dt) {
    int successes = 0, failures = 0;
    for (NodePtr& child : children_) {
        const Status status = child->Tick(blackboard, dt);
        successes += status == Status::Success;
        failures += status == Status::Failure;
    }
    const int total = static_cast<int>(children_.size());
    if (successes >= threshold_) {
        Abort();
        return Status::Success;
    }
    if (total - failures < threshold_) {
        Abort();
        return Status::Failure;
    }
    return Status::Running;
}

Status Inverter::Tick(Blackboard& blackboard, float dt) {
    const Status status = child_ ? child_->Tick(blackboard, dt) : Status::Failure;
    if (status == Status::Running) return status;
    return status == Status::Success ? Status::Failure : Status::Success;
}

Status Succeeder::Tick(Blackboard& blackboard, float dt) {
    const Status status = child_ ? child_->Tick(blackboard, dt) : Status::Success;
    return status == Status::Running ? Status::Running : Status::Success;
}

Status Repeat::Tick(Blackboard& blackboard, float dt) {
    if (!child_) return Status::Failure;
    const Status status = child_->Tick(blackboard, dt);
    if (status == Status::Running) return status;
    if (status == Status::Failure) {
        Abort();
        return Status::Failure;
    }
    ++done_;
    child_->Abort();
    if (count_ >= 0 && done_ >= count_) {
        done_ = 0;
        return Status::Success;
    }
    return Status::Running;
}

void Repeat::Reset() {
    done_ = 0;
    Decorator::Reset();
}

void Repeat::Abort() {
    done_ = 0;
    Decorator::Abort();
}

Status Cooldown::Tick(Blackboard& blackboard, float dt) {
    if (!childRunning_ && remaining_ > 0.0f) {
        remaining_ = std::max(0.0f, remaining_ - std::max(0.0f, dt));
        return Status::Failure;
    }
    const Status status = child_ ? child_->Tick(blackboard, dt) : Status::Failure;
    childRunning_ = status == Status::Running;
    if (!childRunning_) remaining_ = seconds_;
    return status;
}

void Cooldown::Reset() {
    remaining_ = 0.0f;
    childRunning_ = false;
    Decorator::Reset();
}

void Cooldown::Abort() {
    // An aborted branch keeps its cooldown; an aborted running child starts it.
    if (childRunning_) remaining_ = seconds_;
    childRunning_ = false;
    Decorator::Abort();
}

Status TimeLimit::Tick(Blackboard& blackboard, float dt) {
    elapsed_ += std::max(0.0f, dt);
    if (elapsed_ > seconds_) {
        Abort();
        return Status::Failure;
    }
    const Status status = child_ ? child_->Tick(blackboard, dt) : Status::Failure;
    if (status != Status::Running) elapsed_ = 0.0f;
    return status;
}

void TimeLimit::Reset() {
    elapsed_ = 0.0f;
    Decorator::Reset();
}

Status Wait::Tick(Blackboard&, float dt) {
    elapsed_ += std::max(0.0f, std::isfinite(dt) ? dt : 0.0f);
    if (elapsed_ >= seconds_) {
        elapsed_ = 0.0f;
        return Status::Success;
    }
    return Status::Running;
}

Status BehaviorTree::Tick(Blackboard& blackboard, float dt) {
    if (!root_) return Status::Failure;
    last_ = root_->Tick(blackboard, dt);
    return last_;
}

void BehaviorTree::Reset() {
    if (root_) root_->Reset();
}

bool CanSee(const Physics::PhysicsWorld& world, Math::Vec3 eye, Math::Vec3 forward, Math::Vec3 target, float fovDegrees,
    float range, std::uint32_t occluderMask) {
    const Math::Vec3 toTarget = target - eye;
    const float distance = Math::Length(toTarget);
    if (!(distance > 1.0e-4f) || distance > range) return distance <= 1.0e-4f;
    const Math::Vec3 direction = toTarget / distance;
    if (Math::Dot(direction, Math::Normalize(forward, {0, 0, 1})) < std::cos(Math::Radians(fovDegrees * 0.5f))) return false;
    Physics::RaycastHit hit;
    if (!world.Raycast({eye, direction}, distance, hit, occluderMask)) return true;
    const Physics::Body* body = world.GetBody(hit.body);
    // Only static geometry occludes; characters and debris do not hide targets.
    return body && body->type != Physics::BodyType::Static;
}

} // namespace Astral::AI
