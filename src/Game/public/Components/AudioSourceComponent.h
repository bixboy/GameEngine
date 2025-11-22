#pragma once
#include "Components/Component.h"
#include "Systems/AudioSystem.h"
#include "Ressources/RessourcesClass/AudioClip.h"
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
        ~AudioSourceComponent();

        void BeginPlay() override;
        void Update(float dt) override;
        void DrawInspectorUI() override;
        
        void Play();
        void Stop();
        void SetVolume(float volume);

        [[nodiscard]] String GetTypeName() const override { return "AudioSourceComponent"; }

        // Properties
        // Properties
        BPROPERTY()
        std::shared_ptr<resources::AudioClip> AudioClip;

        BPROPERTY()
        float Volume = 1.0f;

        BPROPERTY()
        bool Loop = false;

        BPROPERTY()
        bool Is3D = true;

        BPROPERTY()
        bool IsMusic = false;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
