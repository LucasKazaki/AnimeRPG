#include "Engine/Core/GameTime.h"
#include "Engine/Core/JobSystem.h"
#include "Engine/Core/Profiler.h"
#include "Engine/Core/Random.h"
#include "Engine/Core/SlotMap.h"
#include "Tests/EngineTestSupport.h"

#include <atomic>
#include <cstdio>
#include <fstream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Astral::Core;

ASTRAL_TEST(JobDependenciesRunInOrder) {
    for (int workers : {0, 1, 3}) {
        JobSystem jobs(workers);
        std::vector<int> order;
        std::mutex orderMutex;
        auto record = [&](int value) {
            std::lock_guard<std::mutex> lock(orderMutex);
            order.push_back(value);
        };
        const JobHandle a = jobs.Schedule([&] { record(1); });
        const JobHandle b = jobs.Schedule([&] { record(2); }, {a});
        const JobHandle c = jobs.Schedule([&] { record(3); }, {a});
        const JobHandle d = jobs.Schedule([&] { record(4); }, {b, c});
        jobs.Wait(d);
        ASTRAL_CHECK(order.size() == 4);
        ASTRAL_CHECK(order.front() == 1);
        ASTRAL_CHECK(order.back() == 4);
        ASTRAL_CHECK(a.IsComplete() && b.IsComplete() && c.IsComplete() && d.IsComplete());
    }
}

ASTRAL_TEST(JobDiamondStressIsExact) {
    JobSystem jobs(3);
    std::atomic<int> counter{0};
    std::vector<JobHandle> layer;
    for (int i = 0; i < 64; ++i) layer.push_back(jobs.Schedule([&] { counter.fetch_add(1); }));
    for (int depth = 0; depth < 20; ++depth) {
        std::vector<JobHandle> next;
        for (int i = 0; i < 16; ++i) {
            next.push_back(jobs.Schedule([&] { counter.fetch_add(1); }, layer));
        }
        layer = next;
    }
    jobs.WaitAll(layer);
    ASTRAL_CHECK(counter.load() == 64 + 20 * 16);
    const JobSystemStats stats = jobs.Stats();
    ASTRAL_CHECK(stats.scheduled == stats.executed);
}

ASTRAL_TEST(ParallelForCoversRangeExactlyOnce) {
    for (int workers : {0, 2, 4}) {
        JobSystem jobs(workers);
        std::vector<int> hits(100003, 0);
        jobs.ParallelFor(hits.size(), 64, [&](std::size_t begin, std::size_t end) {
            for (std::size_t i = begin; i < end; ++i) ++hits[i];
        });
        for (int value : hits) ASTRAL_CHECK(value == 1);
        jobs.ParallelFor(0, 1, [&](std::size_t, std::size_t) { ASTRAL_CHECK(false); });
    }
}

