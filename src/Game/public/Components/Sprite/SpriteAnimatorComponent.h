#pragma once
#include "Components/Sprite/SpriteComponent.h"
#include "Utils/ReflectionMacros.h"
#include "Render/Sprite/SpriteAnimator.h"
#include <memory>
#include "Ressources/RessourcesClass/SpriteAtlas.h"
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
        [[nodiscard]] const String& GetAtlasPath() const noexcept { return atlasPath_; }
        [[nodiscard]] const String& GetCurrentAnimation() const noexcept { return currentAnimation_; }

    private:
        void ApplyCurrentFrame(bool allowFallbackToDefault);

        BPROPERTY()
        std::shared_ptr<resources::SpriteAtlas> atlas_{};

        String defaultAnimation_{};

        resources::SpriteAnimator animator_{};
        
        String atlasPath_{};
        String currentAnimation_{};
    };
}
