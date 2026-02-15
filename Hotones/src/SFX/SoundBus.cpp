#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>
#include <SoundBus.hpp>
#include <raylib.h>

namespace Ho_tones {

    // Keep track of raylib-created Sounds and their temp filenames so we can stop/unload them from StopAll().
    static std::vector<Sound> raylibSounds;
    static std::vector<std::string> raylibTempFiles;

    struct SoundBus::Voice {
        std::vector<int16_t> samples; // interleaved
        int sampleRate = 44100;
        int channels = 1;
        float gain = 1.0f; // per-voice
        float playbackPos = 0.0f; // in frames
        size_t frameCount() const { return samples.empty() ? 0 : samples.size() / channels; }
    };

    int SoundBus::SetVolume(int newVolume) {
        volume = std::max(0, std::min(100, newVolume));
        return volume;
    }

    int SoundBus::GetVolume() const {
        return volume;
    }

    SoundBus::SoundBus() : volume(100) {
    }

    SoundBus::~SoundBus() {
        StopAll();
    }

    void SoundBus::PlaySound(const std::string& soundName) {
        std::cout << "Playing sound: " << soundName << std::endl;
    }

    void SoundBus::PlayPCM(const std::vector<int16_t>& data, int sampleRate, int channels, float gain) {
        if (data.empty() || sampleRate <= 0 || channels <= 0) return;
        auto v = std::make_unique<Voice>();
        v->samples = data;
        v->sampleRate = sampleRate;
        v->channels = channels;
        v->gain = gain;
        v->playbackPos = 0.0f;

        std::lock_guard<std::mutex> lk(voicesMutex);
        voices.push_back(std::move(v));
    }

    void SoundBus::PlayPCMViaRaylib(const std::vector<int16_t>& data, int sampleRate, int channels, float gain) {
        if (data.empty() || sampleRate <= 0 || channels <= 0) return;
        if (!IsAudioDeviceReady()) return;

        // Write a temporary WAV file and use raylib's LoadSound to load/play it.
        try {
            namespace fs = std::filesystem;
            fs::path tmpdir = fs::temp_directory_path();
            auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            std::ostringstream name;
            name << "hotones_sound_" << now << ".wav";
            fs::path tmpfile = tmpdir / name.str();

            // WAV header fields
            uint32_t sampleCount = (uint32_t)data.size(); // samples (frames * channels)
            uint16_t bitsPerSample = 16;
            uint16_t numChannels = (uint16_t)channels;
            uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
            uint16_t blockAlign = numChannels * bitsPerSample / 8;
            uint32_t dataBytes = sampleCount * sizeof(int16_t);

            std::ofstream ofs(tmpfile, std::ios::binary);
            if (!ofs) return;

            // RIFF header
            ofs.write("RIFF", 4);
            uint32_t chunkSize = 36 + dataBytes;
            ofs.write(reinterpret_cast<const char*>(&chunkSize), 4);
            ofs.write("WAVE", 4);

            // fmt subchunk
            ofs.write("fmt ", 4);
            uint32_t subchunk1Size = 16;
            ofs.write(reinterpret_cast<const char*>(&subchunk1Size), 4);
            uint16_t audioFormat = 1; // PCM
            ofs.write(reinterpret_cast<const char*>(&audioFormat), 2);
            ofs.write(reinterpret_cast<const char*>(&numChannels), 2);
            ofs.write(reinterpret_cast<const char*>(&sampleRate), 4);
            ofs.write(reinterpret_cast<const char*>(&byteRate), 4);
            ofs.write(reinterpret_cast<const char*>(&blockAlign), 2);
            ofs.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

            // data subchunk
            ofs.write("data", 4);
            ofs.write(reinterpret_cast<const char*>(&dataBytes), 4);
            ofs.write(reinterpret_cast<const char*>(data.data()), dataBytes);
            ofs.close();

            Sound s = LoadSound(tmpfile.string().c_str());
            SetSoundVolume(s, gain);
            ::PlaySound(s);

            raylibSounds.push_back(s);
            raylibTempFiles.push_back(tmpfile.string());
        } catch (...) {
            return;
        }
    }

    void SoundBus::MixInto(float* output, size_t frames, int outSampleRate, int outChannels) {
        if (!output || frames == 0 || outChannels <= 0 || outSampleRate <= 0) return;

        // zero the output
        size_t outSamples = frames * (size_t)outChannels;
        for (size_t i = 0; i < outSamples; ++i) output[i] = 0.0f;

        std::lock_guard<std::mutex> lk(voicesMutex);
        if (voices.empty()) return;

        float busGain = volume / 100.0f;

        // Mix each voice
        for (auto it = voices.begin(); it != voices.end();) {
            Voice* voice = it->get();
            size_t vFrames = voice->frameCount();
            if (vFrames == 0) {
                it = voices.erase(it);
                continue;
            }

            float rateRatio = (float)voice->sampleRate / (float)outSampleRate;

            bool finished = false;

            for (size_t f = 0; f < frames; ++f) {
                size_t outBase = f * outChannels;
                size_t srcFrame = static_cast<size_t>(voice->playbackPos);
                if (srcFrame >= vFrames) { finished = true; break; }

                for (int c = 0; c < outChannels; ++c) {
                    // pick appropriate source channel (if fewer channels, duplicate last channel)
                    int srcChan = c < voice->channels ? c : (voice->channels - 1);
                    size_t srcIdx = srcFrame * voice->channels + srcChan;
                    int16_t s = voice->samples[srcIdx];
                    float fs = (float)s / 32768.0f;
                    output[outBase + c] += fs * voice->gain * busGain;
                }

                voice->playbackPos += rateRatio;
            }

            if (finished || static_cast<size_t>(voice->playbackPos) >= vFrames) {
                it = voices.erase(it);
            } else {
                ++it;
            }
        }
    }

    void SoundBus::StopAll() {
        std::lock_guard<std::mutex> lk(voicesMutex);
        voices.clear();

        // Stop and unload any raylib Sounds we created, and remove temp files
        for (auto &s : raylibSounds) {
            StopSound(s);
            UnloadSound(s);
        }
        raylibSounds.clear();

        for (auto &p : raylibTempFiles) {
            // best-effort remove
            std::error_code ec;
            std::filesystem::remove(p, ec);
        }
        raylibTempFiles.clear();
    }

}
