#include "Engine/Core/JobSystem.h"

#include <algorithm>

namespace Astral::Core {

JobSystem::JobSystem(int workerCount) {
    if (workerCount < 0) {
        const unsigned hardware = std::thread::hardware_concurrency();
        workerCount = hardware > 1 ? static_cast<int>(hardware) - 1 : 1;
    }
    workers_.reserve(static_cast<std::size_t>(workerCount));
    for (int index = 0; index < workerCount; ++index) {
        workers_.emplace_back([this] { WorkerLoop(); });
    }
}

JobSystem::~JobSystem() {
    // Drain: every scheduled job (including dependents released while draining)
    // completes before the workers stop, so no handle is left forever pending.
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (outstanding_.load(std::memory_order_acquire) != 0) {
            if (!queue_.empty() || workers_.empty()) {
                lock.unlock();
                TryRunOne(true);
                lock.lock();
            } else {
                jobCompleted_.wait(lock);
            }
        }
        stopping_ = true;
    }
    workAvailable_.notify_all();
    for (std::thread& worker : workers_) worker.join();
}

JobHandle JobSystem::Schedule(std::function<void()> work, std::initializer_list<JobHandle> prerequisites) {
    return Schedule(std::move(work), std::vector<JobHandle>(prerequisites));
}

JobHandle JobSystem::Schedule(std::function<void()> work, const std::vector<JobHandle>& prerequisites) {
    auto job = std::make_shared<Detail::JobState>();
    job->work = std::move(work);
    // +1 guard prevents a prerequisite finishing mid-registration from
    // enqueueing the job before registration is complete.
    job->pendingPrerequisites.store(1, std::memory_order_relaxed);
    outstanding_.fetch_add(1, std::memory_order_acq_rel);
    scheduled_.fetch_add(1, std::memory_order_relaxed);
    for (const JobHandle& prerequisite : prerequisites) {
        if (!prerequisite.state_) continue;
        std::lock_guard<std::mutex> lock(prerequisite.state_->successorsMutex);
        if (prerequisite.state_->complete.load(std::memory_order_acquire)) continue;
        job->pendingPrerequisites.fetch_add(1, std::memory_order_acq_rel);
        prerequisite.state_->successors.push_back(job);
    }
    if (job->pendingPrerequisites.fetch_sub(1, std::memory_order_acq_rel) == 1) Enqueue(job);
    return JobHandle(job);
}

void JobSystem::Enqueue(const std::shared_ptr<Detail::JobState>& job) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(job);
    }
    workAvailable_.notify_one();
    // Waiters on the main thread may be able to help with the new job.
    jobCompleted_.notify_all();
}

bool JobSystem::TryRunOne(bool fromWaiter) {
    std::shared_ptr<Detail::JobState> job;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return false;
        job = std::move(queue_.front());
        queue_.pop_front();
    }
    Execute(job, fromWaiter);
    return true;
}

void JobSystem::Execute(const std::shared_ptr<Detail::JobState>& job, bool fromWaiter) {
    try {
        if (job->work) job->work();
    } catch (...) {
        job->error = std::current_exception();
    }
    job->work = nullptr;
    // Statistics are published before completion so a waiter never observes a
    // completed job that is missing from Stats().
    executed_.fetch_add(1, std::memory_order_relaxed);
    if (fromWaiter) executedByWaiters_.fetch_add(1, std::memory_order_relaxed);
    std::vector<std::shared_ptr<Detail::JobState>> released;
    {
        std::lock_guard<std::mutex> lock(job->successorsMutex);
        job->complete.store(true, std::memory_order_release);
        released.swap(job->successors);
    }
    for (const auto& successor : released) {
        if (successor->pendingPrerequisites.fetch_sub(1, std::memory_order_acq_rel) == 1) Enqueue(successor);
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        outstanding_.fetch_sub(1, std::memory_order_acq_rel);
    }
    jobCompleted_.notify_all();
}

void JobSystem::WorkerLoop() {
    for (;;) {
        std::shared_ptr<Detail::JobState> job;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            workAvailable_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
            if (queue_.empty()) return; // stopping_ with nothing left
            job = std::move(queue_.front());
            queue_.pop_front();
        }
        Execute(job, false);
    }
}

void JobSystem::Wait(const JobHandle& handle) {
    if (!handle.state_) return;
    while (!handle.state_->complete.load(std::memory_order_acquire)) {
        if (TryRunOne(true)) continue;
        std::unique_lock<std::mutex> lock(mutex_);
        jobCompleted_.wait(lock, [&] {
            return handle.state_->complete.load(std::memory_order_acquire) || !queue_.empty();
        });
    }
    if (handle.state_->error) std::rethrow_exception(handle.state_->error);
}

void JobSystem::WaitAll(const std::vector<JobHandle>& handles) {
    std::exception_ptr first;
    for (const JobHandle& handle : handles) {
        try {
            Wait(handle);
        } catch (...) {
            if (!first) first = std::current_exception();
        }
    }
    if (first) std::rethrow_exception(first);
}

void JobSystem::ParallelFor(std::size_t count, std::size_t minBatchSize,
    const std::function<void(std::size_t, std::size_t)>& fn) {
    if (count == 0) return;
    minBatchSize = std::max<std::size_t>(1, minBatchSize);
    const std::size_t lanes = static_cast<std::size_t>(workers_.size()) + 1;
    // Several batches per lane smooths out uneven work (e.g. screen tiles).
    std::size_t batchSize = std::max(minBatchSize, (count + lanes * 4 - 1) / (lanes * 4));
    const std::size_t batches = (count + batchSize - 1) / batchSize;
    if (batches <= 1 || workers_.empty()) {
        fn(0, count);
        return;
    }
    std::vector<JobHandle> handles;
    handles.reserve(batches - 1);
    for (std::size_t batch = 1; batch < batches; ++batch) {
        const std::size_t begin = batch * batchSize;
        const std::size_t end = std::min(count, begin + batchSize);
        handles.push_back(Schedule([&fn, begin, end] { fn(begin, end); }));
    }
    std::exception_ptr localError;
    try {
        fn(0, std::min(count, batchSize));
    } catch (...) {
        localError = std::current_exception();
    }
    // Always wait for the batches: they reference fn, which lives on this stack.
    std::exception_ptr batchError;
    try {
        WaitAll(handles);
    } catch (...) {
        batchError = std::current_exception();
    }
    if (localError) std::rethrow_exception(localError);
    if (batchError) std::rethrow_exception(batchError);
}

JobSystemStats JobSystem::Stats() const {
    return {scheduled_.load(), executed_.load(), executedByWaiters_.load()};
}

} // namespace Astral::Core
