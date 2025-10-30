#include "Game/Components/SpriteAnimatorComponent.h"
#include "Game/Actor.h"

namespace BixEngine::Game
{
    SpriteAnimatorComponent::SpriteAnimatorComponent(Actor* owner): Component(owner)
    {
    }

    void SpriteAnimatorComponent::BeginPlay()
    {
        Component::BeginPlay();
    }

    void SpriteAnimatorComponent::Update(float deltaTime)
    {
        animator_.Update(deltaTime);

        if (const auto* frame = animator_.GetCurrentFrame())
        {
            if (auto* spriteRenderer = owner_->GetComponent<Render::>())
            {
                spriteRenderer->SetTexture(frame->TexturePtr);
                spriteRenderer->SetUV(frame->UVRect);
            }
        }
    }

    void SpriteAnimatorComponent::AddAnimation(const Render::SpriteAnimation& animation)
    {
        animator_.AddAnimation(animation);
    }

    void SpriteAnimatorComponent::Play(const String& name)
    {
        animator_.Play(name);
    }

    void SpriteAnimatorComponent::Pause()
    {
        animator_.Pause();
    }

    void SpriteAnimatorComponent::Stop()
    {
        animator_.Stop();
    }

    void SpriteAnimatorComponent::SetPlaybackSpeed(float speed)
    {
        animator_.SetSpeed(speed);
    }

    bool SpriteAnimatorComponent::IsPlaying() const noexcept
    {
        return animator_.IsPlaying();
    }
}
