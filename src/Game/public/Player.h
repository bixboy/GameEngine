#pragma once
#include <SDL3/SDL.h>
#include <iosfwd>
#include <memory>
#include "Actor.h"
#include "Math/Vector2.h"
#include "Components/AudioSourceComponent.h"
#include "Ressources/RessourcesClass/AudioClip.h"
#include "Player.generated.h"


namespace BixEngine::Graphics
{
    class Renderer;
}

namespace BixEngine::Input
{
    class InputManager;
}

namespace BixEngine::Game
{
    class SpriteAnimatorComponent;
    class SpriteComponent;

    BCLASS()
    class Player : public Actor
    {
        GENERATED_BODY()

        public:
            Player();
            Player(Math::Transform transform = Math::Transform());

            void SetupInput(Input::InputManager& inputManager) override;

            void BeginPlay() override;
            void Update(float deltaTime) override;

            void MoveForward(float value);
            void MoveRight(float value);

            [[nodiscard]] String GetTypeName() const noexcept override { return "Player"; }
            [[nodiscard]] std::unique_ptr<Actor> ClonePrototype() const override { return std::make_unique<Player>(Math::Transform()); }

        private:
            void OnComponentRemoved(const Component& component) override;

            void ApplyMovement(float deltaTime);
            void InitializeSpriteComponent();
            void RefreshSpriteComponent();

            void StarTestMusic();

            Math::Vector2<float> pendingInput_{};

            BPROPERTY()
            float moveSpeed_{200.0f};
        
            BPROPERTY()
            std::shared_ptr<resources::AudioClip> audioClip_;

            BPROPERTY()
            String texturePath_;

            BPROPERTY()
            String animationName_;

            SpriteComponent* spriteComponent_{nullptr};
            SpriteAnimatorComponent* animatorComponent_{nullptr};

            AudioSourceComponent* audioSrc_{nullptr};
        };
}
