#include "Engine/Render/SpriteAnimator.h"

namespace BixEngine::Render
{
    void SpriteAnimator::AddAnimation(const SpriteAnimation& animation)
    {
        animations_.push_back(animation);
    }

    void SpriteAnimator::Play(const String& name)
    {
        for (auto& anim : animations_)
        {
            if (anim.Name == name)
            {
                currentAnim_ = &anim;
                timeAccumulator_ = 0.0f;
                currentFrameIndex_ = 0;
                bPlaying_ = true;
                
                return;
            }
        }
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
    }

    void SpriteAnimator::Update(float deltaTime)
    {
        if (!bPlaying_ || !currentAnim_ || currentAnim_->Frames.empty())
            return;

        timeAccumulator_ += deltaTime * speedMultiplier_;

        float frameDuration = 1.0f / currentAnim_->FrameRate;
        while (timeAccumulator_ >= frameDuration)
        {
            timeAccumulator_ -= frameDuration;
            currentFrameIndex_++;

            if (currentFrameIndex_ >= currentAnim_->Frames.size())
            {
                if (currentAnim_->bLoop)
                    currentFrameIndex_ = 0;
                else
                {
                    currentFrameIndex_ = currentAnim_->Frames.size() - 1;
                    bPlaying_ = false;
                    break;
                }
            }
        }
    }

    const SpriteFrame* SpriteAnimator::GetCurrentFrame() const noexcept
    {
        if (!currentAnim_ || currentAnim_->Frames.empty())
            return nullptr;
        
        return &currentAnim_->Frames[currentFrameIndex_];
    }
}
