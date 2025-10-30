#include "Engine/Render/SpriteAnimator.h"

#include <algorithm>
#include <cmath>

namespace BixEngine::Render
{
    namespace
    {
        constexpr size_t kInvalidAnimationIndex = std::numeric_limits<size_t>::max();

        [[nodiscard]] float Clamp01(float value) noexcept
        {
            return std::clamp(value, 0.0f, 1.0f);
        }
    }

    SpriteAnimator::SpriteAnimator()
        : currentAnimIndex_(kInvalidAnimationIndex)
        , playbackTime_(0.0f)
        , currentFrameIndex_(0)
        , bPlaying_(false)
        , speedMultiplier_(1.0f)
        , playbackDirection_(SpritePlaybackDirection::Forward)
        , bPendingPingPongBack_(false)
        , bPlayQueuedAfterCurrent_(false)
        , bForceSinglePlayback_(false)
    {
    }

    void SpriteAnimator::AddAnimation(SpriteAnimation animation)
    {
        if (animation.Name.IsEmpty())
            return;

        const auto found = animationLookup_.find(animation.Name);
        if (found != animationLookup_.end())
        {
            const size_t index = found->second;
            animations_[index] = std::move(animation);
            if (eventTriggerState_.size() <= index)
                eventTriggerState_.resize(index + 1);
            eventTriggerState_[index] = std::vector<bool>(animations_[index].Events.size(), false);
            if (currentAnimIndex_ == index)
            {
                currentFrameIndex_ = 0;
                playbackTime_ = 0.0f;
            }
            return;
        }

        const size_t index = animations_.size();
        animationLookup_[animation.Name] = index;
        if (eventTriggerState_.size() <= index)
            eventTriggerState_.resize(index + 1);
        eventTriggerState_[index] = std::vector<bool>(animation.Events.size(), false);
        animations_.push_back(std::move(animation));
    }

    void SpriteAnimator::RemoveAnimation(const String& name)
    {
        const auto found = animationLookup_.find(name);
        if (found == animationLookup_.end())
            return;

        const size_t removeIndex = found->second;
        animations_.erase(animations_.begin() + static_cast<std::ptrdiff_t>(removeIndex));
        eventTriggerState_.erase(eventTriggerState_.begin() + static_cast<std::ptrdiff_t>(removeIndex));
        animationLookup_.erase(found);

        for (auto& [animName, idx] : animationLookup_)
        {
            if (idx > removeIndex)
                --idx;
        }

        if (currentAnimIndex_ == removeIndex)
        {
            Stop();
        }
        else if (currentAnimIndex_ > removeIndex && currentAnimIndex_ != kInvalidAnimationIndex)
        {
            --currentAnimIndex_;
        }
    }

    bool SpriteAnimator::HasAnimation(const String& name) const noexcept
    {
        return animationLookup_.find(name) != animationLookup_.end();
    }

    void SpriteAnimator::Play(const String& name, bool bReverse)
    {
        const auto found = animationLookup_.find(name);
        if (found == animationLookup_.end())
            return;

        const size_t index = found->second;
        currentAnimIndex_ = index;
        currentFrameIndex_ = bReverse ? animations_[index].Frames.empty() ? 0 : animations_[index].Frames.size() - 1 : 0;
        playbackTime_ = 0.0f;
        bPlaying_ = true;
        playbackDirection_ = bReverse || animations_[index].bReverse ? SpritePlaybackDirection::Reverse : SpritePlaybackDirection::Forward;
        bPendingPingPongBack_ = false;
        bPlayQueuedAfterCurrent_ = false;
        queuedAnimation_.Clear();
        bForceSinglePlayback_ = false;
        if (eventTriggerState_.size() <= index)
            eventTriggerState_.resize(index + 1);
        eventTriggerState_[index] = std::vector<bool>(animations_[index].Events.size(), false);
        bForceSinglePlayback_ = false;
    }

    void SpriteAnimator::PlayOnceThen(const String& animName, const String& nextAnim)
    {
        Play(animName, false);
        queuedAnimation_ = nextAnim;
        bPlayQueuedAfterCurrent_ = true;
        bForceSinglePlayback_ = true;
    }

    void SpriteAnimator::Pause()
    {
        bPlaying_ = false;
    }

    void SpriteAnimator::Stop()
    {
        bPlaying_ = false;
        playbackTime_ = 0.0f;
        currentFrameIndex_ = 0;
        currentAnimIndex_ = kInvalidAnimationIndex;
        playbackDirection_ = SpritePlaybackDirection::Forward;
        bPendingPingPongBack_ = false;
        queuedAnimation_.Clear();
        bPlayQueuedAfterCurrent_ = false;
        bForceSinglePlayback_ = false;
    }

