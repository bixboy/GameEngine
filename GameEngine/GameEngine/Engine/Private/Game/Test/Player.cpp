#include "Game/Test/Player.h"

namespace Engine::Game
{
    Player::Player(const Math::Vector3& position, const Math::Vector3& size)
        : Actor(Math::Transform(position, Math::Rotator(), size))
    {
        
    }

    void Player::SetupInput(Input::InputManager& input)
    {
        input.BindAction("MoveUp",    SDLK_Z, Input::InputEvent::Hold, this, &Player::MoveUp);
        input.BindAction("MoveDown",  SDLK_S, Input::InputEvent::Hold, this, &Player::MoveDown);
        input.BindAction("MoveLeft",  SDLK_Q, Input::InputEvent::Hold, this, &Player::MoveLeft);
        input.BindAction("MoveRight", SDLK_D, Input::InputEvent::Hold, this, &Player::MoveRight);
    }

    void Player::MoveUp()    { AddMovement(0.0f, -1.0f); }
    void Player::MoveDown()  { AddMovement(0.0f,  1.0f); }
    void Player::MoveLeft()  { AddMovement(-1.0f, 0.0f); }
    void Player::MoveRight() { AddMovement( 1.0f, 0.0f); }

    void Player::AddMovement(float dx, float dy)
    {
        Math::Vector3 pos = GetPosition();
        pos.x += dx * speed_ * deltaTime_;
        pos.y += dy * speed_ * deltaTime_;
        
        SetPosition(pos);
    }

    void Player::Update(float deltaTime)
    {
        deltaTime_ = deltaTime;
    }
}
