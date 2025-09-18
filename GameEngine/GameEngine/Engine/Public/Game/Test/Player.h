#pragma once
#include "Game/Actor.h"
#include "Input/InputManager.h"

namespace Engine::Game
{
    class Player : public Actor
    {
        public:
            Player(const Math::Vector3& position,
                   const Math::Vector3& size);

            void Update(float deltaTime) override;

            void SetupInput(Input::InputManager& input);

            void MoveUp();
            void MoveDown();
            void MoveLeft();
            void MoveRight();

        private:
            void AddMovement(float dx, float dy);
        
            float speed_ = 200.0f;
            float deltaTime_ = 0.0f;
    };
}
