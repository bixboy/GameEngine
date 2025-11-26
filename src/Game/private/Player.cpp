#include "Player.h"
#include <memory>
#include "Components/SpriteComponent.h"
#include "InputManager.h"
#include "Components/AudioSourceComponent.h"
#include "Components/SpriteAnimatorComponent.h"
#include "Ressources/ResourceManager.h"



namespace BixEngine::Game
{

    Player::Player() : Actor("Player")
    {
        InitializeSpriteComponent();
    }

    Player::Player(Math::Transform transform) : Actor(transform)
    {
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

        inputManager.BindAction("PlayMusicTest", SDLK_SPACE, Input::InputEvent::Pressed, this, &Player::StarTestMusic);

        inputManager.UnbindAxis("MoveRight", SDLK_W);
    }
    
    void Player::BeginPlay()
    {
        RefreshSpriteComponent();
        
        if (animatorComponent_ && !texturePath_.IsEmpty())
        {
            if (animatorComponent_->GetAtlasPath().IsEmpty())
            {
                animatorComponent_->LoadSpriteAtlas(texturePath_, animationName_);
                if (!animationName_.IsEmpty())
                {
                    animatorComponent_->Play(animationName_);
                }
            }
        }

        if (audioSrc_ && audioClip_)
        {
            if (!audioSrc_->AudioClip)
            {
                audioSrc_->AudioClip = audioClip_;
            }
        }

        // Initialize components AFTER setting them up
        Actor::BeginPlay();
    }

    void Player::Update(float deltaTime)
    {
        Actor::Update(deltaTime);

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
        animatorComponent_ = AddComponent<SpriteAnimatorComponent>();
        spriteComponent_ = animatorComponent_;

        audioSrc_ = AddComponent<AudioSourceComponent>();
        if (audioSrc_)
        {
            audioSrc_->Is3D = false; // Disable 3D for testing
        }
    }

    void Player::RefreshSpriteComponent()
    {
        if (!spriteComponent_)
            InitializeSpriteComponent();
    }

    void Player::StarTestMusic()
    {
        LOG_INFO("Player::StarTestMusic triggered");
        if (audioSrc_)
        {
            audioSrc_->Play();
        }
        else
        {
            LOG_ERROR("Player::StarTestMusic: No AudioSourceComponent!");
        }
    }
}
