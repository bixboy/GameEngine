#include "Game/Components/SpriteAnimatorComponent.h"

#include "Core/Logger.h"
#include "Engine/Ressources/ResourceManager.h"
#include "Engine/Ressources/SpriteAtlas.h"
#include "Game/Actor.h"
#include <algorithm>

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

        if (!atlas_)
            return;

        animator_.Update(std::max(0.0f, deltaTime));
        ApplyCurrentFrame(false);
    }

    bool SpriteAnimatorComponent::LoadSpriteAtlas(const String& atlasPath, const String& defaultAnimation)
    {
        auto& resourceManager = resources::ResourceManager::Get();
        auto atlas = resourceManager.Get<resources::SpriteAtlas>(atlasPath);
        if (!atlas)
        {
            LOG_ERROR("❌ Failed to load sprite atlas: " + atlasPath);
            atlas_.reset();
            animator_.SetSpriteAtlas(nullptr);
            currentAnimation_.Clear();
            SetTexture(nullptr);
            return false;
        }

        atlas_ = std::move(atlas);
        animator_.SetSpriteAtlas(atlas_);

        if (!defaultAnimation.IsEmpty() && atlas_->GetAnimation(defaultAnimation))
        {
            currentAnimation_ = defaultAnimation;
        }
        else if (!atlas_->GetAnimations().empty())
        {
            currentAnimation_ = atlas_->GetAnimations().front().name;
        }
        else
        {
            currentAnimation_.Clear();
        }

        if (!currentAnimation_.IsEmpty())
        {
            animator_.Play(currentAnimation_);
        }
        else
        {
            animator_.Stop();
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

        if (!atlas_ || !atlas_->GetAnimation(currentAnimation_))
        {
            LOG_WARNING("⚠️ Animation not found in atlas: " + currentAnimation_);
            return;
        }

        if (!animator_.Play(currentAnimation_))
        {
            LOG_WARNING("⚠️ Failed to start animation: " + currentAnimation_);
            return;
        }

        ApplyCurrentFrame(false);
    }

    void SpriteAnimatorComponent::Play(const String& animationName)
    {
        if (animationName.IsEmpty())
        {
            LOG_WARNING("⚠️ Cannot play animation with empty name.");
            return;
        }

        if (!atlas_ || !atlas_->GetAnimation(animationName))
        {
            LOG_WARNING("⚠️ Animation not found in atlas: " + animationName);
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
        const resources::SpriteFrame* frame = animator_.GetCurrentFrame();

        if (!frame && allowFallbackToDefault && atlas_ && !currentAnimation_.IsEmpty())
        {
            const resources::SpriteAnimation* animation = atlas_->GetAnimation(currentAnimation_);
            if (animation && !animation->frameIndices.empty())
            {
                frame = atlas_->GetFrame(animation->frameIndices.front());
            }
        }

        if (frame && frame->IsValid())
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
