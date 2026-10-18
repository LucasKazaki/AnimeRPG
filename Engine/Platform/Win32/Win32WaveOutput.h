#pragma once

#include "Engine/Audio/AudioMixer.h"

#include <windows.h>
#include <mmsystem.h>

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

namespace Astral::Platform {

// waveOut stream fed by an engine mixer on its own thread. Construction
// failure leaves it inactive (silent). The mixer must outlive this adapter.
class Win32WaveOutput {
public:
    explicit Win32WaveOutput(Audio::AudioMixer& mixer);
    ~Win32WaveOutput();
    Win32WaveOutput(const Win32WaveOutput&) = delete;
    Win32WaveOutput& operator=(const Win32WaveOutput&) = delete;
    bool Active() const { return device_ != nullptr; }

private:
    static constexpr int kBufferCount = 4;
    static constexpr int kFramesPerBuffer = 1024; // ~21 ms at 48 kHz
    void Fill(int index);
    void Pump();

    Audio::AudioMixer& mixer_;
    HWAVEOUT device_{};
    HANDLE event_{};
    WAVEHDR headers_[kBufferCount]{};
    std::vector<std::int16_t> pcm_[kBufferCount];
    std::vector<float> scratch_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

} // namespace Astral::Platform
