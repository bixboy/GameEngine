#pragma once
#include <SDL3/SDL.h>
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
                Player(const Math::Vector3& position, const Math::Vector3& size, SDL_Color color);

                void SetupInput(Input::InputManager& inputManager);

                void Update(float deltaTime) override;

                void MoveForward(float value);
                void MoveRight(float value);

            private:
                void ApplyMovement(float deltaTime);

                Math::Vector2 pendingInput_{};
                float moveSpeed_{200.0f};
        };
    }
}

