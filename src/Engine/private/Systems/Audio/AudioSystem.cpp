#include "Systems/Audio/AudioSystem.h"
#include "Debug/Logger.h"
#include <miniaudio.h>
#include <vector>

#include "Ressources/Core/ResourceManager.h"


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
        {
            LOG_ERROR("Failed to initialize Audio Engine.");
            return false;
        }

        impl_->initialized = true;
        ma_engine_set_volume(&impl_->engine, 1.0f);
        
        LOG_INFO("Audio System initialized successfully.");
        return true;
    }

    void AudioSystem::Shutdown()
    {
        if (impl_->initialized)
        {
            // 1. Nettoyage de la musique
            if (impl_->musicInitialized)
            {
                ma_sound_uninit(&impl_->musicSound);
                impl_->musicInitialized = false;
            }
            
            // 2. Nettoyage des sons SFX actifs
            for (auto* sound : impl_->activeSounds)
            {
                if (sound)
                {
                    ma_sound_uninit(sound);
                    delete sound;
                }
            }
            impl_->activeSounds.clear();

            // 3. Arrêt du moteur
            ma_engine_uninit(&impl_->engine);
            impl_->initialized = false;
            
            LOG_INFO("Audio System shutdown.");
        }
    }

    void AudioSystem::UpdateListener(const Math::Vector3& pos, const Math::Vector3& fwd, const Math::Vector3& up)
    {
        if (!impl_->initialized)
            return;

        ma_engine_listener_set_position(&impl_->engine, 0, pos.x, pos.y, pos.z);
        ma_engine_listener_set_direction(&impl_->engine, 0, fwd.x, fwd.y, fwd.z);
        ma_engine_listener_set_world_up(&impl_->engine, 0, up.x, up.y, up.z);

        for (auto it = impl_->activeSounds.begin(); it != impl_->activeSounds.end(); )
        {
            if (ma_sound_at_end(*it))
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
        if (!impl_->initialized)
            return;
        
        ma_engine_play_sound(&impl_->engine, filePath.c_str(), nullptr);
    }

    void AudioSystem::PlaySound(const String& filePath, float volume, float pitch)
    {
        auto clip = Resources::ResourceManager::Get().Get<Resources::AudioClip>(filePath);
        if (clip)
        {
            PlaySound(clip, volume, pitch);
        }
        else
        {
            LOG_ERROR("AudioSystem: Impossible de jouer le son (fichier introuvable ou échec chargement) : " + filePath);
        }
    }

    void AudioSystem::PlaySound(std::shared_ptr<Resources::AudioClip> clip, float volume, float pitch)
    {
        if (!impl_->initialized || !clip)
            return;

        ma_decoder_config decoderConfig = ma_decoder_config_init_default();
        ma_decoder decoder;
    
        ma_result result = ma_decoder_init_memory(
            clip->GetData().data(),
            clip->GetData().size(),
            &decoderConfig, 
            &decoder
        );

        if (result != MA_SUCCESS)
        {
            LOG_ERROR("AudioSystem: Failed to create decoder from memory.");
            return;
        }

        ma_sound* sound = new ma_sound;

        result = ma_sound_init_from_data_source(
            &impl_->engine, 
            &decoder, 
            MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC,
            nullptr, 
            sound
        );

        if (result == MA_SUCCESS)
        {
            ma_sound_set_volume(sound, volume);
            ma_sound_set_pitch(sound, pitch);
            ma_sound_start(sound);
        
            impl_->activeSounds.push_back(sound);
        }
        else
        {
            LOG_ERROR("AudioSystem: Failed to init sound from data source.");
            delete sound;
        }

        ma_decoder_uninit(&decoder);
    }

    void AudioSystem::PlayMusic(const String& filePath)
    {
        if (!impl_->initialized)
            return;

        if (impl_->musicInitialized && impl_->currentMusicPath == filePath)
        {
            if (!ma_sound_is_playing(&impl_->musicSound))
            {
                ma_sound_start(&impl_->musicSound);
            }
            
            return;
        }

        if (impl_->musicInitialized)
        {
            ma_sound_stop(&impl_->musicSound);
            ma_sound_uninit(&impl_->musicSound);
            impl_->musicInitialized = false;
        }
        
        ma_result result = ma_sound_init_from_file(&impl_->engine, filePath.c_str(), MA_SOUND_FLAG_STREAM, nullptr, nullptr, &impl_->musicSound);
        
        if (result == MA_SUCCESS)
        {
            ma_sound_set_looping(&impl_->musicSound, MA_TRUE);
            ma_sound_start(&impl_->musicSound);
            
            impl_->musicInitialized = true;
            impl_->currentMusicPath = filePath;
        }
        else
        {
            LOG_ERROR(String("Failed to load music: " + filePath));
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
        }
        
        ma_sound_seek_to_pcm_frame(&impl_->musicSound, 0);
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
