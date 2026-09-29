#pragma once

// Software audio engine: PCM clips, a thread-safe voice mixer with buses
// (master/SFX/music/voice/UI), per-voice gain/pitch/pan, looping, 3D
// spatialisation (inverse-distance attenuation, equal-power panning from the
// listener frame), priority-based voice stealing, bus ducking and a soft
// limiter. Mix() is the only call the platform audio thread makes. A
// procedural synth provides placeholder SFX without external assets.

#include "Engine/Math/VectorMath.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace Astral::Audio {

struct AudioClip {
    int sampleRate{48000};
    std::vector<float> samples; // mono
    float Duration() const { return sampleRate > 0 ? static_cast<float>(samples.size()) / static_cast<float>(sampleRate) : 0.0f; }
};

enum class Bus : std::uint8_t { Master, Sfx, Music, Voice, Ui, Count };

struct PlayParams {
    float volume{1.0f};
    float pitch{1.0f};
    float pan{0.0f}; // -1 left .. +1 right (non-spatial voices)
    bool loop{false};
    Bus bus{Bus::Sfx};
    bool spatial{false};
    Math::Vec3 position{};
    float minDistance{1.0f};
    float maxDistance{40.0f};
    int priority{0}; // higher survives voice stealing
};

struct VoiceId {
    std::uint32_t index{0xFFFFFFFFu};
    std::uint32_t generation{};
    bool IsNull() const { return index == 0xFFFFFFFFu; }
};

struct MixerStats {
    int activeVoices{};
    int stolenVoices{};
    float peak{};
    std::uint64_t limitedSamples{};
};

class AudioMixer {
public:
    explicit AudioMixer(int sampleRate = 48000, int maxVoices = 32);

    VoiceId Play(const AudioClip* clip, const PlayParams& params);
    void Stop(VoiceId id);
    bool IsPlaying(VoiceId id) const;
    void SetVoicePosition(VoiceId id, Math::Vec3 position);
    void SetVoiceVolume(VoiceId id, float volume);
    void SetBusVolume(Bus bus, float volume);
    // Temporarily lowers a bus (e.g. music under the Thought Focus drone).
    void Duck(Bus bus, float amount, float seconds);
    void SetListener(Math::Vec3 position, Math::Vec3 forward, Math::Vec3 up = {0, 1, 0});

    // Renders interleaved stereo float frames. Called from the audio thread.
    void Mix(float* stereo, int frames);
    MixerStats Stats() const;
    int SampleRate() const { return sampleRate_; }

private:
    struct Voice {
        const AudioClip* clip{};
        PlayParams params;
        double cursor{};
        std::uint32_t generation{1};
        bool active{};
        std::uint64_t startOrder{};
    };
    void ComputeGains(const Voice& voice, float& left, float& right) const;

    int sampleRate_;
    std::vector<Voice> voices_;
    float busVolume_[static_cast<int>(Bus::Count)]{1, 1, 1, 1, 1};
    float duckAmount_[static_cast<int>(Bus::Count)]{};
    float duckRemaining_[static_cast<int>(Bus::Count)]{};
    Math::Vec3 listenerPosition_{};
    Math::Vec3 listenerRight_{1, 0, 0};
    Math::Vec3 listenerForward_{0, 0, 1};
    std::uint64_t order_{};
    MixerStats stats_;
    mutable std::mutex mutex_;
};

namespace Synth {
AudioClip Tone(float frequency, float seconds, float attack = 0.01f, float release = 0.1f, int sampleRate = 48000);
// Filtered-noise sweep: blade swings and dashes.
AudioClip Whoosh(float seconds, float startCutoff, float endCutoff, std::uint32_t seed = 1, int sampleRate = 48000);
// Pitch-dropping thump plus noise crack: hits and impacts.
AudioClip Impact(float seconds, float pitch, std::uint32_t seed = 2, int sampleRate = 48000);
// Bright bell partials: discovery / reward chimes.
AudioClip Chime(float frequency, float seconds, int sampleRate = 48000);
// Detuned low drone with slow beating: Thought Focus.
AudioClip Drone(float frequency, float seconds, int sampleRate = 48000);
AudioClip Footstep(std::uint32_t seed, int sampleRate = 48000);
} // namespace Synth

// 16-bit PCM WAV writer for captures and tests.
bool WriteWav(const std::string& path, const std::vector<float>& stereo, int sampleRate, std::string& error);
// WAV reader for sound assets: PCM 8/16/24/32-bit, IEEE float 32/64-bit and
// WAVE_FORMAT_EXTENSIBLE, any channel count (downmixed to the mono AudioClip).
bool DecodeWav(const std::uint8_t* data, std::size_t size, AudioClip& out, std::string& error);
bool ReadWav(const std::string& path, AudioClip& out, std::string& error);

} // namespace Astral::Audio
