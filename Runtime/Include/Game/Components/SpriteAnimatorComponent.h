#pragma once

#include "Game/Components/SpriteComponent.h"
#include "Reflection/ReflectionMacros.h"
#include "Engine/Render/SpriteAnimator.h"
#include <memory>
#include "Engine/Ressources/SpriteAtlas.h"
#include "SpriteAnimatorComponent.generated.h"


namespace BixEngine::Game
{
    BCLASS()
    class SpriteAnimatorComponent : public SpriteComponent
    {
        GENERATED_BODY()

    public:
        explicit SpriteAnimatorComponent(Actor* owner);

        void BeginPlay() override;
        void Update(float deltaTime) override;
        void DrawInspectorUI() override;

        bool LoadSpriteAtlas(const String& atlasPath, const String& defaultAnimation = {});

        void Play();
        void Play(const String& animationName);
        void Stop();

        [[nodiscard]] bool IsPlaying() const noexcept { return animator_.IsPlaying(); }

    private:
        void ApplyCurrentFrame(bool allowFallbackToDefault);

        BPROPERTY()
        std::shared_ptr<resources::SpriteAtlas> atlas_{};

        BPROPERTY()
        String defaultAnimation_{};

        resources::SpriteAnimator animator_{};
        String atlasPath_{};
        String currentAnimation_{};
    };
}
