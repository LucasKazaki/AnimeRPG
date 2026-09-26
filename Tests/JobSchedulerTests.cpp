#include "Engine/Core/Clock.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>

namespace {

bool RunAndWaitAll() {
    Astral::Core::JobScheduler scheduler(2);
    std::atomic<int> completed{0};
    constexpr int jobCount = 32;

    for (int i = 0; i < jobCount; ++i) {
        scheduler.Submit([&completed] {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            completed.fetch_add(1, std::memory_order_relaxed);
        });
    }

    scheduler.WaitAll();
    return completed.load(std::memory_order_relaxed) == jobCount;
}

bool ShutdownDrainsQueuedJobs() {
    Astral::Core::JobScheduler scheduler(2);
    std::atomic<int> completed{0};
    constexpr int jobCount = 16;

    for (int i = 0; i < jobCount; ++i) {
        scheduler.Submit([&completed] {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            completed.fetch_add(1, std::memory_order_relaxed);
        });
    }

    scheduler.Shutdown();
    return completed.load(std::memory_order_relaxed) == jobCount;
}

bool ZeroThreadInputStillRuns() {
    Astral::Core::JobScheduler scheduler(0);
    std::atomic<int> completed{0};
    scheduler.Submit([&completed] { completed.fetch_add(1, std::memory_order_relaxed); });
    scheduler.WaitAll();
    return completed.load(std::memory_order_relaxed) == 1;
}

bool ShutdownRejectsNewJobs() {
    Astral::Core::JobScheduler scheduler(1);
    std::atomic<int> completed{0};
    scheduler.Shutdown();
    scheduler.Submit([&completed] { completed.fetch_add(1, std::memory_order_relaxed); });
    scheduler.WaitAll();
    return completed.load(std::memory_order_relaxed) == 0;
}

bool DeterministicShutdownUnderLoad() {
    Astral::Core::JobScheduler scheduler(4);
    std::atomic<int> completed{0};
    constexpr int rounds = 8;
    constexpr int jobsPerRound = 32;

    for (int round = 0; round < rounds; ++round) {
        for (int index = 0; index < jobsPerRound; ++index) {
            scheduler.Submit([&completed, round, index, jobsPerRound] {
                if (((round * jobsPerRound) + index) % 7 == 0) {
                    std::this_thread::yield();
                }
                completed.fetch_add(1, std::memory_order_relaxed);
            });
        }
    }

    scheduler.Shutdown();
    return completed.load(std::memory_order_relaxed) == rounds * jobsPerRound;
}

} // namespace

int main() {
    if (!RunAndWaitAll()) return 1;
    if (!ShutdownDrainsQueuedJobs()) return 2;
    if (!ZeroThreadInputStillRuns()) return 3;
    if (!ShutdownRejectsNewJobs()) return 4;
    if (!DeterministicShutdownUnderLoad()) return 5;
    return 0;
}