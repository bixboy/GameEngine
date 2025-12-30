#pragma once
#include "Containers/String.h"
#include "Math/Vector3.h"
#include "Ressources/RessourcesClass/AudioClip.h"

struct ma_engine;

namespace BixEngine::Systems
{
    class AudioSystem
    {
    public:
        static AudioSystem& Get();

        AudioSystem();
        ~AudioSystem();

        AudioSystem(const AudioSystem&) = delete;
        AudioSystem& operator=(const AudioSystem&) = delete;

        // --- Cycle de vie ---
        
        bool Initialize();
        void Shutdown();

        // --- Runtime ---
        
        void UpdateListener(const Math::Vector3& pos, const Math::Vector3& fwd, const Math::Vector3& up);

        void PlaySound(const String& filePath);
        void PlaySound(const String& filePath, float volume, float pitch);
        void PlaySound(std::shared_ptr<Resources::AudioClip> clip, float volume = 1.0f, float pitch = 1.0f);
        
        void PlayMusic(const String& filePath);
        
        void Pause();
        void Stop();
        
        void SetGlobalVolume(float volume);
        
        // --- Getters ---
        
        float GetMusicDuration() const;
        float GetMusicCursor() const;
        
        void* GetEngineHandle() const;
        ma_engine* GetEngine() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}