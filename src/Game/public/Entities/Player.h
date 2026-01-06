#pragma once
#include <SDL3/SDL.h>
#include <iosfwd>
#include <memory>
#include "Framework/Actor.h"
#include "Math/Vector2.h"
#include "Components/Audio/AudioSourceComponent.h"
#include "Ressources/RessourcesClass/AudioClip.h"
#include "Framework/CollisionHitResult.h"
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
    class BoxColliderComponent;

    BCLASS()
    class Player : public Actor
    {
        GENERATED_BODY()

    public:
    
        explicit Player(const Math::Transform& transform = Math::Transform());

        void SetupInput(Input::InputManager& inputManager) override;

        void BeginPlay() override;
        void Update(float deltaTime) override;
    
        [[nodiscard]] String GetTypeName() const noexcept override { return "Player"; }
        [[nodiscard]] std::unique_ptr<Actor> ClonePrototype() const override { return std::make_unique<Player>(Math::Transform()); }

        void SetMoveSpeed(float newSpeed);
        float GetMoveSpeed() const;

    protected:
        BPROPERTY(EditAnywhere, Category="Gameplay")
        float moveSpeed_{200.0f};

        void OnComponentRemoved(const Component& component) override;

        void ApplyMovement(float deltaTime);
        void InitializeComponents();
        void RefreshSpriteComponent();

        void StarTestMusic();

        void OnCollisionEnter(Actor* other, const CollisionHitResult& result);

        Math::Vector2 pendingInput_{};

        SpriteAnimatorComponent* animatorComponent_{nullptr};
        BoxColliderComponent* physicsComponent_{nullptr};

        AudioSourceComponent* audioSrc_{nullptr};
    };
}
