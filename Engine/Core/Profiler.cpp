#include "Engine/Core/Profiler.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <map>

namespace Astral::Core {

namespace {
void AppendJsonString(std::string& out, const char* text) {
    out.push_back('"');
    for (const char* c = text ? text : ""; *c; ++c) {
        const unsigned char value = static_cast<unsigned char>(*c);
        if (value == '"' || value == '\\') {
            out.push_back('\\');
            out.push_back(*c);
        } else if (value < 0x20) {
            static const char kHex[] = "0123456789abcdef";
            out += "\\u00";
            out.push_back(kHex[value >> 4]);
            out.push_back(kHex[value & 15]);
        } else {
            out.push_back(*c);
        }
    }
    out.push_back('"');
}
} // namespace

Profiler& Profiler::Instance() {
    static Profiler profiler;
    return profiler;
}

std::uint64_t Profiler::NowNs() {
    using Clock = std::chrono::steady_clock;
    static const Clock::time_point origin = Clock::now();
    // +1 keeps a valid timestamp distinguishable from ProfileScope's "not capturing" 0.
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - origin).count()) + 1u;
}

Profiler::ThreadBuffer& Profiler::LocalBuffer() {
    // Buffers are owned by the profiler so they outlive the threads that wrote them.
    thread_local ThreadBuffer* buffer = nullptr;
    if (!buffer) {
        std::lock_guard<std::mutex> lock(buffersMutex_);
        buffers_.push_back(std::make_unique<ThreadBuffer>());
        buffer = buffers_.back().get();
        buffer->threadIndex = static_cast<std::uint32_t>(buffers_.size() - 1);
    }
    return *buffer;
}

void Profiler::BeginCapture(std::size_t maxEvents) {
    std::lock_guard<std::mutex> lock(buffersMutex_);
    for (auto& buffer : buffers_) {
        std::lock_guard<std::mutex> bufferLock(buffer->mutex);
        buffer->events.clear();
    }
    budget_.store(maxEvents, std::memory_order_relaxed);
    dropped_.store(0, std::memory_order_relaxed);
    capturing_.store(true, std::memory_order_release);
}

void Profiler::EndCapture() { capturing_.store(false, std::memory_order_release); }

void Profiler::Record(const char* name, std::uint64_t beginNs, std::uint64_t endNs) {
    if (!Capturing()) return;
    std::size_t remaining = budget_.load(std::memory_order_relaxed);
    do {
        if (remaining == 0) {
            dropped_.fetch_add(1, std::memory_order_relaxed);
            return;
        }
    } while (!budget_.compare_exchange_weak(remaining, remaining - 1, std::memory_order_relaxed));
    ThreadBuffer& buffer = LocalBuffer();
    std::lock_guard<std::mutex> lock(buffer.mutex);
    buffer.events.push_back({name, beginNs, std::max(beginNs, endNs), buffer.threadIndex});
}

std::vector<ProfileEvent> Profiler::Events() const {
    std::vector<ProfileEvent> all;
    std::lock_guard<std::mutex> lock(buffersMutex_);
    for (const auto& buffer : buffers_) {
        std::lock_guard<std::mutex> bufferLock(buffer->mutex);
        all.insert(all.end(), buffer->events.begin(), buffer->events.end());
    }
    std::sort(all.begin(), all.end(), [](const ProfileEvent& a, const ProfileEvent& b) {
        return a.beginNs != b.beginNs ? a.beginNs < b.beginNs : a.threadIndex < b.threadIndex;
    });
    return all;
}

std::vector<ProfileZoneStats> Profiler::Summarize() const {
    std::map<std::string, ProfileZoneStats> zones;
    for (const ProfileEvent& event : Events()) {
        ProfileZoneStats& stats = zones[event.name ? event.name : ""];
        stats.name = event.name ? event.name : "";
        const double ms = static_cast<double>(event.endNs - event.beginNs) / 1.0e6;
        ++stats.count;
        stats.totalMs += ms;
        stats.maxMs = std::max(stats.maxMs, ms);
    }
    std::vector<ProfileZoneStats> result;
    for (auto& entry : zones) result.push_back(entry.second);
    std::sort(result.begin(), result.end(),
        [](const ProfileZoneStats& a, const ProfileZoneStats& b) { return a.totalMs > b.totalMs; });
    return result;
}

bool Profiler::WriteChromeTrace(const std::string& path, std::string& error) const {
    std::string json = "{\"displayTimeUnit\":\"ms\",\"traceEvents\":[";
    bool first = true;
    for (const ProfileEvent& event : Events()) {
        if (!first) json += ",";
        first = false;
        json += "{\"name\":";
        AppendJsonString(json, event.name);
        json += ",\"ph\":\"X\",\"pid\":1,\"tid\":" + std::to_string(event.threadIndex);
        json += ",\"ts\":" + std::to_string(static_cast<double>(event.beginNs) / 1000.0);
        json += ",\"dur\":" + std::to_string(static_cast<double>(event.endNs - event.beginNs) / 1000.0);
        json += "}";
    }
    json += "],\"otherData\":{\"droppedEvents\":" + std::to_string(DroppedEvents()) + "}}\n";
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        error = "cannot open trace output: " + path;
        return false;
    }
    file.write(json.data(), static_cast<std::streamsize>(json.size()));
    if (!file) {
        error = "failed writing trace output: " + path;
        return false;
    }
    return true;
}

} // namespace Astral::Core
