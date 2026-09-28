#pragma once

// Dependency-aware job system (the Astral counterpart of UE's Tasks System and
// Unity's C# Job System). Jobs form a DAG: a job runs only after all of its
// prerequisites complete. Waiting threads help execute queued work, so waits
// never deadlock while work remains. With zero workers every job runs inline
// on the waiting thread, which gives a deterministic single-threaded mode.

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <functional>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace Astral::Core {

class JobSystem;

namespace Detail {
struct JobState {
    std::function<void()> work;
    std::atomic<int> pendingPrerequisites{0};
    std::atomic<bool> complete{false};
    std::mutex successorsMutex;
    std::vector<std::shared_ptr<JobState>> successors;
    std::exception_ptr error;
};
} // namespace Detail

class JobHandle {
public:
    JobHandle() = default;
    bool Valid() const { return state_ != nullptr; }
    bool IsComplete() const { return !state_ || state_->complete.load(std::memory_order_acquire); }

private:
    friend class JobSystem;
    explicit JobHandle(std::shared_ptr<Detail::JobState> state) : state_(std::move(state)) {}
    std::shared_ptr<Detail::JobState> state_;
};

struct JobSystemStats {
    std::size_t scheduled{};
    std::size_t executed{};
    std::size_t executedByWaiters{};
};

class JobSystem {
public:
    // workerCount == -1 selects hardware_concurrency() - 1 (at least one).
    explicit JobSystem(int workerCount = -1);
    ~JobSystem();
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    JobHandle Schedule(std::function<void()> work, std::initializer_list<JobHandle> prerequisites = {});
    JobHandle Schedule(std::function<void()> work, const std::vector<JobHandle>& prerequisites);

    // Blocks until the job completes, executing other queued jobs meanwhile.
    // Rethrows the first exception thrown by the job itself.
    void Wait(const JobHandle& handle);
    void WaitAll(const std::vector<JobHandle>& handles);

    // Splits [0, count) into batches of at least minBatchSize and blocks until
    // every batch has run. fn(begin, end) must be safe to call concurrently.
    void ParallelFor(std::size_t count, std::size_t minBatchSize,
        const std::function<void(std::size_t, std::size_t)>& fn);

    int WorkerCount() const { return static_cast<int>(workers_.size()); }
    JobSystemStats Stats() const;

private:
    void Enqueue(const std::shared_ptr<Detail::JobState>& job);
    bool TryRunOne(bool fromWaiter);
    void Execute(const std::shared_ptr<Detail::JobState>& job, bool fromWaiter);
    void WorkerLoop();

    std::vector<std::thread> workers_;
    std::deque<std::shared_ptr<Detail::JobState>> queue_;
    mutable std::mutex mutex_;
    std::condition_variable workAvailable_;
    std::condition_variable jobCompleted_;
    bool stopping_{false};
    std::atomic<std::size_t> outstanding_{0};
    std::atomic<std::size_t> scheduled_{0};
    std::atomic<std::size_t> executed_{0};
    std::atomic<std::size_t> executedByWaiters_{0};
};

} // namespace Astral::Core
