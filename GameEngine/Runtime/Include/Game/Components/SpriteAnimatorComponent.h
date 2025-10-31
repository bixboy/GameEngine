#pragma once

#include "Game/Components/SpriteComponent.h"
#include "Engine/Render/TextureManager.h"
#include "Reflection/ReflectionMacros.h"
#include "Engine/Render/SpriteFrame.h"
#include <memory>
#include <vector>
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
        
        void LoadSpriteSheet(const String& texturePath, int columns, int rows, float frameRate = 8.f, bool loop = true);

        void Play();
        void Stop();

        [[nodiscard]] bool IsPlaying() const noexcept { return bPlaying_; }

    private:
        std::vector<Ressources::SpriteFrame> frames_;
        std::shared_ptr<Ressources::Texture> loadedTexture_{};
        int totalFrames_ = 0;
        int currentFrame_ = 0;
        float frameRate_ = 8.f;
        float timer_ = 0.f;
        bool bLoop_ = true;
        bool bPlaying_ = false;
    };
}
