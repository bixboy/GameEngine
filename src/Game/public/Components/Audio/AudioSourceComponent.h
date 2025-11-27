#pragma once
#include "Components/Core/Component.h"
#include "Systems/Audio/AudioSystem.h"
#include "Ressources/RessourcesClass/AudioClip.h"
#include "Ressources/RessourcesClass/AudioContainer.h"
#include "AudioSourceComponent.generated.h"

namespace BixEngine::Game
{
    BCLASS()
    class AudioSourceComponent : public Component
    {
        GENERATED_BODY()
        
    public:
        using Super = Component;
        
        explicit AudioSourceComponent(Actor* owner);
        ~AudioSourceComponent() override;

        void BeginPlay() override;
        void Update(float dt) override;
        void DrawInspectorUI() override;
        
        void Play();
        void Stop();
        void SetVolume(float volume);

        [[nodiscard]] String GetTypeName() const override { return "AudioSourceComponent"; }

        
        // --- Properties ---
        BPROPERTY()
        std::shared_ptr<resources::AudioClip> AudioClip;

        BPROPERTY()
        std::shared_ptr<resources::AudioContainer> AudioContainer;

        BPROPERTY()
        float Volume = 1.0f;

        BPROPERTY()
        bool Loop = false;

        BPROPERTY()
        bool Is3D = false;

        BPROPERTY()
        bool IsMusic = false;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}