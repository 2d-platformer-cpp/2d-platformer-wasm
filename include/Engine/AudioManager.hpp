/// @file AudioManager.hpp
/// SDL_Audio wrapper for one-shot SFX playback and looping audio streams.
/// Opens the default playback device, loads all WAV files on construction,
/// and manages active / looping audio streams.

#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "Common/SFX.hpp"

namespace Engine {

/// SDL_Audio wrapper for one-shot SFX playback and looping audio streams.
/// Opens the default playback device, loads all WAV files on construction,
/// and manages active / looping audio streams.
class AudioManager {
public:
    /// Opens the default audio device and loads all WAV files.
    AudioManager();
    /// Closes the audio device and frees all resources.
    ~AudioManager();

    AudioManager(const AudioManager &) = delete;
    AudioManager &operator=(const AudioManager &) = delete;

    /// Plays a sound effect once.
    /// The stream is reaped automatically after playback completes.
    /// @param id  The SFX identifier to play.
    void playSFX(Common::SFX id);

    /// Starts looping a sound effect continuously.
    /// Only one loop per SFX ID can be active at a time.
    /// @param id  The SFX identifier to loop.
    void startLoop(Common::SFX id);

    /// Stops a currently looping sound effect.
    /// @param id  The SFX identifier to stop.
    void stopLoop(Common::SFX id);

    /// Stops all active one-shot and looping streams immediately.
    void stopAll();

    /// Per-frame maintenance: reaps finished one-shot streams.
    /// Called once per frame from Game::update.
    void update();

private:
    /// Stores raw PCM data and format info for a single WAV file.
    struct SoundData {
        Uint8 *buffer = nullptr;  ///< Raw PCM sample data.
        Uint32 length = 0;        ///< Buffer size in bytes.
        SDL_AudioSpec spec;       ///< Audio format descriptor.
    };

    SDL_AudioDeviceID m_device = 0;                                 ///< Opened playback device.
    std::unordered_map<Common::SFX, SoundData> m_sfxData;           ///< All loaded SFX samples.
    std::vector<SDL_AudioStream *> m_activeStreams;                 ///< One-shot streams yet to finish.
    std::unordered_map<Common::SFX, SDL_AudioStream *> m_loopingStreams;  ///< Active looping streams.

    bool loadWAV(const std::string &path, SoundData &sound);        ///< Load a WAV into SoundData.
    void freeSound(SoundData &sound);                               ///< Free PCM buffer.
    void loadAllSFX();                                              ///< Load every WAV in the SFX enum.
    SDL_AudioDeviceID openPlaybackDevice();                         ///< Open default audio device.
    SDL_AudioStream *createStream(const SoundData &sound);          ///< Bind SoundData to a new stream.
};

} // namespace Engine
