#pragma once

// Scoped CPU zone profiler with Chrome Trace Event export (open the JSON in
// Perfetto or chrome://tracing). It fills the "Unreal Insights timeline" role
// for Astral: nested zones per thread, bounded memory, aggregate summaries.

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Astral::Core {

struct ProfileEvent {
    const char* name{};
    std::uint64_t beginNs{};
    std::uint64_t endNs{};
    std::uint32_t threadIndex{};
};

struct ProfileZoneStats {
    std::string name;
    std::uint64_t count{};
    double totalMs{};
    double maxMs{};
};

class Profiler {
public:
    static Profiler& Instance();
    static std::uint64_t NowNs();

    // Starts a fresh capture that keeps at most maxEvents; later events are counted as dropped.
    void BeginCapture(std::size_t maxEvents = 1u << 20);
    void EndCapture();
    bool Capturing() const { return capturing_.load(std::memory_order_acquire); }

    // `name` must outlive the capture (string literals in practice).
    void Record(const char* name, std::uint64_t beginNs, std::uint64_t endNs);

    std::vector<ProfileEvent> Events() const;
    std::vector<ProfileZoneStats> Summarize() const;
    std::size_t DroppedEvents() const { return dropped_.load(std::memory_order_relaxed); }
    bool WriteChromeTrace(const std::string& path, std::string& error) const;

private:
    struct ThreadBuffer {
        std::mutex mutex;
        std::vector<ProfileEvent> events;
        std::uint32_t threadIndex{};
    };
    ThreadBuffer& LocalBuffer();

    std::atomic<bool> capturing_{false};
    std::atomic<std::size_t> budget_{0};
    std::atomic<std::size_t> dropped_{0};
    mutable std::mutex buffersMutex_;
    std::vector<std::unique_ptr<ThreadBuffer>> buffers_;
};

class ProfileScope {
public:
    explicit ProfileScope(const char* name)
        : name_(name), begin_(Profiler::Instance().Capturing() ? Profiler::NowNs() : 0) {}
    ~ProfileScope() {
        if (begin_ != 0) Profiler::Instance().Record(name_, begin_, Profiler::NowNs());
    }
    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;

private:
    const char* name_;
    std::uint64_t begin_;
};

} // namespace Astral::Core

#define ASTRAL_PROFILE_CONCAT_INNER(a, b) a##b
#define ASTRAL_PROFILE_CONCAT(a, b) ASTRAL_PROFILE_CONCAT_INNER(a, b)
#define ASTRAL_PROFILE_SCOPE(name) \
    ::Astral::Core::ProfileScope ASTRAL_PROFILE_CONCAT(astralProfileScope, __LINE__)(name)
