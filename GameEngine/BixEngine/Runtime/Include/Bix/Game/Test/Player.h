#pragma once
#include <SDL3/SDL.h>
#include <iosfwd>
#include <memory>
#include "Bix/Game/Actor.h"
#include "Player.generated.h"


namespace BixEngine
{
    namespace Graphics { class Renderer; }
    namespace Input    { class InputManager; }

    namespace Game
    {
        class SpriteComponent;

        BCLASS()
        class Player : public Actor
        {
            public:
                GENERATED_BODY();

                Player();
                Player(const Math::Vector3& position, const Math::Vector3& size, SDL_Color color);

                void SetupInput(Input::InputManager& inputManager);

                void Update(float deltaTime) override;

                void MoveForward(float value);
                void MoveRight(float value);

                [[nodiscard]] String GetTypeName() const noexcept override { return "Player"; }
                [[nodiscard]] std::unique_ptr<Actor> ClonePrototype() const override { return std::make_unique<Player>(); }

            private:
                void SerializeBinaryImpl(std::ostream& stream) const override;
                void DeserializeBinaryImpl(std::istream& stream) override;

                void OnComponentRemoved(const Component& component) override;

                void ApplyMovement(float deltaTime);
                void InitializeSpriteComponent();
                void RefreshSpriteComponent();

                Math::Vector2 pendingInput_{};
            
                BPROPERTY()
                float moveSpeed_{200.0f};
            
                BPROPERTY()
                Math::Vector3 size_{Math::Vector3(32.0f, 32.0f, 1.0f)};
            
                BPROPERTY()
                SDL_Color color_{255, 255, 255, 255};
            
                SpriteComponent* spriteComponent_{nullptr};
        };
    }
}

