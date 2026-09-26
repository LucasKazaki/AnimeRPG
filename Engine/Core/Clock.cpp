#include "Engine/Core/Clock.h"

namespace Astral::Core {

Clock::Clock() : lastTick_(std::chrono::steady_clock::now()), start_(lastTick_) {}

float Clock::Tick() {
    const auto now = std::chrono::steady_clock::now();
    const auto delta = std::chrono::duration<float>(now - lastTick_).count();
    lastTick_ = now;
    return delta;
}

double Clock::ElapsedSeconds() const {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start_).count();
}

JobScheduler::JobScheduler(std::size_t numThreads) {
    if (numThreads == 0) {
        numThreads = 1;
    }
    workers_.reserve(numThreads);
    for (std::size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back(&JobScheduler::WorkerLoop, this);
    }
}

JobScheduler::~JobScheduler() {
    Shutdown();
}

void JobScheduler::Submit(std::function<void()> job) {
    if (!job) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shutdown_) {
            return;
        }
        jobs_.push(std::move(job));
    }
    condition_.notify_one();
}

void JobScheduler::WaitAll() {
    std::unique_lock<std::mutex> lock(mutex_);
    completion_.wait(lock, [this] {
        return jobs_.empty() && activeJobs_ == 0;
    });
}

void JobScheduler::Shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shutdown_) {
            return;
        }
        shutdown_ = true;
    }
    condition_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
}

void JobScheduler::WorkerLoop() {
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this] {
                return shutdown_ || !jobs_.empty();
            });
            if (shutdown_ && jobs_.empty()) {
                return;
            }
            job = std::move(jobs_.front());
            jobs_.pop();
            ++activeJobs_;
        }

        try {
            job();
        } catch (...) {
            // A failed job must not terminate the worker or strand the queue.
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            --activeJobs_;
            if (jobs_.empty() && activeJobs_ == 0) {
                completion_.notify_all();
            }
        }
    }
}

} // namespace Astral::Core