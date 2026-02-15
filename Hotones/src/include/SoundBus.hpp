#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <mutex>

namespace Ho_tones {

class SoundBus {
public:
    SoundBus();
    ~SoundBus();

    // Volume in 0-100
    int SetVolume(int newVolume);
    int GetVolume() const;

    // Legacy placeholder
    void PlaySound(const std::string& soundName);

    // Play raw PCM interleaved 16-bit signed samples.
    // `data` is interleaved PCM (frames * channels).
    // `sampleRate` is samples per second of the data.
    // `channels` is number of channels in `data` (1 or 2 typically).
    // `gain` is a per-voice multiplier (1.0 = unchanged).
    void PlayPCM(const std::vector<int16_t>& data, int sampleRate, int channels, float gain = 1.0f);

    // Create a raylib `Wave`/`Sound` from provided interleaved int16 PCM and play it immediately.
    // This requires raylib audio to be initialized (InitAudioDevice()).
    // The SoundBus will keep track of the created Sound and will unload it on StopAll()/destructor.
    void PlayPCMViaRaylib(const std::vector<int16_t>& data, int sampleRate, int channels, float gain = 1.0f);

    // Mix active voices into `output`. `output` is a float buffer
    // with `frames * outChannels` elements, in interleaved float samples
    // with range roughly -1.0..1.0.
    void MixInto(float* output, size_t frames, int outSampleRate, int outChannels);

    // Stop all currently playing voices.
    void StopAll();

private:
    struct Voice;
    std::vector<std::unique_ptr<Voice>> voices;
    std::mutex voicesMutex;
    int volume; // 0-100
};

} // namespace Ho_tones
