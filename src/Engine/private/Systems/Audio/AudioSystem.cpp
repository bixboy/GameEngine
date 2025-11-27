#include "Systems/Audio/AudioSystem.h"
#include "Debug/Logger.h"
#include "Core/public/Math/Vector3.h"
#include <miniaudio.h>
#include <vector>

namespace BixEngine::Systems
{
    struct AudioSystem::Impl
    {
        ma_engine engine;
        ma_sound musicSound;
        bool initialized = false;
        bool musicInitialized = false;
        String currentMusicPath;
        std::vector<ma_sound*> activeSounds;
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
            if (impl_->musicInitialized)
            {
                ma_sound_uninit(&impl_->musicSound);
                impl_->musicInitialized = false;
            }
            
            // Cleanup active sounds
            for (auto* sound : impl_->activeSounds)
            {
                ma_sound_uninit(sound);
                delete sound;
            }
            impl_->activeSounds.clear();

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

        // Cleanup finished sounds
        for (auto it = impl_->activeSounds.begin(); it != impl_->activeSounds.end(); )
        {
            if (!ma_sound_is_playing(*it))
            {
                ma_sound_uninit(*it);
                delete *it;
                it = impl_->activeSounds.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void AudioSystem::PlaySound(const String& filePath)
    {
        PlaySound(filePath, 1.0f, 1.0f);
    }

    void AudioSystem::PlaySound(const String& filePath, float volume, float pitch)
    {
        if (!impl_->initialized)
            return;

        if (volume == 1.0f && pitch == 1.0f)
        {
            ma_engine_play_sound(&impl_->engine, filePath.c_str(), NULL);
            return;
        }

        ma_sound* sound = new ma_sound;
        ma_result result = ma_sound_init_from_file(&impl_->engine, filePath.c_str(), MA_SOUND_FLAG_DECODE, NULL, NULL, sound);
        if (result == MA_SUCCESS)
        {
            ma_sound_set_volume(sound, volume);
            ma_sound_set_pitch(sound, pitch);
            ma_sound_start(sound);
            impl_->activeSounds.push_back(sound);
        }
        else
        {
            delete sound;
        }
    }

    void AudioSystem::PlayMusic(const String& filePath)
    {
        if (!impl_->initialized)
            return;

        if (impl_->musicInitialized && impl_->currentMusicPath == filePath)
        {
            ma_sound_start(&impl_->musicSound);
            return;
        }

        if (impl_->musicInitialized)
        {
            ma_sound_stop(&impl_->musicSound);
            ma_sound_uninit(&impl_->musicSound);
            impl_->musicInitialized = false;
        }

        ma_result result = ma_sound_init_from_file(&impl_->engine, filePath.c_str(), MA_SOUND_FLAG_STREAM, NULL, NULL, &impl_->musicSound);
        if (result == MA_SUCCESS)
        {
            ma_sound_start(&impl_->musicSound);
            impl_->musicInitialized = true;
            impl_->currentMusicPath = filePath;
        }
        else
        {
            LOG_ERROR("Failed to load music: %s", filePath.c_str());
        }
    }

    void AudioSystem::Pause()
    {
        if (!impl_->initialized || !impl_->musicInitialized)
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
        if (!impl_->initialized || !impl_->musicInitialized)
            return;

        if (ma_sound_is_playing(&impl_->musicSound))
        {
            ma_sound_stop(&impl_->musicSound);
            ma_sound_seek_to_pcm_frame(&impl_->musicSound, 0);
        }
        else
        {
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
        if (!impl_->initialized || !impl_->musicInitialized)
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
        if (!impl_->initialized || !impl_->musicInitialized)
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
        return impl_ ? &impl_->engine : nullptr;
    }

    ma_engine* AudioSystem::GetEngine() const
    {
        return impl_ ? &impl_->engine : nullptr;
    }
}
