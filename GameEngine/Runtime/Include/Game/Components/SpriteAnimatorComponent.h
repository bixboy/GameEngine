    #pragma once
    #include "Game/Components/Component.h"
    #include "Engine/Render/SpriteAnimator.h"
    #include "Core/Math/Rect.h"
    #include "SpriteComponent.generated.h"


    namespace BixEngine::Render
    {
        class Texture;
    }

    namespace BixEngine::Game
    {
        class SpriteComponent;

        BCLASS(Blueprintable)
        class SpriteAnimatorComponent : public Component
        {
            GENERATED_BODY()

        public:
            explicit SpriteAnimatorComponent(Actor* owner);

            void BeginPlay() override;
            void Update(float deltaTime) override;

            void AddAnimation(Render::SpriteAnimation animation);

            void Play(const String& name);
            void Pause();
            void Stop();

            void SetPlaybackSpeed(float speed);

            [[nodiscard]] bool IsPlaying() const noexcept;

            void SetTargetSprite(SpriteComponent* sprite) noexcept { spriteComponent_ = sprite; }

        private:
            Render::SpriteAnimator animator_;
            SpriteComponent* spriteComponent_{nullptr};
            Render::Texture* currentTexture_{nullptr};
            Math::Rect currentUVRect_{};
        };
    }
