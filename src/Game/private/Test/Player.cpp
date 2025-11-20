#include "Test/Player.h"
#include <istream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include "Components/SpriteComponent.h"
#include "InputManager.h"
#include "Components/SpriteAnimatorComponent.h"


namespace BixEngine::Game
{
    Player::Player() : Actor("Player")
    {
        InitializeSpriteComponent();
    }

    Player::Player(const Math::Vector3& position, const Math::Vector3& size, SDL_Color color) : Actor(
        Math::Transform(position, Math::Rotator(), size))
    {
        size_ = size;
        color_ = color;
        InitializeSpriteComponent();
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

        inputManager.UnbindAxis("MoveRight", SDLK_W);
    }

    void Player::Update(float deltaTime)
    {
        Super::Update(deltaTime);

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
        Math::Vector2<float> input = pendingInput_;
        if (input.LengthSquared() > 1.0f)
            input = input.Normalized();

        Math::Vector3 movement{input.x, -input.y, 0.0f};
        movement = movement * (moveSpeed_ * deltaTime);

        SetPosition(GetPosition() + movement);
    }

    void Player::SerializeBinaryImpl(std::ostream& stream) const
    {
        WritePrimitive(stream, moveSpeed_);
        stream.write(reinterpret_cast<const char*>(&color_), sizeof(color_));
        WritePrimitive(stream, size_.x);
        WritePrimitive(stream, size_.y);
        WritePrimitive(stream, size_.z);
    }

    void Player::DeserializeBinaryImpl(std::istream& stream)
    {
        ReadPrimitive(stream, moveSpeed_);
        stream.read(reinterpret_cast<char*>(&color_), sizeof(color_));

        if (!stream)
            throw std::runtime_error("Failed to read player color from stream.");

        ReadPrimitive(stream, size_.x);
        ReadPrimitive(stream, size_.y);
        ReadPrimitive(stream, size_.z);

        RefreshSpriteComponent();
        SetScale(size_);
    }

    void Player::OnComponentRemoved(const Component& component)
    {
        if (&component == spriteComponent_)
        {
            spriteComponent_ = nullptr;
        }

        Actor::OnComponentRemoved(component);
    }

    void Player::InitializeSpriteComponent()
    {
        auto animator = std::make_unique<SpriteAnimatorComponent>(this);
        animatorComponent_ = animator.get();

        // Setup
        animatorComponent_->SetColor(color_);
        animatorComponent_->SetDimensions(size_.x, size_.y);

        AddComponent(std::move(animator));
        SetScale(size_);
    }


    void Player::RefreshSpriteComponent()
    {
        if (!spriteComponent_)
        {
            InitializeSpriteComponent();
            return;
        }

        spriteComponent_->SetColor(color_);
        spriteComponent_->SetDimensions(size_.x, size_.y);
    }
}
