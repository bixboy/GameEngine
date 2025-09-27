#pragma once
#include <SDL3/SDL.h>
#include <iosfwd>
#include "Game/Actor.h"

namespace Engine
{
    namespace Graphics { class Renderer; }
    namespace Input    { class InputManager; }

    namespace Game
    {
        class SpriteComponent;

        class Player : public Actor
        {
            public:
                Player();
                Player(const Math::Vector3& position, const Math::Vector3& size, SDL_Color color);

                void SetupInput(Input::InputManager& inputManager);

                void Update(float deltaTime) override;

                void MoveForward(float value);
                void MoveRight(float value);

                [[nodiscard]] std::string_view GetTypeName() const noexcept override { return "Player"; }

            private:
                void SerializeBinaryImpl(std::ostream& stream) const override;
                void DeserializeBinaryImpl(std::istream& stream) override;

                void ApplyMovement(float deltaTime);
                void InitializeSpriteComponent();
                void RefreshSpriteComponent();

                Math::Vector2 pendingInput_{};
                float moveSpeed_{200.0f};
                Math::Vector3 size_{Math::Vector3(32.0f, 32.0f, 1.0f)};
                SDL_Color color_{255, 255, 255, 255};
                SpriteComponent* spriteComponent_{nullptr};
        };
    }
}

