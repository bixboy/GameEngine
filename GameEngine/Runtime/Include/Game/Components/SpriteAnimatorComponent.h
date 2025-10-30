    #pragma once
    #include "Game/Components/Component.h"
    #include "Engine/Render/SpriteAnimator.h"
    #include "SpriteComponent.generated.h"


    namespace BixEngine::Game
    {
        BCLASS(Blueprintable)
        class SpriteAnimatorComponent : public Component
        {
            GENERATED_BODY()

        public:
            explicit SpriteAnimatorComponent(Actor* owner);

            void BeginPlay() override;
            void Update(float deltaTime) override;

            void AddAnimation(const Render::SpriteAnimation& animation);
            
            void Play(const String& name);
            void Pause();
            void Stop();
            
            void SetPlaybackSpeed(float speed);
            
            [[nodiscard]] bool IsPlaying() const noexcept;

        private:
            Render::SpriteAnimator animator_;
        };
    }