    void SpriteAnimator::Update(float deltaTime, std::optional<float> deltaTimeOverride)
    {
        const SpriteAnimation* currentAnimation = GetCurrentAnimation();
        if (!bPlaying_ || !currentAnimation || currentAnimation->Frames.empty())
            return;

        const float frameRate = currentAnimation->FrameRate * currentAnimation->SpeedMultiplier * speedMultiplier_;
        if (frameRate <= 0.0f)
            return;

        const float clampedDelta = deltaTimeOverride.has_value() ? deltaTimeOverride.value() : deltaTime;
        if (clampedDelta <= 0.0f)
            return;

        EvaluateNextFrame(clampedDelta);
    }

    const SpriteFrame* SpriteAnimator::GetCurrentFrame() const noexcept
    {
        const SpriteAnimation* currentAnimation = GetCurrentAnimation();
        if (!currentAnimation || currentAnimation->Frames.empty())
            return nullptr;

        return &currentAnimation->Frames[currentFrameIndex_];
    }

    String SpriteAnimator::GetCurrentAnimationName() const
    {
        const SpriteAnimation* animation = GetCurrentAnimation();
        if (!animation)
            return {};

        return animation->Name;
    }

    void SpriteAnimator::SetSpeed(float multiplier) noexcept
    {
        speedMultiplier_ = std::max(0.0f, multiplier);
    }

    void SpriteAnimator::SetAnimationSpeed(const String& name, float multiplier)
    {
        const auto found = animationLookup_.find(name);
        if (found == animationLookup_.end())
            return;

        animations_[found->second].SpeedMultiplier = std::max(0.0f, multiplier);
    }

    void SpriteAnimator::Seek(float normalizedTime)
    {
        const SpriteAnimation* currentAnimation = GetCurrentAnimation();
        if (!currentAnimation || currentAnimation->Frames.empty())
            return;

        const float clamped = Clamp01(normalizedTime);
        const size_t lastFrameIndex = currentAnimation->Frames.size() > 0 ? currentAnimation->Frames.size() - 1 : 0;
        currentFrameIndex_ = static_cast<size_t>(std::round(clamped * static_cast<float>(lastFrameIndex)));
        currentFrameIndex_ = std::min(currentFrameIndex_, lastFrameIndex);
        playbackTime_ = 0.0f;
        if (currentAnimIndex_ < eventTriggerState_.size())
        {
            eventTriggerState_[currentAnimIndex_].assign(currentAnimation->Events.size(), false);
        }
    }

    float SpriteAnimator::GetNormalizedTime() const noexcept
    {
        const SpriteAnimation* animation = GetCurrentAnimation();
        if (!animation || animation->Frames.size() <= 1)
            return 0.0f;

        const float maxIndex = static_cast<float>(animation->Frames.size() - 1);
        return static_cast<float>(currentFrameIndex_) / maxIndex;
    }

    void SpriteAnimator::SetPlaybackDirection(SpritePlaybackDirection direction) noexcept
    {
        playbackDirection_ = direction;
    }

    void SpriteAnimator::SetPingPong(const String& name, bool enabled)
    {
        const auto found = animationLookup_.find(name);
        if (found == animationLookup_.end())
            return;

        animations_[found->second].bPingPong = enabled;
    }

    void SpriteAnimator::AddEvent(const String& animationName, SpriteEvent event)
    {
        if (!event.IsValid())
            return;

        const auto found = animationLookup_.find(animationName);
        if (found == animationLookup_.end())
            return;

        auto& anim = animations_[found->second];
        anim.Events.push_back(std::move(event));
        if (eventTriggerState_.size() <= found->second)
            eventTriggerState_.resize(found->second + 1);
        eventTriggerState_[found->second] = std::vector<bool>(anim.Events.size(), false);
    }

    void SpriteAnimator::ClearEvents(const String& animationName)
    {
        const auto found = animationLookup_.find(animationName);
        if (found == animationLookup_.end())
            return;

        animations_[found->second].Events.clear();
        if (eventTriggerState_.size() > found->second)
        {
            eventTriggerState_[found->second].clear();
        }
    }

    const SpriteAnimation* SpriteAnimator::GetCurrentAnimation() const noexcept
    {
        if (currentAnimIndex_ == kInvalidAnimationIndex || currentAnimIndex_ >= animations_.size())
            return nullptr;

        return &animations_[currentAnimIndex_];
    }

