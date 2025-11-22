#include "Render/SpriteAnimator.h"
#include <algorithm>
#include "Ressources/RessourcesClass/SpriteAtlas.h"


namespace BixEngine::resources
{
    namespace
    {
        [[nodiscard]] size_t AdvanceFrame(float& accumulatedTime, float deltaTime, float frameRate, size_t currentFrame, size_t frameCount, bool loop)
        {
            if (frameRate <= 0.0f || frameCount == 0)
                return currentFrame;

            const float frameDuration = 1.0f / frameRate;
            accumulatedTime += std::max(0.0f, deltaTime);

            while (accumulatedTime >= frameDuration)
            {
                accumulatedTime -= frameDuration;
                if (currentFrame + 1 < frameCount)
                {
                    ++currentFrame;
                }
                else
                {
                    if (loop)
                    {
                        currentFrame = 0;
                    }
                    else
                    {
                        currentFrame = frameCount - 1;
                        accumulatedTime = 0.0f;
                        break;
                    }
                }
            }

            return currentFrame;
        }
    }

    void SpriteAnimator::SetSpriteAtlas(std::shared_ptr<SpriteAtlas> atlas) noexcept
    {
        atlas_ = atlas;
        Stop();
    }

    bool SpriteAnimator::Play(const String& animationName)
    {
        currentAnimationName_ = animationName;
        currentAnimation_ = ResolveAnimation();

        if (!currentAnimation_ || currentAnimation_->frameIndices.empty())
        {
            Stop();
            return false;
        }

        accumulatedTime_ = 0.0f;
        currentFrameIndex_ = 0;
        isPlaying_ = true;
        return true;
    }

    void SpriteAnimator::Stop() noexcept
    {
        isPlaying_ = false;
        accumulatedTime_ = 0.0f;
        currentFrameIndex_ = 0;
        currentAnimation_ = nullptr;
        currentAnimationName_.Clear();
    }

    void SpriteAnimator::Update(float deltaTime) noexcept
    {
        if (!isPlaying_)
            return;

        currentAnimation_ = ResolveAnimation();
        if (!currentAnimation_ || currentAnimation_->frameIndices.empty())
        {
            Stop();
            return;
        }

        currentFrameIndex_ = AdvanceFrame(accumulatedTime_, deltaTime, currentAnimation_->frameRate, currentFrameIndex_,
                                          currentAnimation_->frameIndices.size(), currentAnimation_->loop);

        if (!currentAnimation_->loop && currentFrameIndex_ + 1 == currentAnimation_->frameIndices.size())
        {
            // Stop once the final frame has been displayed without looping.
            // The caller can continue to read the last frame until Play() is called again.
            if (accumulatedTime_ == 0.0f)
                isPlaying_ = false;
        }
    }

    const SpriteFrame* SpriteAnimator::GetCurrentFrame() const noexcept
    {
        const SpriteAnimation* animation = ResolveAnimation();
        if (!animation || animation->frameIndices.empty())
            return nullptr;

        const size_t frameIndex = std::min(currentFrameIndex_, animation->frameIndices.size() - 1);

        if (auto atlas = atlas_.lock())
            return atlas->GetFrame(animation->frameIndices[frameIndex]);

        return nullptr;
    }

    const SpriteAnimation* SpriteAnimator::ResolveAnimation() const noexcept
    {
        auto atlas = atlas_.lock();
        if (!atlas || currentAnimationName_.IsEmpty())
            return nullptr;

        return atlas->GetAnimation(currentAnimationName_);
    }
}