ASTRAL_TEST(JobExceptionsPropagateToWaiter) {
    JobSystem jobs(2);
    const JobHandle failing = jobs.Schedule([] { throw std::runtime_error("boom"); });
    std::atomic<bool> dependentRan{false};
    const JobHandle dependent = jobs.Schedule([&] { dependentRan = true; }, {failing});
    bool caught = false;
    try {
        jobs.Wait(failing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    ASTRAL_CHECK(caught);
    jobs.Wait(dependent);
    ASTRAL_CHECK(dependentRan.load());
    bool parallelCaught = false;
    try {
        jobs.ParallelFor(1000, 10, [](std::size_t begin, std::size_t) {
            if (begin >= 500) throw std::logic_error("batch");
        });
    } catch (const std::logic_error&) {
        parallelCaught = true;
    }
    ASTRAL_CHECK(parallelCaught);
}

ASTRAL_TEST(JobSystemDestructorDrainsWork) {
    std::atomic<int> counter{0};
    {
        JobSystem jobs(2);
        JobHandle previous;
        for (int i = 0; i < 200; ++i) {
            previous = jobs.Schedule([&] { counter.fetch_add(1); }, {previous});
        }
    }
    ASTRAL_CHECK(counter.load() == 200);
}

ASTRAL_TEST(SlotMapDetectsStaleHandles) {
    SlotMap<std::string> map;
    const Handle a = map.Emplace("alpha");
    const Handle b = map.Emplace("beta");
    ASTRAL_CHECK(map.Size() == 2);
    ASTRAL_CHECK(*map.Get(a) == "alpha");
    ASTRAL_CHECK(map.Remove(a));
    ASTRAL_CHECK(!map.Remove(a));
    ASTRAL_CHECK(map.Get(a) == nullptr);
    const Handle c = map.Emplace("gamma");
    ASTRAL_CHECK(c.index == a.index);
    ASTRAL_CHECK(c.generation != a.generation);
    ASTRAL_CHECK(map.Get(a) == nullptr);
    ASTRAL_CHECK(*map.Get(c) == "gamma");
    ASTRAL_CHECK(!map.Contains(Handle{}));
    int visited = 0;
    map.ForEach([&](Handle, std::string&) { ++visited; });
    ASTRAL_CHECK(visited == 2);
    map.Clear();
    ASTRAL_CHECK(map.Size() == 0 && map.Get(b) == nullptr);
}

ASTRAL_TEST(RandomIsDeterministicAndBounded) {
    Random first(42), second(42), other(43);
    bool differs = false;
    for (int i = 0; i < 1000; ++i) {
        const std::uint32_t value = first.NextU32();
        ASTRAL_CHECK(value == second.NextU32());
        differs |= value != other.NextU32();
        const float f = first.NextFloat();
        second.NextFloat();
        ASTRAL_CHECK(f >= 0.0f && f < 1.0f);
        ASTRAL_CHECK(first.NextBounded(7) < 7u);
        second.NextBounded(7);
    }
    ASTRAL_CHECK(differs);
    // Coarse uniformity: each of 10 buckets gets 10% +- 2%.
    Random random(7);
    int buckets[10]{};
    for (int i = 0; i < 100000; ++i) ++buckets[random.NextBounded(10)];
    for (int count : buckets) ASTRAL_CHECK(count > 8000 && count < 12000);
}

ASTRAL_TEST(GameTimeDilationBlendsInRealTime) {
    GameTime time;
    time.SetDilation(0, 0.35f, 0.2f);
    for (int i = 0; i < 10; ++i) time.Advance(0.01f); // 0.1 s: halfway
    ASTRAL_CHECK_NEAR(time.GlobalScale(), 0.675f, 1e-3);
    for (int i = 0; i < 20; ++i) time.Advance(0.01f);
    ASTRAL_CHECK_NEAR(time.GlobalScale(), 0.35f, 1e-5);
    time.Advance(0.02f);
    ASTRAL_CHECK_NEAR(time.WorldDelta(), 0.007f, 1e-6);
    ASTRAL_CHECK_NEAR(time.RealDelta(), 0.02f, 1e-7);
    time.SetDilation(1, 0.5f, 0.0f);
    ASTRAL_CHECK_NEAR(time.GlobalScale(), 0.175f, 1e-5);
    time.SetGroupScale(2, 1.0f / 0.175f);
    time.Advance(0.01f);
    ASTRAL_CHECK_NEAR(time.GroupDelta(2), 0.01f, 1e-5);
    time.SetDilation(0, 1.0f, 0.0f);
    time.SetDilation(1, std::nanf(""), 0.0f);
    ASTRAL_CHECK_NEAR(time.GlobalScale(), 0.0f, 1e-7);
}

ASTRAL_TEST(GameTimeHitstopAndClamping) {
    GameTime time;
    time.TriggerHitstop(0.05f, 0.0f);
    time.Advance(0.03f);
    ASTRAL_CHECK(time.WorldDelta() == 0.0f);
    ASTRAL_CHECK(time.InHitstop());
    time.Advance(0.03f); // 0.02 frozen, 0.01 free
    ASTRAL_CHECK_NEAR(time.WorldDelta(), 0.01f, 1e-6);
    ASTRAL_CHECK(!time.InHitstop());
    time.Advance(5.0f); // hitch is clamped
    ASTRAL_CHECK_NEAR(time.RealDelta(), 0.1f, 1e-7);
    time.Advance(-1.0f);
    ASTRAL_CHECK(time.RealDelta() == 0.0f);
    time.Advance(std::nanf(""));
    ASTRAL_CHECK(time.RealDelta() == 0.0f);
}

ASTRAL_TEST(GameTimeFixedStepsAndSpiralGuard) {
    GameTimeConfig config;
    config.fixedStepSeconds = 0.01f;
    config.maxFixedStepsPerFrame = 3;
    config.maxDeltaSeconds = 1.0f;
    GameTime time(config);
    time.Advance(0.025f);
    ASTRAL_CHECK(time.FixedStepsThisFrame() == 2);
    ASTRAL_CHECK_NEAR(time.InterpolationAlpha(), 0.5f, 1e-3);
    time.Advance(0.1f);
    ASTRAL_CHECK(time.FixedStepsThisFrame() == 3);
    ASTRAL_CHECK(time.DroppedFixedSteps() >= 7);
}

ASTRAL_TEST(ProfilerCapturesNestedZonesAcrossThreads) {
    Profiler& profiler = Profiler::Instance();
    profiler.BeginCapture(1000);
    {
        ASTRAL_PROFILE_SCOPE("Frame");
        JobSystem jobs(2);
        jobs.ParallelFor(8, 1, [](std::size_t, std::size_t) { ASTRAL_PROFILE_SCOPE("Tile"); });
    }
    profiler.EndCapture();
    { ASTRAL_PROFILE_SCOPE("NotCaptured"); }
    const auto events = profiler.Events();
    int frames = 0, tiles = 0;
    for (const ProfileEvent& event : events) {
        const std::string name = event.name;
        frames += name == "Frame";
        tiles += name == "Tile";
        ASTRAL_CHECK(name != "NotCaptured");
        ASTRAL_CHECK(event.endNs >= event.beginNs);
    }
    ASTRAL_CHECK(frames == 1);
    ASTRAL_CHECK(tiles >= 1);
    const auto summary = profiler.Summarize();
    ASTRAL_CHECK(!summary.empty());
    std::string error;
    const std::string path = "astral_profiler_test_trace.json";
    ASTRAL_CHECK(profiler.WriteChromeTrace(path, error));
    std::ifstream file(path);
    std::stringstream contents;
    contents << file.rdbuf();
    ASTRAL_CHECK(contents.str().find("\"traceEvents\"") != std::string::npos);
    ASTRAL_CHECK(contents.str().find("\"Frame\"") != std::string::npos);
    file.close();
    std::remove(path.c_str());

    profiler.BeginCapture(2);
    for (int i = 0; i < 5; ++i) { ASTRAL_PROFILE_SCOPE("Budget"); }
    profiler.EndCapture();
    ASTRAL_CHECK(profiler.Events().size() == 2);
    ASTRAL_CHECK(profiler.DroppedEvents() == 3);
}

ASTRAL_TEST_MAIN("EngineCoreTests")
