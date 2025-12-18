#include "Components/Audio/AudioSourceComponent.h"
#include "Framework/Actor.h"
#include "Math/Transform.h"
#include "Debug/Logger.h"
#include "Ressources/Core/ResourceManager.h"
#include <miniaudio.h>
#include <imgui.h>
#include <filesystem>
#include <vector>
#include <string>
#include "Utils/FileIO/FilesUtils.h"
#include "Gui/Utils/ContentBrowserUtils.h"

namespace BixEngine::Game
{
    struct AudioSourceComponent::Impl
    {
        ma_sound sound;
        bool initialized = false;
    };
    
    AudioSourceComponent::AudioSourceComponent() : impl_(std::make_unique<Impl>())
    {
    }

    AudioSourceComponent::AudioSourceComponent(Actor* owner) : Component(owner), impl_(std::make_unique<Impl>())
    {
    }

    AudioSourceComponent::~AudioSourceComponent()
    {
        if (impl_->initialized)
        {
            ma_sound_uninit(&impl_->sound);
        }
    }
    
    void AudioSourceComponent::BeginPlay()
    {
    }

    void AudioSourceComponent::Update(float dt)
    {
        if (!impl_->initialized) return;

        if (Is3D && owner_)
        {
            auto pos = owner_->GetTransform().GetPosition();
            ma_sound_set_position(&impl_->sound, pos.x, pos.y, pos.z);
        }
    }
    
    // ==============================================================================
    // AUDIO CONTROL
    // ==============================================================================
    void AudioSourceComponent::Play()
    {
        auto* audioSystem = &Systems::AudioSystem::Get();
        ma_engine* engine = audioSystem->GetEngine();
        
        if (!engine)
        {
            LOG_ERROR("AudioSystem engine is null.");
            return;
        }

        std::shared_ptr<resources::AudioClip> clipToPlay = AudioClip;
        float volumeMult = 1.0f;
        float pitchMult = 1.0f;

        if (AudioContainer)
        {
            auto resolved = AudioContainer->ResolveSound();
            if (resolved.Clip)
            {
                clipToPlay = resolved.Clip;
                volumeMult = resolved.Volume;
                pitchMult = resolved.Pitch;
            }
        }

        if (!clipToPlay)
        {
            LOG_WARNING("AudioSourceComponent::Play: No AudioClip or AudioContainer resolved.");
            return;
        }

        if (impl_->initialized)
        {
            ma_sound_uninit(&impl_->sound);
            impl_->initialized = false;
        }

        ma_uint32 flags = IsMusic ? MA_SOUND_FLAG_STREAM : MA_SOUND_FLAG_DECODE;
        
        if (!Is3D)
            flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

        String path = clipToPlay->GetPath();
        ma_result result = ma_sound_init_from_file(
            engine,
            path.c_str(),
            flags,
            NULL,
            NULL,
            &impl_->sound
        );

        if (result != MA_SUCCESS)
        {
            LOG_ERROR("Miniaudio Error: Failed to load " + path);
            return;
        }

        impl_->initialized = true;

        ma_sound_set_volume(&impl_->sound, Volume * volumeMult);
        ma_sound_set_pitch(&impl_->sound, pitchMult);
        ma_sound_set_looping(&impl_->sound, Loop); 

        if (Is3D && owner_)
        {
            auto pos = owner_->GetTransform().GetPosition();
            ma_sound_set_position(&impl_->sound, pos.x, pos.y, pos.z);
        }

        ma_sound_start(&impl_->sound);
        LOG_INFO("Playing: " + path + " (Vol: " + std::to_string(volumeMult) + ", Pitch: " + std::to_string(pitchMult) + ")");
    }

    void AudioSourceComponent::Stop()
    {
        if (impl_->initialized)
        {
            ma_sound_stop(&impl_->sound);
        }
    }

    void AudioSourceComponent::SetVolume(float volume)
    {
        Volume = volume;
        if (impl_->initialized)
        {
            ma_sound_set_volume(&impl_->sound, Volume);
        }
    }

    // ==============================================================================
    // EDITOR UI
    // ==============================================================================
    // DrawInspectorUI removed
}