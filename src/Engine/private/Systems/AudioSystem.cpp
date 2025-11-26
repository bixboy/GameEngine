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
        ma_sound musicSound;
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

        ma_result result = ma_engine_init(nullptr, &impl_->engine);
        if (result != MA_SUCCESS)
            return false;

        impl_->initialized = true;
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

    void AudioSystem::PlayMusic(const String& filePath)
    {
        if (!impl_->initialized)
            return;

        // Stop existing music if any
        if (ma_sound_is_playing(&impl_->musicSound))
        {
            ma_sound_stop(&impl_->musicSound);
            ma_sound_uninit(&impl_->musicSound);
        }

        // Initialize and play new sound as music (streamed)
        ma_result result = ma_sound_init_from_file(&impl_->engine, filePath.c_str(), MA_SOUND_FLAG_STREAM, NULL, NULL, &impl_->musicSound);
        if (result == MA_SUCCESS)
        {
            ma_sound_start(&impl_->musicSound);
        }
        else
        {
            LOG_ERROR("Failed to load music: %s", filePath.c_str());
        }
    }

    void AudioSystem::Pause()
    {
        if (!impl_->initialized)
            return;

        if (ma_sound_is_playing(&impl_->musicSound))
        {
            ma_sound_stop(&impl_->musicSound);
        }
        else
        {
            ma_sound_start(&impl_->musicSound);
        }
    }

    void AudioSystem::Stop()
    {
        if (!impl_->initialized)
            return;

        if (ma_sound_is_playing(&impl_->musicSound))
        {
            ma_sound_stop(&impl_->musicSound);
            ma_sound_seek_to_pcm_frame(&impl_->musicSound, 0);
        }
    }

    void AudioSystem::SetGlobalVolume(float volume)
    {
        if (!impl_->initialized)
            return;

        ma_engine_set_volume(&impl_->engine, volume);
    }

    float AudioSystem::GetMusicDuration() const
    {
        if (!impl_->initialized)
            return 0.0f;

        float length;
        if (ma_sound_get_length_in_seconds(&impl_->musicSound, &length) == MA_SUCCESS)
        {
            return length;
        }
        return 0.0f;
    }

    float AudioSystem::GetMusicCursor() const
    {
        if (!impl_->initialized)
            return 0.0f;

        float cursor;
        if (ma_sound_get_cursor_in_seconds(&impl_->musicSound, &cursor) == MA_SUCCESS)
        {
            return cursor;
        }
        return 0.0f;
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
