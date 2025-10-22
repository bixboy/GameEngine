#include "Bix/Game/Test/Player.h"

#include <memory>

#include "Bix/Game/Components/SpriteComponent.h"
#include "Bix/Input/InputManager.h"
#include "Bix/Math/Math.h"

namespace BixEngine::Game
{
    namespace
    {
        constexpr const char* kPlayerModule = "Test";
    }

    BIX_DEFINE_SCRIPT_CLASS(Player, (::BixEngine::Game::Scripting::ScriptRegistrationDescriptor{
        .name = "Player",
        .moduleName = kPlayerModule,
        .kind = ::BixEngine::Game::Scripting::ScriptKind::Actor,
    }));

    BIX_IMPLEMENT_CLASS(Player);

    void Player::RegisterProperties(::BixEngine::Engine::SaveSystem::BixClass& cls)
    {
        Actor::RegisterProperties(cls);

        using ::BixEngine::Engine::SaveSystem::RegisterProperty;

        RegisterProperty<Player>(cls, "moveSpeed", &Player::moveSpeed_);
        RegisterProperty<Player>(cls, "size", &Player::size_);
        RegisterProperty<Player>(cls, "color", &Player::color_);
    }

    Player::Player() : Actor("Player")
    {
        InitializeSpriteComponent();
    }

    Player::Player(const Math::Vector3& position, const Math::Vector3& size, SDL_Color color)
        : Actor(Math::Transform(position, Math::Rotator(), size))
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

    void Player::OnPostDeserialize()
    {
        Actor::OnPostDeserialize();
        RefreshSpriteComponent();
        SetScale(size_);
    }

    void Player::Update(float deltaTime)
    {
        RefreshSpriteComponent();
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
        auto sprite = std::make_unique<SpriteComponent>(this, color_, size_.x, size_.y);
        spriteComponent_ = sprite.get();
        AddComponent(std::move(sprite));
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
        SetScale(size_);
    }
}

