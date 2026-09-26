#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace Astral::Core {

class Clock {
public:
    Clock();
    float Tick();
    double ElapsedSeconds() const;

private:
    std::chrono::steady_clock::time_point lastTick_;
    std::chrono::steady_clock::time_point start_;
};

// Runtime/jobs: bounded thread-pool scheduler used by the engine runtime.
class JobScheduler {
public:
    explicit JobScheduler(std::size_t numThreads = std::thread::hardware_concurrency());
    ~JobScheduler();

    void Submit(std::function<void()> job);
    void WaitAll();
    void Shutdown();

private:
    void WorkerLoop();

    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::condition_variable completion_;
    std::queue<std::function<void()>> jobs_;
    std::vector<std::thread> workers_;
    std::size_t activeJobs_ = 0;
    bool shutdown_ = false;
};

} // namespace Astral::Core