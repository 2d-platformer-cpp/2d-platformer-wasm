#include "Engine/AudioManager.hpp"
#include <iostream>

namespace Engine {

SDL_AudioDeviceID AudioManager::openPlaybackDevice() {
    int count = 0;
    SDL_AudioDeviceID *devices = SDL_GetAudioPlaybackDevices(&count);

    std::cout << "AudioManager: Found " << count << " playback device(s)" << std::endl;
    for (int i = 0; i < count; i++) {
        const char *name = SDL_GetAudioDeviceName(devices[i]);
        std::cout << "  [" << i << "] " << (name ? name : "?") << std::endl;
    }

    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (dev) {
        const char *name = SDL_GetAudioDeviceName(dev);
        std::cout << "AudioManager: Opened default device" << (name ? (": " + std::string(name)).c_str() : "") << std::endl;
        SDL_free(devices);
        return dev;
    }
    std::cout << "AudioManager: Default device failed: " << SDL_GetError() << std::endl;

    for (int i = 0; i < count; i++) {
        dev = SDL_OpenAudioDevice(devices[i], NULL);
        if (dev) {
            const char *name = SDL_GetAudioDeviceName(devices[i]);
            std::cout << "AudioManager: Opened device [" << i << "] " << (name ? name : "?") << std::endl;
            SDL_free(devices);
            return dev;
        }
        std::cout << "AudioManager: Device [" << i << "] failed: " << SDL_GetError() << std::endl;
    }

    SDL_free(devices);
    std::cout << "AudioManager: No playback device available" << std::endl;
    return 0;
}

AudioManager::AudioManager() {
    m_device = openPlaybackDevice();
    loadAllSFX();
}

AudioManager::~AudioManager() {
    stopAll();
    for (auto &[id, data] : m_sfxData) {
        freeSound(data);
    }
}

bool AudioManager::loadWAV(const std::string &path, SoundData &sound) {
    if (!SDL_LoadWAV(path.c_str(), &sound.spec, &sound.buffer, &sound.length)) {
        std::cout << "AudioManager: Failed to load " << path << ": " << SDL_GetError() << std::endl;
        return false;
    }
    return true;
}

void AudioManager::freeSound(SoundData &sound) {
    if (sound.buffer) {
        SDL_free(sound.buffer);
        sound.buffer = nullptr;
    }
}

void AudioManager::loadAllSFX() {
    struct SFXPath {
        Common::SFX id;
        const char *path;
    };

    static const SFXPath paths[] = {
        {Common::SFX::RUN_LOOP,         "assets/audio/player/run_loop.wav"},
        {Common::SFX::LANDING,          "assets/audio/player/landing.wav"},
        {Common::SFX::DEATH_SCREAM,     "assets/audio/player/death_scream.wav"},
        {Common::SFX::RESPAWN,          "assets/audio/player/respawn.wav"},
        {Common::SFX::IDLE_BREATHING,   "assets/audio/player/idle_breathing.wav"},
        {Common::SFX::HEAVY_BREATHING,  "assets/audio/player/heavy_breathing.wav"},
        {Common::SFX::WIND_GRASS,       "assets/audio/ambience/wind_grass.wav"},
        {Common::SFX::BIRDS_CHIRP,      "assets/audio/ambience/birds_chirp.wav"},
        {Common::SFX::LAVA,             "assets/audio/ambience/lava.wav"},
    };

    for (const auto &sfx : paths) {
        SoundData data;
        if (loadWAV(sfx.path, data)) {
            m_sfxData[sfx.id] = std::move(data);
        }
    }
}

SDL_AudioStream *AudioManager::createStream(const SoundData &sound) {
    SDL_AudioSpec deviceSpec;
    int sampleFrames = 0;
    if (!SDL_GetAudioDeviceFormat(m_device, &deviceSpec, &sampleFrames)) {
        return nullptr;
    }

    SDL_AudioStream *stream = SDL_CreateAudioStream(&sound.spec, &deviceSpec);
    if (!stream) return nullptr;

    if (!SDL_PutAudioStreamData(stream, sound.buffer, static_cast<int>(sound.length))) {
        SDL_DestroyAudioStream(stream);
        return nullptr;
    }

    return stream;
}

void AudioManager::playSFX(Common::SFX id) {
    if (!m_device) return;

    auto it = m_sfxData.find(id);
    if (it == m_sfxData.end() || !it->second.buffer) return;

    SDL_AudioStream *stream = createStream(it->second);
    if (!stream) return;

    if (!SDL_BindAudioStream(m_device, stream)) {
        SDL_DestroyAudioStream(stream);
        return;
    }

    m_activeStreams.push_back(stream);
}

void AudioManager::startLoop(Common::SFX id) {
    if (!m_device) return;
    if (m_loopingStreams.count(id)) return;

    auto it = m_sfxData.find(id);
    if (it == m_sfxData.end() || !it->second.buffer) return;

    SDL_AudioStream *stream = createStream(it->second);
    if (!stream) return;

    if (!SDL_BindAudioStream(m_device, stream)) {
        SDL_DestroyAudioStream(stream);
        return;
    }

    m_loopingStreams[id] = stream;
}

void AudioManager::stopLoop(Common::SFX id) {
    auto it = m_loopingStreams.find(id);
    if (it == m_loopingStreams.end()) return;

    SDL_UnbindAudioStream(it->second);
    SDL_DestroyAudioStream(it->second);
    m_loopingStreams.erase(it);
}

void AudioManager::stopAll() {
    for (auto &stream : m_activeStreams) {
        SDL_UnbindAudioStream(stream);
        SDL_DestroyAudioStream(stream);
    }
    m_activeStreams.clear();

    for (auto &[id, stream] : m_loopingStreams) {
        SDL_UnbindAudioStream(stream);
        SDL_DestroyAudioStream(stream);
    }
    m_loopingStreams.clear();
}

void AudioManager::update() {
    m_activeStreams.erase(
        std::remove_if(m_activeStreams.begin(), m_activeStreams.end(),
            [](SDL_AudioStream *stream) {
                Sint32 queued = SDL_GetAudioStreamQueued(stream);
                if (queued <= 0) {
                    SDL_UnbindAudioStream(stream);
                    SDL_DestroyAudioStream(stream);
                    return true;
                }
                return false;
            }),
        m_activeStreams.end());

    for (auto &[id, stream] : m_loopingStreams) {
        auto it = m_sfxData.find(id);
        if (it == m_sfxData.end() || !it->second.buffer) continue;

        Sint32 queued = SDL_GetAudioStreamQueued(stream);
        if (queued >= 0 && static_cast<Uint32>(queued) < it->second.length / 2) {
            SDL_PutAudioStreamData(stream, it->second.buffer,
                                   static_cast<int>(it->second.length));
        }
    }
}

} // namespace Engine
