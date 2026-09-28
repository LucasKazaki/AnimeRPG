#include "Engine/Audio/AudioMixer.h"

#include "Engine/Core/Random.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>

namespace Astral::Audio {

using namespace Math;

AudioMixer::AudioMixer(int sampleRate, int maxVoices)
    : sampleRate_(std::clamp(sampleRate, 8000, 192000)), voices_(static_cast<std::size_t>(std::clamp(maxVoices, 1, 256))) {}

VoiceId AudioMixer::Play(const AudioClip* clip, const PlayParams& params) {
    if (!clip || clip->samples.empty() || clip->sampleRate <= 0) return {};
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t slot = voices_.size();
    for (std::size_t i = 0; i < voices_.size(); ++i) {
        if (!voices_[i].active) {
            slot = i;
            break;
        }
    }
    if (slot == voices_.size()) {
        // Steal the lowest-priority, oldest voice if the new one is at least as important.
        std::size_t victim = 0;
        for (std::size_t i = 1; i < voices_.size(); ++i) {
            const Voice& a = voices_[i];
            const Voice& b = voices_[victim];
            if (a.params.priority < b.params.priority || (a.params.priority == b.params.priority && a.startOrder < b.startOrder))
                victim = i;
        }
        if (voices_[victim].params.priority > params.priority) return {};
        slot = victim;
        ++stats_.stolenVoices;
    }
    Voice& voice = voices_[slot];
    voice.clip = clip;
    voice.params = params;
    voice.params.volume = std::isfinite(params.volume) ? std::max(0.0f, params.volume) : 0.0f;
    voice.params.pitch = std::isfinite(params.pitch) ? Clamp(params.pitch, 0.05f, 8.0f) : 1.0f;
    voice.params.pan = std::isfinite(params.pan) ? Clamp(params.pan, -1.0f, 1.0f) : 0.0f;
    voice.cursor = 0.0;
    voice.active = true;
    voice.startOrder = ++order_;
    ++voice.generation;
    return {static_cast<std::uint32_t>(slot), voice.generation};
}

void AudioMixer::Stop(VoiceId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (id.index < voices_.size() && voices_[id.index].generation == id.generation) voices_[id.index].active = false;
}

bool AudioMixer::IsPlaying(VoiceId id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return id.index < voices_.size() && voices_[id.index].generation == id.generation && voices_[id.index].active;
}

void AudioMixer::SetVoicePosition(VoiceId id, Vec3 position) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (id.index < voices_.size() && voices_[id.index].generation == id.generation && IsFinite(position))
        voices_[id.index].params.position = position;
}

void AudioMixer::SetVoiceVolume(VoiceId id, float volume) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (id.index < voices_.size() && voices_[id.index].generation == id.generation && std::isfinite(volume))
        voices_[id.index].params.volume = std::max(0.0f, volume);
}

void AudioMixer::SetBusVolume(Bus bus, float volume) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (bus < Bus::Count && std::isfinite(volume)) busVolume_[static_cast<int>(bus)] = Clamp(volume, 0.0f, 4.0f);
}

void AudioMixer::Duck(Bus bus, float amount, float seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (bus >= Bus::Count || !std::isfinite(amount) || !std::isfinite(seconds)) return;
    duckAmount_[static_cast<int>(bus)] = Saturate(amount);
    duckRemaining_[static_cast<int>(bus)] = std::max(0.0f, seconds);
}

void AudioMixer::SetListener(Vec3 position, Vec3 forward, Vec3 up) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!IsFinite(position) || !IsFinite(forward)) return;
    listenerPosition_ = position;
    listenerForward_ = Normalize(forward, {0, 0, 1});
    listenerRight_ = Normalize(Cross(up, listenerForward_), {1, 0, 0});
}

void AudioMixer::ComputeGains(const Voice& voice, float& left, float& right) const {
    float gain = voice.params.volume * busVolume_[static_cast<int>(voice.params.bus)] * busVolume_[0];
    const int bus = static_cast<int>(voice.params.bus);
    if (duckRemaining_[bus] > 0.0f) gain *= 1.0f - duckAmount_[bus];
    float pan = voice.params.pan;
    if (voice.params.spatial) {
        const Vec3 offset = voice.params.position - listenerPosition_;
        const float distance = Length(offset);
        const float minD = std::max(0.01f, voice.params.minDistance);
        const float maxD = std::max(minD, voice.params.maxDistance);
        float attenuation = distance <= minD ? 1.0f : minD / distance;
        // Fade to silence at maxDistance so voices can be culled.
        attenuation *= 1.0f - SmoothStep(maxD * 0.8f, maxD, distance);
        gain *= attenuation;
        pan = distance > 1.0e-4f ? Clamp(Dot(offset / distance, listenerRight_), -1.0f, 1.0f) : 0.0f;
    }
    // Equal-power panning.
    const float angle = (pan + 1.0f) * 0.25f * kPi;
    left = gain * std::cos(angle);
    right = gain * std::sin(angle);
}

