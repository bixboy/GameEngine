#include "Entities/Player.h"
#include <memory>
#include "Components/Sprite/SpriteComponent.h"
#include "Components/Core/BoxColliderComponent.h"
#include "InputManager.h"
#include "Components/Audio/AudioSourceComponent.h"
#include "Components/Sprite/SpriteAnimatorComponent.h"
#include "Ressources/Core/ResourceManager.h"
#include "Framework/BGameplayStatics.h"



namespace BixEngine::Game
{

    Player::Player(const Math::Transform& transform) : Actor("Player", transform)
    {
        InitializeComponents();
    }

    void Player::SetupInput(Input::InputManager&  )
    {
        LOG_INFO("Player::SetupInput called");
    }
    


    void Player::BeginPlay()
    {
        RefreshSpriteComponent();

        if (GetOwningScene())
        {
            SetupInput(GetOwningScene()->GetInputManager());
        }
        else
        {
            LOG_ERROR("Player::BeginPlay: Owning Scene is null, cannot SetupInput.");
        }

        Actor::BeginPlay();
    }

    void Player::Update(float deltaTime)
    {
        Actor::Update(deltaTime);
        ApplyMovement(deltaTime);
    }

    void Player::ApplyMovement(float deltaTime)
    {
        if (physicsComponent_)
        {
            Math::Vector2<float> currentVel = physicsComponent_->GetLinearVelocity();
            float targetX = moveSpeed_;
            
            physicsComponent_->SetLinearVelocity({ targetX, currentVel.y });
        }
        else
        {
            Math::Vector3 movement{moveSpeed_, 0.0f, 0.0f};
            SetPosition(GetPosition() + movement * deltaTime);
        }
    }

    void Player::SetMoveSpeed(float newSpeed)
    {
        moveSpeed_ = newSpeed;
    }

    float Player::GetMoveSpeed() const
    {
        return moveSpeed_;
    }

    void Player::OnComponentRemoved(const Component& component)
    {
        if (&component == animatorComponent_)
        {
            animatorComponent_ = nullptr;
        }

        Actor::OnComponentRemoved(component);
    }

    void Player::InitializeComponents()
    {
        animatorComponent_ = AddComponent<SpriteAnimatorComponent>();

        physicsComponent_ = AddComponent<BoxColliderComponent>();
        if (physicsComponent_)
        {
            physicsComponent_->SetSimulatePhysics(true);
            physicsComponent_->SetFixedRotation(true);
            physicsComponent_->SetBoxExtent({25.0f, 25.0f}); 
            physicsComponent_->SetFriction(0.0f); 
            
            physicsComponent_->SetGravityScale(2.5f);
            physicsComponent_->SetMass(50.0f);
            physicsComponent_->SetAirResistance(1.0f);
            physicsComponent_->SetMaxFallSpeed(1500.0f);

            physicsComponent_->BindOnCollisionEnter(this, &Player::OnCollisionEnter);
        }

        audioSrc_ = AddComponent<AudioSourceComponent>();
        if (audioSrc_)
        {
            audioSrc_->Is3D = false; 
        }
    }

    void Player::RefreshSpriteComponent()
    {
        if (!animatorComponent_)
            InitializeComponents();

        if (!physicsComponent_)
            physicsComponent_ = GetComponent<BoxColliderComponent>();
    }

    void Player::StarTestMusic()
    {
        LOG_INFO("Player::StarTestMusic triggered");
        if (audioSrc_)
        {
            audioSrc_->Play();
        }
    }

    void Player::OnCollisionEnter(Actor* other, const CollisionHitResult& result)
    {
        if (other)
        {
            LOG_INFO("Player collided with: " + other->GetTypeName());
            if (other->GetTypeName() == "Zapper")
            {
                 LOG_INFO("Player hit Zapper! Restarting scene...");
                 if (const auto* scene = GetOwningScene())
                 {
                     BGameplayStatics::LoadScene(GetOwningScene(), scene->GetName());
                 }
            }
        }
    }
}
