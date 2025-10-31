#include "Game/Components/SpriteAnimatorComponent.h"
#include "Core/Logger.h"
#include "Engine/Ressources/ResourceManager.h"
#include "Engine/Ressources/SpriteAtlas.h"
#include "Game/Actor.h"
#include <algorithm>
#include <string>

namespace BixEngine::Game
{
    SpriteAnimatorComponent::SpriteAnimatorComponent(Actor* owner): SpriteComponent(owner)
    {
    }

    void SpriteAnimatorComponent::BeginPlay()
    {
        SpriteComponent::BeginPlay();

        ApplyCurrentFrame(true);
    }

    void SpriteAnimatorComponent::Update(float deltaTime)
    {
        Super::Update(deltaTime);

        if (!atlas_ || !animator_.IsPlaying())
            return;

        animator_.Update(std::max(0.0f, deltaTime));
        ApplyCurrentFrame(false);
    }

    bool SpriteAnimatorComponent::LoadSpriteAtlas(const String& atlasPath, const String& defaultAnimation)
    {
        auto& resourceManager = Core::ResourceManager::Get();
        auto atlas = resourceManager.Get<Ressources::SpriteAtlas>(atlasPath);
        if (!atlas)
        {
            LOG_ERROR("❌ Failed to load sprite atlas: " + atlasPath);
            atlas_.reset();
            animator_ = Ressources::SpriteAnimator{};
            currentAnimation_.Clear();
            SetTexture(nullptr);
            return false;
        }

        atlas_ = std::move(atlas);
        animator_ = Ressources::SpriteAnimator{};

        for (const auto& animation : atlas_->GetAnimations())
        {
            animator_.AddAnimation(animation);
        }

        if (!defaultAnimation.IsEmpty() && animator_.HasAnimation(defaultAnimation))
        {
            currentAnimation_ = defaultAnimation;
        }
        else if (!atlas_->GetAnimations().empty())
        {
            currentAnimation_ = atlas_->GetAnimations().front().Name;
        }
        else
        {
            currentAnimation_.Clear();
        }

        ApplyCurrentFrame(true);
        return true;
    }

    void SpriteAnimatorComponent::Play()
    {
        if (currentAnimation_.IsEmpty())
        {
            LOG_WARNING("⚠️ No animation selected — cannot play.");
            return;
        }

        if (!animator_.HasAnimation(currentAnimation_))
        {
            LOG_WARNING("⚠️ Animation not found in atlas: " + currentAnimation_);
            return;
        }

        animator_.Play(currentAnimation_);
        ApplyCurrentFrame(false);
    }

    void SpriteAnimatorComponent::Play(const String& animationName)
    {
        if (animationName.IsEmpty())
        {
            LOG_WARNING("⚠️ Cannot play animation with empty name.");
            return;
        }

        currentAnimation_ = animationName;
        Play();
    }

    void SpriteAnimatorComponent::Stop()
    {
        animator_.Stop();
        ApplyCurrentFrame(true);
    }

    void SpriteAnimatorComponent::ApplyCurrentFrame(bool allowFallbackToDefault)
    {
        const Ressources::SpriteFrame* frame = animator_.GetCurrentFrame();

        if (!frame && allowFallbackToDefault && atlas_ && !currentAnimation_.IsEmpty())
        {
            const Ressources::SpriteAnimation* animation = atlas_->GetAnimation(currentAnimation_);
            if (animation && !animation->Frames.empty())
            {
                frame = &animation->Frames.front();
            }
        }

        if (frame)
        {
            SetTexture(frame->GetTexture());
            SetUVRect(frame->GetUVRect());
        }
        else
        {
            SetTexture(nullptr);
        }
    }
}
