#pragma once

// Generational handles: stale handles are detected instead of aliasing a new
// object that reused the slot (the pattern behind UE's FObjectHandle checks and
// most modern ECS entity ids).

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace Astral::Core {

struct Handle {
    std::uint32_t index{0xFFFFFFFFu};
    std::uint32_t generation{0};

    bool IsNull() const { return index == 0xFFFFFFFFu; }
    friend bool operator==(Handle a, Handle b) { return a.index == b.index && a.generation == b.generation; }
    friend bool operator!=(Handle a, Handle b) { return !(a == b); }
};

template <typename T>
class SlotMap {
public:
    template <typename... Args>
    Handle Emplace(Args&&... args) {
        std::uint32_t index;
        if (!freeList_.empty()) {
            index = freeList_.back();
            freeList_.pop_back();
            slots_[index].value = T(std::forward<Args>(args)...);
            slots_[index].alive = true;
        } else {
            index = static_cast<std::uint32_t>(slots_.size());
            slots_.push_back(Slot{T(std::forward<Args>(args)...), 1u, true});
        }
        ++size_;
        return {index, slots_[index].generation};
    }

    bool Remove(Handle handle) {
        if (!Contains(handle)) return false;
        Slot& slot = slots_[handle.index];
        slot.alive = false;
        slot.value = T();
        // Skip generation 0 on wrap so a default Handle{} never validates.
        slot.generation = slot.generation == 0xFFFFFFFFu ? 1u : slot.generation + 1u;
        freeList_.push_back(handle.index);
        --size_;
        return true;
    }

    bool Contains(Handle handle) const {
        return handle.index < slots_.size() && slots_[handle.index].alive
            && slots_[handle.index].generation == handle.generation;
    }

    T* Get(Handle handle) { return Contains(handle) ? &slots_[handle.index].value : nullptr; }
    const T* Get(Handle handle) const { return Contains(handle) ? &slots_[handle.index].value : nullptr; }

    std::size_t Size() const { return size_; }
    void Clear() {
        for (std::uint32_t index = 0; index < slots_.size(); ++index) {
            if (slots_[index].alive) Remove({index, slots_[index].generation});
        }
    }

    template <typename Fn>
    void ForEach(Fn&& fn) {
        for (std::uint32_t index = 0; index < slots_.size(); ++index) {
            if (slots_[index].alive) fn(Handle{index, slots_[index].generation}, slots_[index].value);
        }
    }
    template <typename Fn>
    void ForEach(Fn&& fn) const {
        for (std::uint32_t index = 0; index < slots_.size(); ++index) {
            if (slots_[index].alive) fn(Handle{index, slots_[index].generation}, slots_[index].value);
        }
    }

private:
    struct Slot {
        T value{};
        std::uint32_t generation{1};
        bool alive{};
    };
    std::vector<Slot> slots_;
    std::vector<std::uint32_t> freeList_;
    std::size_t size_{};
};

} // namespace Astral::Core
