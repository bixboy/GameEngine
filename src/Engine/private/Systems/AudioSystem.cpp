#include "Systems/AudioSystem.h"
#include "Logger.h"
#include "Core/public/Math/Vector3.h"

// Include miniaudio header
// We assume MINIAUDIO_IMPLEMENTATION is defined in a dedicated file (e.g. miniaudio.cpp)
// or we define it here if it's not. 
// The user said "The user has 1 active workspaces... miniaudio.cpp (LANGUAGE_CPP)".
// So miniaudio.cpp likely handles the implementation.
#include <miniaudio.h>

namespace BixEngine::Systems
{
    struct AudioSystem::Impl
    {
        ma_engine engine;
        bool initialized = false;
    };

    AudioSystem& AudioSystem::Get()
    {
        static AudioSystem instance;
        return instance;
    }

    AudioSystem::AudioSystem() : impl_(std::make_unique<Impl>())
    {
    }

    AudioSystem::~AudioSystem()
    {
        Shutdown();
    }

    bool AudioSystem::Initialize()
    {
        if (impl_->initialized)
            return true;

        ma_result result = ma_engine_init(NULL, &impl_->engine);
        if (result != MA_SUCCESS)
        {
            LOG_ERROR("Failed to initialize audio engine.");
            return false;
        }

        impl_->initialized = true;
        LOG_INFO("AudioSystem initialized successfully. Engine ptr: " + std::to_string((uintptr_t)&impl_->engine));
        
        // Set master volume to 1.0 just in case
        ma_engine_set_volume(&impl_->engine, 1.0f);
        
        return true;
    }

    void AudioSystem::Shutdown()
    {
        if (impl_->initialized)
        {
            ma_engine_uninit(&impl_->engine);
            impl_->initialized = false;
        }
    }

    void AudioSystem::UpdateListener(const Math::Vector3& pos, const Math::Vector3& fwd, const Math::Vector3& up)
    {
        if (!impl_->initialized)
            return;

        ma_engine_listener_set_position(&impl_->engine, 0, pos.x, pos.y, pos.z);
        ma_engine_listener_set_direction(&impl_->engine, 0, fwd.x, fwd.y, fwd.z);
        ma_engine_listener_set_world_up(&impl_->engine, 0, up.x, up.y, up.z);
    }

    void AudioSystem::PlaySound(const String& filePath)
    {
        if (!impl_->initialized)
            return;

        ma_engine_play_sound(&impl_->engine, filePath.c_str(), NULL);
    }

    void* AudioSystem::GetEngineHandle() const
    {
        return impl_ ? &impl_->engine :
        nullptr;
    }

    ma_engine* AudioSystem::GetEngine() const
    {
        return impl_ ? &impl_->engine : nullptr;
    }
    

}