    void SpriteAnimator::EvaluateNextFrame(float deltaTime)
    {
        auto* animation = const_cast<SpriteAnimation*>(GetCurrentAnimation());
        if (!animation)
            return;

        const float effectiveRate = animation->FrameRate * animation->SpeedMultiplier * speedMultiplier_;
        if (effectiveRate <= 0.0f)
            return;

        const float frameDuration = 1.0f / effectiveRate;
        if (frameDuration <= 0.0f)
            return;

        playbackTime_ += deltaTime;

        while (playbackTime_ >= frameDuration)
        {
            playbackTime_ -= frameDuration;

            const size_t prevFrame = currentFrameIndex_;
            const bool wasPlayingForward = playbackDirection_ == SpritePlaybackDirection::Forward;

            if (playbackDirection_ == SpritePlaybackDirection::Forward)
            {
                ++currentFrameIndex_;
                if (currentFrameIndex_ >= animation->Frames.size())
                {
                    if (animation->bPingPong || bPendingPingPongBack_)
                    {
                        playbackDirection_ = SpritePlaybackDirection::Reverse;
                        bPendingPingPongBack_ = true;
                        currentFrameIndex_ = animation->Frames.size() > 1 ? animation->Frames.size() - 2 : 0;
                    }
                    else if (animation->bLoop && !bForceSinglePlayback_)
                    {
                        currentFrameIndex_ = 0;
                    }
                    else
                    {
                        currentFrameIndex_ = animation->Frames.size() - 1;
                        bPlaying_ = false;
                        DispatchFrameEvents(currentAnimIndex_, *animation, prevFrame, currentFrameIndex_, false);
                        if (bPlayQueuedAfterCurrent_ && HasAnimation(queuedAnimation_))
                        {
                            Play(queuedAnimation_);
                            bForceSinglePlayback_ = false;
                        }
                        else
                        {
                            bForceSinglePlayback_ = false;
                        }
                        return;
                    }
                }
            }
            else
            {
                if (currentFrameIndex_ == 0)
                {
                    if (animation->bPingPong)
                    {
                        playbackDirection_ = SpritePlaybackDirection::Forward;
                        currentFrameIndex_ = animation->Frames.size() > 1 ? 1 : 0;
                    }
                    else if (animation->bLoop && !bForceSinglePlayback_)
                    {
                        currentFrameIndex_ = animation->Frames.size() > 0 ? animation->Frames.size() - 1 : 0;
                    }
                    else
                    {
                        bPlaying_ = false;
                        DispatchFrameEvents(currentAnimIndex_, *animation, prevFrame, currentFrameIndex_, false);
                        if (bPlayQueuedAfterCurrent_ && HasAnimation(queuedAnimation_))
                        {
                            Play(queuedAnimation_);
                            bForceSinglePlayback_ = false;
                        }
                        else
                        {
                            bForceSinglePlayback_ = false;
                        }
                        return;
                    }
                }
                else
                {
                    --currentFrameIndex_;
                }
            }

            DispatchFrameEvents(currentAnimIndex_, *animation, prevFrame, currentFrameIndex_, wasPlayingForward);
        }
    }

    void SpriteAnimator::DispatchFrameEvents(size_t animationIndex, const SpriteAnimation& animation, size_t fromFrame, size_t toFrame, bool inclusiveStart)
    {
        if (animation.Events.empty())
            return;

        const bool forward = playbackDirection_ == SpritePlaybackDirection::Forward;
        if (eventTriggerState_.size() <= animationIndex)
            return;

        auto& triggers = eventTriggerState_[animationIndex];
        if (triggers.size() != animation.Events.size())
            triggers.assign(animation.Events.size(), false);

        for (size_t eventIndex = 0; eventIndex < animation.Events.size(); ++eventIndex)
        {
            const auto& evt = animation.Events[eventIndex];
            if (!evt.IsValid())
                continue;

            bool shouldTrigger = false;
            if (evt.NormalizedTime >= 0.0f)
            {
                const float normalized = GetNormalizedTime();
                shouldTrigger = forward ? normalized >= evt.NormalizedTime : normalized <= evt.NormalizedTime;
            }
            else
            {
                if (forward)
                {
                    const size_t start = inclusiveStart ? fromFrame : fromFrame + 1;
                    shouldTrigger = evt.FrameIndex >= start && evt.FrameIndex <= toFrame;
                }
                else
                {
                    size_t start = inclusiveStart ? fromFrame : (fromFrame == 0 ? 0 : fromFrame - 1);
                    shouldTrigger = evt.FrameIndex <= start && evt.FrameIndex >= toFrame;
                }
            }

            if (shouldTrigger)
            {
                if ((!evt.bTriggerOnce || !triggers[eventIndex]) && evt.Callback)
                {
                    evt.Callback();
                    triggers[eventIndex] = true;
                }
                else if (!evt.Callback)
                {
                    triggers[eventIndex] = true;
                }
            }
            else if (!evt.bTriggerOnce)
            {
                triggers[eventIndex] = false;
            }
        }
    }
}
