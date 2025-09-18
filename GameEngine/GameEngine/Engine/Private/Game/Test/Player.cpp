#include "Game/Test/Player.h"

#include <memory>

#include "Game/Components/SpriteComponent.h"
#include "Input/InputManager.h"
#include "Math/Math.h"

namespace Engine::Game
{
    Player::Player(const Math::Vector3& position, const Math::Vector3& size, SDL_Color color)
        : Actor(Math::Transform(position, Math::Rotator(), size))
    {
        AddComponent(std::make_unique<SpriteComponent>(this, color, size.x, size.y));
    }

    void Player::SetupInput(Input::InputManager& inputManager)
    {
        inputManager.BindAxis("MoveForward", SDLK_W, this, &Player::MoveForward, 1.0f);
        inputManager.BindAxis("MoveForward", SDLK_Z, this, &Player::MoveForward, 1.0f);
        inputManager.BindAxis("MoveForward", SDLK_UP, this, &Player::MoveForward, 1.0f);
        inputManager.BindAxis("MoveForward", SDLK_S, this, &Player::MoveForward, -1.0f);
        inputManager.BindAxis("MoveForward", SDLK_DOWN, this, &Player::MoveForward, -1.0f);

        inputManager.BindAxis("MoveRight", SDLK_D, this, &Player::MoveRight, 1.0f);
        inputManager.BindAxis("MoveRight", SDLK_RIGHT, this, &Player::MoveRight, 1.0f);
        inputManager.BindAxis("MoveRight", SDLK_A, this, &Player::MoveRight, -1.0f);
        inputManager.BindAxis("MoveRight", SDLK_Q, this, &Player::MoveRight, -1.0f);
        inputManager.BindAxis("MoveRight", SDLK_LEFT, this, &Player::MoveRight, -1.0f);
    }

    void Player::Update(float deltaTime)
    {
        ApplyMovement(deltaTime);
    }

    void Player::MoveForward(float value)
    {
        pendingInput_.y = value;
    }

    void Player::MoveRight(float value)
    {
        pendingInput_.x = value;
    }

    void Player::ApplyMovement(float deltaTime)
    {
        Math::Vector2 input = pendingInput_;
        if (input.LengthSquared() > 1.0f)
            input = input.Normalized();

        Math::Vector3 movement{ input.x, -input.y, 0.0f };
        movement = movement * (moveSpeed_ * deltaTime);

        SetPosition(GetPosition() + movement);
    }
}

