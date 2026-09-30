#pragma once

// Multicast delegates (Unreal's DECLARE_MULTICAST_DELEGATE, C# events) and a
// typed event bus. Handlers may add or remove handlers — including themselves —
// while a broadcast is running: removals take effect immediately, additions
// start with the next broadcast. Not thread-safe; use from one thread.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Astral::Core {

using DelegateHandle = std::uint64_t;

template <typename... Args>
class MulticastDelegate {
public:
    DelegateHandle Add(std::function<void(Args...)> function) {
        if (!function) return 0;
        const DelegateHandle handle = ++nextHandle_;
        entries_.push_back({handle, std::move(function), true});
        return handle;
    }
    // Owner-scoped removal: RemoveAll(owner) drops every handler added with that owner.
    DelegateHandle Add(const void* owner, std::function<void(Args...)> function) {
        const DelegateHandle handle = Add(std::move(function));
        if (handle) entries_.back().owner = owner;
        return handle;
    }
    bool Remove(DelegateHandle handle) {
        for (Entry& entry : entries_) {
            if (entry.handle == handle && entry.alive) {
                entry.alive = false;
                Compact();
                return true;
            }
        }
        return false;
    }
    std::size_t RemoveAll(const void* owner) {
        std::size_t removed = 0;
        for (Entry& entry : entries_) {
            if (entry.alive && entry.owner == owner && owner) {
                entry.alive = false;
                ++removed;
            }
        }
        Compact();
        return removed;
    }
    void Clear() {
        for (Entry& entry : entries_) entry.alive = false;
        Compact();
    }
    std::size_t Count() const {
        return static_cast<std::size_t>(std::count_if(entries_.begin(), entries_.end(), [](const Entry& e) { return e.alive; }));
    }
    bool IsBound() const { return Count() > 0; }

    void Broadcast(Args... args) {
        ++depth_;
        const std::size_t count = entries_.size(); // handlers added during the broadcast wait for the next one
        for (std::size_t i = 0; i < count && i < entries_.size(); ++i) {
            if (!entries_[i].alive) continue;
            // Copy: the handler may clear the delegate (and destroy its own std::function).
            const std::function<void(Args...)> function = entries_[i].function;
            function(args...);
        }
        --depth_;
        Compact();
    }

private:
    struct Entry {
        DelegateHandle handle{};
        std::function<void(Args...)> function;
        bool alive{};
        const void* owner{};
    };
    void Compact() {
        if (depth_ > 0) return;
        entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [](const Entry& e) { return !e.alive; }),
            entries_.end());
    }
    std::vector<Entry> entries_;
    DelegateHandle nextHandle_{};
    int depth_{};
};

// RAII subscription: removes itself from the delegate on destruction.
class ScopedSubscription {
public:
    ScopedSubscription() = default;
    explicit ScopedSubscription(std::function<void()> unsubscribe) : unsubscribe_(std::move(unsubscribe)) {}
    ScopedSubscription(ScopedSubscription&& other) noexcept : unsubscribe_(std::move(other.unsubscribe_)) {
        other.unsubscribe_ = nullptr;
    }
    ScopedSubscription& operator=(ScopedSubscription&& other) noexcept {
        if (this != &other) {
            Reset();
            unsubscribe_ = std::move(other.unsubscribe_);
            other.unsubscribe_ = nullptr;
        }
        return *this;
    }
    ScopedSubscription(const ScopedSubscription&) = delete;
    ScopedSubscription& operator=(const ScopedSubscription&) = delete;
    ~ScopedSubscription() { Reset(); }
    void Reset() {
        if (unsubscribe_) unsubscribe_();
        unsubscribe_ = nullptr;
    }

private:
    std::function<void()> unsubscribe_;
};

// Typed publish/subscribe keyed by event type. Publish delivers immediately;
// Queue defers until Dispatch (e.g. once per frame, after physics).
class EventBus {
public:
    template <typename Event>
    DelegateHandle Subscribe(std::function<void(const Event&)> handler) {
        return Channel<Event>().Add(std::move(handler));
    }
    template <typename Event>
    bool Unsubscribe(DelegateHandle handle) {
        return Channel<Event>().Remove(handle);
    }
    template <typename Event>
    ScopedSubscription SubscribeScoped(std::function<void(const Event&)> handler) {
        const DelegateHandle handle = Subscribe<Event>(std::move(handler));
        return ScopedSubscription([this, handle] { Unsubscribe<Event>(handle); });
    }
    template <typename Event>
    void Publish(const Event& event) {
        Channel<Event>().Broadcast(event);
    }
    template <typename Event>
    void Queue(Event event) {
        auto shared = std::make_shared<Event>(std::move(event));
        queued_.push_back([this, shared] { Publish<Event>(*shared); });
    }
    // Delivers queued events in order; events queued by handlers are delivered
    // in the same call (bounded to avoid feedback loops).
    std::size_t Dispatch(std::size_t maxEvents = 100000) {
        std::size_t delivered = 0;
        while (!queued_.empty() && delivered < maxEvents) {
            std::vector<std::function<void()>> batch;
            batch.swap(queued_);
            for (auto& deliver : batch) {
                deliver();
                ++delivered;
            }
        }
        return delivered;
    }
    std::size_t QueuedCount() const { return queued_.size(); }
    template <typename Event>
    std::size_t SubscriberCount() const {
        const auto it = channels_.find(std::type_index(typeid(Event)));
        return it == channels_.end() ? 0 : static_cast<ChannelHolder<Event>*>(it->second.get())->delegate.Count();
    }

private:
    struct ChannelBase {
        virtual ~ChannelBase() = default;
    };
    template <typename Event>
    struct ChannelHolder : ChannelBase {
        MulticastDelegate<const Event&> delegate;
    };
    template <typename Event>
    MulticastDelegate<const Event&>& Channel() {
        std::unique_ptr<ChannelBase>& slot = channels_[std::type_index(typeid(Event))];
        if (!slot) slot = std::make_unique<ChannelHolder<Event>>();
        return static_cast<ChannelHolder<Event>*>(slot.get())->delegate;
    }
    std::unordered_map<std::type_index, std::unique_ptr<ChannelBase>> channels_;
    std::vector<std::function<void()>> queued_;
};

} // namespace Astral::Core
