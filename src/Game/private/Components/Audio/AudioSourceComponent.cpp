#include "Components/Audio/AudioSourceComponent.h"
#include "Framework/Actor.h"
#include "Math/Transform.h"
#include "Debug/Logger.h"
#include <miniaudio.h>
#include <imgui.h>
#include <filesystem>
#include <string>
#include "Gui/Panels/ActorInspector/PropertyInspector.h"


namespace BixEngine::Game
{
    struct AudioSourceComponent::Impl
    {
        ma_sound sound;
        bool initialized = false;
        
        std::string currentFilePath;
        bool wasIsMusic = false;
        bool wasIs3D = false;
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
        if (PlayOnAwake)
        {
            Play();
        }
    }

    void AudioSourceComponent::Update(float dt)
    {
        if (!impl_->initialized)
            return;

        if (Is3D && owner_)
        {
            auto pos = owner_->GetTransform().GetWorldPosition();
            ma_sound_set_position(&impl_->sound, pos.x, pos.y, pos.z);
            
            // Effet Doppler
            // ma_sound_set_velocity(...)
        }
    }
    
    void AudioSourceComponent::Play()
    {
        auto* audioSystem = &Systems::AudioSystem::Get();
        ma_engine* engine = audioSystem->GetEngine();
        
        if (!engine) return;

        std::shared_ptr<Resources::AudioClip> clipToPlay = AudioClip;
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
            return;
        }

        String path = clipToPlay->GetPath();
        if (path.empty())
            return;
        
        bool needsReinit = !impl_->initialized || impl_->currentFilePath != path.c_str() || impl_->wasIsMusic != IsMusic || impl_->wasIs3D != Is3D;
        if (needsReinit)
        {
            if (impl_->initialized)
            {
                ma_sound_uninit(&impl_->sound);
                impl_->initialized = false;
            }

            ma_uint32 flags = IsMusic ? MA_SOUND_FLAG_STREAM : MA_SOUND_FLAG_DECODE;
            if (!Is3D)
                flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

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
                LOG_ERROR("Audio: Failed to load " + path);
                return;
            }

            impl_->initialized = true;
            impl_->currentFilePath = path.c_str();
            impl_->wasIsMusic = IsMusic;
            impl_->wasIs3D = Is3D;
        }
        else
        {
            ma_sound_seek_to_pcm_frame(&impl_->sound, 0);
        }
        
        ma_sound_set_volume(&impl_->sound, Volume * volumeMult);
        ma_sound_set_pitch(&impl_->sound, Pitch * pitchMult);
        ma_sound_set_looping(&impl_->sound, Loop); 

        if (Is3D && owner_)
        {
            auto pos = owner_->GetTransform().GetWorldPosition();
            ma_sound_set_position(&impl_->sound, pos.x, pos.y, pos.z);
        }

        ma_sound_start(&impl_->sound);
    }

    void AudioSourceComponent::Stop()
    {
        if (impl_->initialized)
        {
            ma_sound_stop(&impl_->sound);
            ma_sound_seek_to_pcm_frame(&impl_->sound, 0);
        }
    }
    
    void AudioSourceComponent::SetPaused(bool paused)
    {
        if (!impl_->initialized) return;
        
        if (paused)
        {
            ma_sound_stop(&impl_->sound);
        }
        else
        {
            ma_sound_start(&impl_->sound);
        }
    }

    bool AudioSourceComponent::IsPlaying() const
    {
        if (!impl_->initialized)
            return false;
        
        return ma_sound_is_playing(&impl_->sound);
    }

    void AudioSourceComponent::SetVolume(float volume)
    {
        Volume = volume;
        if (impl_->initialized)
        {
            ma_sound_set_volume(&impl_->sound, Volume);
        }
    }
    
    void AudioSourceComponent::SetPitch(float pitch)
    {
        Pitch = pitch;
        if (impl_->initialized)
        {
            ma_sound_set_pitch(&impl_->sound, Pitch);
        }
    }

    void AudioSourceComponent::DrawInspectorUI()
    {
        // PropertyInspector::DrawClassProperties est déjà appelé par l'éditeur avant DrawInspectorUI.
        // On ne dessine que les contrôles supplémentaires ici.

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextDisabled("Preview");

        if (ImGui::Button("Play"))
        {
            Play();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Stop"))
        {
            Stop();
        }

        if (impl_->initialized)
        {
            bool playing = ma_sound_is_playing(&impl_->sound);
            ImGui::SameLine();
            if (playing)
            {
                ImGui::TextColored(ImVec4(0,1,0,1), "Playing...");
            }
            else
            {
                ImGui::TextColored(ImVec4(1,1,1,0.5f), "Stopped");
            }
        }
    }
}