#include <iostream>
#include <vector>
#include <cmath>
#include <SoundBus.hpp>
#include <raylib.h>
#include <thread>
#include <chrono>

// int main() {
//     const int sampleRate = 44100;
//     const int channels = 2;
//     const float durationSeconds = 3.0f;
//     const float freq = 440.0f;

//     InitWindow(200, 100, "Raylib SoundBus Example");
//     InitAudioDevice();

//     size_t frames = (size_t)(sampleRate * durationSeconds);
//     std::vector<int16_t> data(frames * channels);

//     for (size_t i = 0; i < frames; ++i) {
//         float t = (float)i / (float)sampleRate;
//         float s = sinf(2.0f * 3.14159265358979323846f * freq * t) * 0.5f;
//         int16_t val = (int16_t)std::lround(s * 32767.0f);
//         for (int c = 0; c < channels; ++c) data[i * channels + c] = val;
//     }

//     Ho_tones::SoundBus bus;
//     bus.PlayPCMViaRaylib(data, sampleRate, channels, 1.0f);

//     double start = GetTime();
//     while (!WindowShouldClose() && (GetTime() - start) < durationSeconds + 0.5) {
//         BeginDrawing();
//         ClearBackground(RAYWHITE);
//         DrawText("Playing sine via SoundBus", 10, 40, 10, BLACK);
//         EndDrawing();
//         // small sleep
//         std::this_thread::sleep_for(std::chrono::milliseconds(10));
//     }

//     // cleanup
//     bus.StopAll();
//     CloseAudioDevice();
//     CloseWindow();
//     return 0;
// }
