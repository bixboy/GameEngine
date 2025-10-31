#include "Game/Test/Player.h"

#include <istream>
#include <memory>
#include <ostream>
#include <stdexcept>

#include "Game/Components/SpriteComponent.h"
#include "Input/InputManager.h"
#include "Core/Math/Math.h"
#include "Game/Components/SpriteAnimatorComponent.h"

namespace BixEngine::Game
{
    Player::Player() : Actor("Player")
    {
        InitializeSpriteComponent();
    }

    Player::Player(const Math::Vector3& position, const Math::Vector3& size, SDL_Color color) : Actor(Math::Transform(position, Math::Rotator(), size))
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

        Math::Vector3 movement{ input.x, -input.y, 0.0f };
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
        // Création du sprite visuel
        auto sprite = std::make_unique<SpriteComponent>(this, color_, size_.x, size_.y);
        spriteComponent_ = sprite.get();
        AddComponent(std::move(sprite));

        // Création du composant d’animation
        auto animator = std::make_unique<SpriteAnimatorComponent>(this);
        animatorComponent_ = animator.get();
        AddComponent(std::move(animator));

        // Lier le sprite à l’animator
        animatorComponent_->AddSpriteLayer(spriteComponent_);

        SpriteAnimatorComponent::SpriteAnimationClipConfig idleClip{};
        idleClip.Name = "Idle";
        idleClip.TexturePath = "../../../../Resources/Pink_Monster/Pink_Monster_Idle_4.png";
        idleClip.Columns = 4;
        idleClip.Rows = 1;
        idleClip.FrameCount = 4;
        idleClip.FrameRate = 8.0f;
        idleClip.bLoop = true;

        animatorComponent_->SetInitialClip(idleClip.Name);
        animatorComponent_->SetClips({idleClip});

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

