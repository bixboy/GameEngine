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
        SpriteAnimatorComponent() = default;
        explicit SpriteAnimatorComponent(Actor* owner);

        void BeginPlay() override;
        void Update(float deltaTime) override;

        bool LoadSpriteAtlas(const String& atlasPath, const String& defaultAnimation = {});
        
        void DrawInspectorUI() override;

        void Play();
        void Play(const String& animationName);
        void Stop();

        [[nodiscard]] bool IsPlaying() const noexcept { return animator_.IsPlaying(); }
        [[nodiscard]] const String& GetAtlasPath() const noexcept { return atlasPath_; }
        [[nodiscard]] const String& GetCurrentAnimation() const noexcept { return currentAnimation_; }

    private:
        void ApplyCurrentFrame(bool allowFallbackToDefault);

        std::shared_ptr<Resources::SpriteAtlas> atlas_{};

        BPROPERTY(EditAnywhere, Category="Animation")
        String atlasPath_{};

        BPROPERTY(EditAnywhere, Category="Animation")
        String defaultAnimation_{};

        Resources::SpriteAnimator animator_{};
        String currentAnimation_{};
    };
}