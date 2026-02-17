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

    void PlaySound(const std::string &soundName);

    // Volume in 0-100
    int SetVolume(int newVolume);
    int GetVolume() const;

    // Legacy placeholder
    // Play a sound by filename (wav/ogg). `soundName` is a filesystem path.
    bool PlaySound(const std::string& soundPath, float gain = 1.0f);

    // Load a sound file and associate it under a logical group name. Multiple
    // files can be loaded under the same `name` to form variants.
    // Returns true on success.
    bool LoadSoundFile(const std::string& name, const std::string& filePath);

    // Play a previously loaded sound by name. Plays the first loaded variant.
    bool PlayLoaded(const std::string& name, float gain = 1.0f);

    // Play a random variant from the loaded group for `name`.
    bool PlayRandom(const std::string& name, float gain = 1.0f);

    // Play the next variant in the loaded group for `name` in a round-robin
    // fashion. This ensures each file in the category is played in turn.
    bool PlaySequential(const std::string& name, float gain = 1.0f);

    // Async/overlapping version of PlaySequential: will load a fresh copy of
    // the selected variant and play it immediately so multiple footsteps can
    // overlap. Returns true if playback started.
    bool PlaySequentialAsync(const std::string& name, float gain = 1.0f);
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
