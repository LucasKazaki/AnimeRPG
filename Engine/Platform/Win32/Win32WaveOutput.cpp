#include "Engine/Platform/Win32/Win32WaveOutput.h"

#include <algorithm>
#include <cmath>

namespace Astral::Platform {

Win32WaveOutput::Win32WaveOutput(Audio::AudioMixer& mixer) : mixer_(mixer) {
    event_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!event_) return;
    WAVEFORMATEX format{};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 2;
    format.nSamplesPerSec = static_cast<DWORD>(mixer.SampleRate());
    format.wBitsPerSample = 16;
    format.nBlockAlign = static_cast<WORD>(format.nChannels * format.wBitsPerSample / 8);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    if (waveOutOpen(&device_, WAVE_MAPPER, &format, reinterpret_cast<DWORD_PTR>(event_), 0, CALLBACK_EVENT)
        != MMSYSERR_NOERROR) {
        device_ = nullptr;
        CloseHandle(event_);
        event_ = nullptr;
        return;
    }
    scratch_.assign(static_cast<std::size_t>(kFramesPerBuffer) * 2u, 0.0f);
    int prepared = 0;
    for (; prepared < kBufferCount; ++prepared) {
        std::vector<std::int16_t>& pcm = pcm_[prepared];
        pcm.assign(static_cast<std::size_t>(kFramesPerBuffer) * 2u, 0);
        WAVEHDR& header = headers_[prepared];
        header.lpData = reinterpret_cast<LPSTR>(pcm.data());
        header.dwBufferLength = static_cast<DWORD>(pcm.size() * sizeof(std::int16_t));
        if (waveOutPrepareHeader(device_, &header, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) break;
    }
    bool started = prepared == kBufferCount;
    if (started) {
        running_ = true;
        for (int index = 0; index < kBufferCount; ++index) Fill(index);
        try {
            thread_ = std::thread([this] { Pump(); });
        } catch (...) {
            started = false;
        }
    }
    if (!started) {
        running_ = false;
        waveOutReset(device_);
        for (int index = 0; index < prepared; ++index) waveOutUnprepareHeader(device_, &headers_[index], sizeof(WAVEHDR));
        waveOutClose(device_);
        device_ = nullptr;
        CloseHandle(event_);
        event_ = nullptr;
    }
}

Win32WaveOutput::~Win32WaveOutput() {
    if (!device_) return;
    running_ = false;
    SetEvent(event_);
    if (thread_.joinable()) thread_.join();
    waveOutReset(device_);
    for (WAVEHDR& header : headers_) waveOutUnprepareHeader(device_, &header, sizeof(WAVEHDR));
    waveOutClose(device_);
    CloseHandle(event_);
}

void Win32WaveOutput::Fill(int index) {
    mixer_.Mix(scratch_.data(), kFramesPerBuffer);
    std::vector<std::int16_t>& pcm = pcm_[index];
    for (std::size_t sample = 0; sample < scratch_.size(); ++sample) {
        const float value = std::isfinite(scratch_[sample]) ? std::clamp(scratch_[sample], -1.0f, 1.0f) : 0.0f;
        pcm[sample] = static_cast<std::int16_t>(std::lround(value * 32767.0f));
    }
    waveOutWrite(device_, &headers_[index], sizeof(WAVEHDR));
}

void Win32WaveOutput::Pump() {
    while (running_.load()) {
        WaitForSingleObject(event_, 100);
        for (int index = 0; index < kBufferCount && running_.load(); ++index) {
            const volatile DWORD& flags = headers_[index].dwFlags; // written by the driver
            if ((flags & WHDR_DONE) != 0) Fill(index);
        }
    }
}

} // namespace Astral::Platform