void AudioMixer::Mix(float* stereo, int frames) {
    if (!stereo || frames <= 0) return;
    std::fill(stereo, stereo + static_cast<std::size_t>(frames) * 2u, 0.0f);
    std::lock_guard<std::mutex> lock(mutex_);
    int active = 0;
    for (Voice& voice : voices_) {
        if (!voice.active) continue;
        ++active;
        float left, right;
        ComputeGains(voice, left, right);
        const std::vector<float>& samples = voice.clip->samples;
        const double step = voice.params.pitch * static_cast<double>(voice.clip->sampleRate) / static_cast<double>(sampleRate_);
        const double length = static_cast<double>(samples.size());
        for (int f = 0; f < frames; ++f) {
            if (voice.cursor >= length) {
                if (!voice.params.loop) {
                    voice.active = false;
                    break;
                }
                voice.cursor = std::fmod(voice.cursor, length);
            }
            const std::size_t i0 = static_cast<std::size_t>(voice.cursor);
            const std::size_t i1 = i0 + 1 < samples.size() ? i0 + 1 : (voice.params.loop ? 0 : i0);
            const float t = static_cast<float>(voice.cursor - static_cast<double>(i0));
            const float sample = samples[i0] + (samples[i1] - samples[i0]) * t;
            stereo[f * 2] += sample * left;
            stereo[f * 2 + 1] += sample * right;
            voice.cursor += step;
        }
    }
    // Soft limiter (tanh-like knee above 0.8) keeps the master bus out of clipping.
    float peak = 0.0f;
    for (int i = 0; i < frames * 2; ++i) {
        float s = stereo[i];
        const float magnitude = std::fabs(s);
        if (magnitude > 0.8f) {
            const float over = magnitude - 0.8f;
            s = std::copysign(0.8f + 0.2f * over / (over + 0.2f), s);
            ++stats_.limitedSamples;
        }
        stereo[i] = s;
        peak = std::max(peak, std::fabs(s));
    }
    const float seconds = static_cast<float>(frames) / static_cast<float>(sampleRate_);
    for (int bus = 0; bus < static_cast<int>(Bus::Count); ++bus) duckRemaining_[bus] = std::max(0.0f, duckRemaining_[bus] - seconds);
    stats_.activeVoices = active;
    stats_.peak = peak;
}

MixerStats AudioMixer::Stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

