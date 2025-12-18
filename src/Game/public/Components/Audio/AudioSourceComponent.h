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
        
        AudioSourceComponent();
        explicit AudioSourceComponent(Actor* owner);
        ~AudioSourceComponent() override;

        void BeginPlay() override;
        void Update(float dt) override;
        // DrawInspectorUI removed (using auto inspector)
        
        void Play();
        void Stop();
        void SetVolume(float volume);

        [[nodiscard]] String GetTypeName() const override { return "AudioSourceComponent"; }

        
        // --- Properties ---
        // --- Properties ---
        BPROPERTY(EditAnywhere, Category="Audio Settings")
        std::shared_ptr<resources::AudioClip> AudioClip;

        BPROPERTY(EditAnywhere, Category="Audio Settings")
        std::shared_ptr<resources::AudioContainer> AudioContainer;

        BPROPERTY(EditAnywhere, Category="Audio Settings")
        float Volume = 1.0f;

        BPROPERTY(EditAnywhere, Category="Audio Settings")
        bool Loop = false;

        BPROPERTY(EditAnywhere, Category="Audio Settings")
        bool Is3D = false;

        BPROPERTY(EditAnywhere, Category="Audio Settings")
        bool IsMusic = false;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}