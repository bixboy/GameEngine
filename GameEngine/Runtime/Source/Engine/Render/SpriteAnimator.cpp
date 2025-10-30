#include "Engine/Render/SpriteAnimator.h"
#include <limits>
#include <utility>

namespace BixEngine::Render
{
    namespace
    {
        constexpr size_t kInvalidAnimationIndex = std::numeric_limits<size_t>::max();
    }

    void SpriteAnimator::AddAnimation(SpriteAnimation animation)
    {
        if (animation.Name.IsEmpty())
            return;

        auto found = animationLookup_.find(animation.Name);
        if (found != animationLookup_.end())
        {
            animations_[found->second] = std::move(animation);

            if (currentAnimIndex_ == found->second)
            {
                timeAccumulator_ = 0.0f;
                currentFrameIndex_ = 0;
            }

            return;
        }

        const size_t index = animations_.size();
        animationLookup_[animation.Name] = index;
        animations_.push_back(std::move(animation));
    }

    void SpriteAnimator::Play(const String& name)
    {
        const auto found = animationLookup_.find(name);
        if (found == animationLookup_.end())
            return;

        currentAnimIndex_ = found->second;
        timeAccumulator_ = 0.0f;
        currentFrameIndex_ = 0;
        bPlaying_ = true;
    }

    void SpriteAnimator::Pause()
    {
        bPlaying_ = false;
    }

    void SpriteAnimator::Stop()
    {
        bPlaying_ = false;
        timeAccumulator_ = 0.0f;
        currentFrameIndex_ = 0;
        currentAnimIndex_ = kInvalidAnimationIndex;
    }

    void SpriteAnimator::Update(float deltaTime)
    {
        const SpriteAnimation* currentAnimation = GetCurrentAnimation();
        if (!bPlaying_ || !currentAnimation || currentAnimation->Frames.empty())
            return;

        timeAccumulator_ += deltaTime * speedMultiplier_;

        if (currentAnimation->FrameRate <= 0.0f)
            return;

        const float frameDuration = 1.0f / currentAnimation->FrameRate;
        while (timeAccumulator_ >= frameDuration)
        {
            timeAccumulator_ -= frameDuration;
            currentFrameIndex_++;

            if (currentFrameIndex_ >= currentAnimation->Frames.size())
            {
                if (currentAnimation->bLoop)
                    currentFrameIndex_ = 0;
                else
                {
                    currentFrameIndex_ = currentAnimation->Frames.size() - 1;
                    bPlaying_ = false;
                    break;
                }
            }
        }
    }

    const SpriteFrame* SpriteAnimator::GetCurrentFrame() const noexcept
    {
        const SpriteAnimation* currentAnimation = GetCurrentAnimation();
        if (!currentAnimation || currentAnimation->Frames.empty())
            return nullptr;

        return &currentAnimation->Frames[currentFrameIndex_];
    }

    const SpriteAnimation* SpriteAnimator::GetCurrentAnimation() const noexcept
    {
        if (currentAnimIndex_ == kInvalidAnimationIndex || currentAnimIndex_ >= animations_.size())
            return nullptr;

        return &animations_[currentAnimIndex_];
    }
}
