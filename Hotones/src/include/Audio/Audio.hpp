#pragma once

// ── Hotones::Audio — C++ convenience API ─────────────────────────────────────
//
// Mirrors the Lua `audio.*` table exactly.
//
// Usage:
//   #include <Audio/Audio.hpp>
//
//   Hotones::Audio::LoadSound("shoot", "assets/sounds/shoot.wav");
//   Hotones::Audio::Play("shoot");
//   Hotones::Audio::SetVolume(75);

#include <SFX/AudioSystem.hpp>
#include <string>

namespace Hotones::Audio {

/// Load a sound file and register it under `name`. Multiple files under the
/// same name form a playback group (random/sequential selection).
inline bool LoadSound(const std::string& name, const std::string& path)
{
    return Ho_tones::GetSoundBus().LoadSoundFile(name, path);
}

/// Play the first loaded variant for `name`.
inline bool Play(const std::string& name, float gain = 1.0f)
{
    return Ho_tones::GetSoundBus().PlayLoaded(name, gain);
}

/// Play a random variant from the group for `name`.
inline bool PlayRandom(const std::string& name, float gain = 1.0f)
{
    return Ho_tones::GetSoundBus().PlayRandom(name, gain);
}

/// Play variants in round-robin order (good for footsteps, impacts etc.).
inline bool PlaySequential(const std::string& name, float gain = 1.0f)
{
    return Ho_tones::GetSoundBus().PlaySequential(name, gain);
}

/// Like PlaySequential but each call starts an independent overlapping voice.
inline bool PlaySequentialAsync(const std::string& name, float gain = 1.0f)
{
    return Ho_tones::GetSoundBus().PlaySequentialAsync(name, gain);
}

/// Master volume, range 0–100.
inline void SetVolume(int vol)  { Ho_tones::GetSoundBus().SetVolume(vol); }
inline int  GetVolume()         { return Ho_tones::GetSoundBus().GetVolume(); }

/// Stop every currently playing voice immediately.
inline void StopAll()           { Ho_tones::GetSoundBus().StopAll(); }

} // namespace Hotones::Audio