namespace Synth {

namespace {
float Envelope(float t, float duration, float attack, float release) {
    const float a = attack > 0.0f ? Saturate(t / attack) : 1.0f;
    const float r = release > 0.0f ? Saturate((duration - t) / release) : 1.0f;
    return a * r;
}
AudioClip Make(float seconds, int sampleRate) {
    AudioClip clip;
    clip.sampleRate = std::clamp(sampleRate, 8000, 192000);
    clip.samples.assign(static_cast<std::size_t>(std::max(0.0f, std::min(seconds, 30.0f)) * static_cast<float>(clip.sampleRate)), 0.0f);
    return clip;
}
} // namespace

AudioClip Tone(float frequency, float seconds, float attack, float release, int sampleRate) {
    AudioClip clip = Make(seconds, sampleRate);
    for (std::size_t i = 0; i < clip.samples.size(); ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(clip.sampleRate);
        clip.samples[i] = 0.5f * std::sin(kTwoPi * frequency * t) * Envelope(t, seconds, attack, release);
    }
    return clip;
}

AudioClip Whoosh(float seconds, float startCutoff, float endCutoff, std::uint32_t seed, int sampleRate) {
    AudioClip clip = Make(seconds, sampleRate);
    Core::Random random(seed);
    float low = 0.0f, band = 0.0f;
    for (std::size_t i = 0; i < clip.samples.size(); ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(clip.sampleRate);
        const float cutoff = Lerp(startCutoff, endCutoff, t / std::max(seconds, 1.0e-4f));
        // Chamberlin state-variable band-pass filter over white noise.
        const float f = 2.0f * std::sin(kPi * std::min(cutoff, static_cast<float>(clip.sampleRate) * 0.2f) / static_cast<float>(clip.sampleRate));
        const float noise = random.NextFloat() * 2.0f - 1.0f;
        low += f * band;
        const float high = noise - low - 0.6f * band;
        band += f * high;
        const float envelope = std::sin(kPi * Saturate(t / seconds));
        clip.samples[i] = band * 0.9f * envelope;
    }
    return clip;
}

AudioClip Impact(float seconds, float pitch, std::uint32_t seed, int sampleRate) {
    AudioClip clip = Make(seconds, sampleRate);
    Core::Random random(seed);
    float phase = 0.0f;
    for (std::size_t i = 0; i < clip.samples.size(); ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(clip.sampleRate);
        const float frequency = pitch * (1.0f + 3.0f * std::exp(-t * 30.0f));
        phase += kTwoPi * frequency / static_cast<float>(clip.sampleRate);
        const float body = std::sin(phase) * std::exp(-t * 9.0f);
        const float crack = (random.NextFloat() * 2.0f - 1.0f) * std::exp(-t * 60.0f);
        clip.samples[i] = 0.7f * body + 0.35f * crack;
    }
    return clip;
}

AudioClip Chime(float frequency, float seconds, int sampleRate) {
    AudioClip clip = Make(seconds, sampleRate);
    const float partials[4] = {1.0f, 2.76f, 5.4f, 8.93f};
    const float weights[4] = {0.5f, 0.25f, 0.12f, 0.06f};
    for (std::size_t i = 0; i < clip.samples.size(); ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(clip.sampleRate);
        float s = 0.0f;
        for (int p = 0; p < 4; ++p) s += weights[p] * std::sin(kTwoPi * frequency * partials[p] * t) * std::exp(-t * (2.5f + p * 2.0f));
        clip.samples[i] = s * Envelope(t, seconds, 0.002f, 0.05f);
    }
    return clip;
}

AudioClip Drone(float frequency, float seconds, int sampleRate) {
    AudioClip clip = Make(seconds, sampleRate);
    for (std::size_t i = 0; i < clip.samples.size(); ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(clip.sampleRate);
        const float s = std::sin(kTwoPi * frequency * t) + 0.7f * std::sin(kTwoPi * frequency * 1.005f * t)
            + 0.3f * std::sin(kTwoPi * frequency * 2.01f * t);
        clip.samples[i] = 0.2f * s * Envelope(t, seconds, 0.3f, 0.3f);
    }
    return clip;
}

AudioClip Footstep(std::uint32_t seed, int sampleRate) {
    AudioClip clip = Make(0.09f, sampleRate);
    Core::Random random(seed);
    float low = 0.0f;
    for (std::size_t i = 0; i < clip.samples.size(); ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(clip.sampleRate);
        low += 0.25f * ((random.NextFloat() * 2.0f - 1.0f) - low);
        clip.samples[i] = low * std::exp(-t * 55.0f) * 1.4f;
    }
    return clip;
}

} // namespace Synth

bool WriteWav(const std::string& path, const std::vector<float>& stereo, int sampleRate, std::string& error) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        error = "cannot open " + path;
        return false;
    }
    auto u32 = [&file](std::uint32_t v) {
        const char b[4] = {static_cast<char>(v & 0xFF), static_cast<char>((v >> 8) & 0xFF), static_cast<char>((v >> 16) & 0xFF), static_cast<char>((v >> 24) & 0xFF)};
        file.write(b, 4);
    };
    auto u16 = [&file](std::uint16_t v) {
        const char b[2] = {static_cast<char>(v & 0xFF), static_cast<char>((v >> 8) & 0xFF)};
        file.write(b, 2);
    };
    const std::uint32_t dataBytes = static_cast<std::uint32_t>(stereo.size() * 2);
    file.write("RIFF", 4);
    u32(36 + dataBytes);
    file.write("WAVEfmt ", 8);
    u32(16);
    u16(1);
    u16(2);
    u32(static_cast<std::uint32_t>(sampleRate));
    u32(static_cast<std::uint32_t>(sampleRate) * 4u);
    u16(4);
    u16(16);
    file.write("data", 4);
    u32(dataBytes);
    for (float s : stereo) {
        const int v = static_cast<int>(std::lround(Clamp(std::isfinite(s) ? s : 0.0f, -1.0f, 1.0f) * 32767.0f));
        u16(static_cast<std::uint16_t>(static_cast<std::int16_t>(v)));
    }
    if (!file) {
        error = "failed writing " + path;
        return false;
    }
    return true;
}

} // namespace Astral::Audio
