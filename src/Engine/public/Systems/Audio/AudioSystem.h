#pragma once

#include "Core/public/Containers/String.h"
#include "Core/public/Math/Vector3.h"


struct ma_engine;

namespace BixEngine::Systems {

    class AudioSystem {
    public:
        static AudioSystem& Get();
        AudioSystem();
        ~AudioSystem();

        bool Initialize();
        void Shutdown();
        void UpdateListener(const Math::Vector3& pos, const Math::Vector3& fwd, const Math::Vector3& up);
        void PlaySound(const String& filePath);
        void PlaySound(const String& filePath, float volume, float pitch);
        void PlayMusic(const String& filePath);
        void Pause();
        void Stop();
        void SetGlobalVolume(float volume);
        float GetMusicDuration() const;
        float GetMusicCursor() const;
        void* GetEngineHandle() const;
        
        ma_engine* GetEngine() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
