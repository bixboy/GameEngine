#include "Game/Components/SpriteAnimatorComponent.h"
#include "Game/Actor.h"
#include "Game/Components/SpriteComponent.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace BixEngine::Game
{
    namespace
    {
        constexpr float kRectEpsilon = 0.0001f;

        [[nodiscard]] bool AreRectsEqual(const Math::Rect& lhs, const Math::Rect& rhs) noexcept
        {
            return std::fabs(lhs.X - rhs.X) < kRectEpsilon &&
                   std::fabs(lhs.Y - rhs.Y) < kRectEpsilon &&
                   std::fabs(lhs.Width - rhs.Width) < kRectEpsilon &&
                   std::fabs(lhs.Height - rhs.Height) < kRectEpsilon;
        }
    }

    SpriteAnimatorComponent::SpriteAnimatorComponent(Actor* owner): Component(owner)
    {
    }

    void SpriteAnimatorComponent::BeginPlay()
    {
        Component::BeginPlay();

        if (!spriteComponent_)
            spriteComponent_ = owner_->GetComponent<SpriteComponent>();
    }

    void SpriteAnimatorComponent::Update(float deltaTime)
    {
        animator_.Update(deltaTime);

        if (const auto* frame = animator_.GetCurrentFrame())
        {
            if (!spriteComponent_)
                spriteComponent_ = owner_->GetComponent<SpriteComponent>();

            if (!spriteComponent_)
                return;

            if (currentTexture_ != frame->TexturePtr)
            {
                currentTexture_ = frame->TexturePtr;
                spriteComponent_->SetTexture(frame->TexturePtr);
            }

            if (!AreRectsEqual(currentUVRect_, frame->UVRect))
            {
                currentUVRect_ = frame->UVRect;
                spriteComponent_->SetUVRect(frame->UVRect);
            }
        }
        else if (spriteComponent_ && currentTexture_ != nullptr)
        {
            currentTexture_ = nullptr;
            currentUVRect_ = {};
            spriteComponent_->SetTexture(nullptr);
        }
    }

    void SpriteAnimatorComponent::AddAnimation(Render::SpriteAnimation animation)
    {
        animator_.AddAnimation(std::move(animation));
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
        animator_.SetSpeed(std::max(speed, 0.0f));
    }

    bool SpriteAnimatorComponent::IsPlaying() const noexcept
    {
        return animator_.IsPlaying();
    }
}
