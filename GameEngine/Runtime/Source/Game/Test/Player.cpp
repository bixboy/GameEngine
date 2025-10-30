#include "Game/Test/Player.h"

#include <istream>
#include <memory>
#include <ostream>
#include <stdexcept>

#include "Game/Components/SpriteComponent.h"
#include "Input/InputManager.h"
#include "Core/Math/Math.h"
#include "Engine/Render/SpriteAnimator.h"
#include "Engine/Render/TextureManager.h"
#include "Game/Components/SpriteAnimatorComponent.h"
#include "Graphics/Renderer.h"

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

        // Charger la texture (image ou spritesheet)
        auto& texMgr = Render::TextureManager::Get();
        auto texture = texMgr.LoadTexture("../../../../Resources/Pink_Monster/Pink_Monster_Idle_4.png", Graphics::Renderer::Get()->GetSDLRenderer());

        // Créer les frames
        std::vector<Render::SpriteFrame> frames;
        const int frameCount = 4;
        const int frameWidth = texture->GetWidth() / frameCount;
        const int frameHeight = texture->GetHeight();

        for (int i = 0; i < frameCount; ++i)
        {
            auto frameData = std::make_shared<Render::SpriteFrameData>();
            frameData->TexturePtr = texture.get();
            frameData->UVRect = Math::Rect{
                static_cast<float>(i * frameWidth) / texture->GetWidth(),
                0.f,
                static_cast<float>(frameWidth) / texture->GetWidth(),
                1.f
            };
            frames.emplace_back(frameData);
        }

        // Créer l’animation "Idle"
        Render::SpriteAnimation idleAnim;
        idleAnim.Name = "Idle";
        idleAnim.Frames = frames;
        idleAnim.FrameRate = 8.f;
        idleAnim.bLoop = true;

        // Ajouter l’animation et la jouer
        animatorComponent_->AddAnimation(idleAnim);
        animatorComponent_->Play("Idle");

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

