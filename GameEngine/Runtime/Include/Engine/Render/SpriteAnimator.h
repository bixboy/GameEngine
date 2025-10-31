#pragma once

#include <limits>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include "Core/Containers/String.h"
#include "Engine/Render/SpriteEvent.h"
#include "Engine/Render/SpriteFrame.h"

namespace BixEngine::Ressources
{
    /**
     * @brief Describes the playback direction of a sprite animation.
     */
    enum class SpritePlaybackDirection : int8_t
    {
        Forward = 1,
        Reverse = -1
    };

    /**
     * @brief Describes a single sprite animation sequence.
     */
    struct SpriteAnimation
    {
        String Name;
        std::vector<SpriteFrame> Frames;
        bool bLoop = true;
        float FrameRate = 12.0f;
        float SpeedMultiplier = 1.0f;
        bool bPingPong = false;
        bool bReverse = false;
        std::vector<SpriteEvent> Events;
    };

    /**
     * @brief High level controller capable of complex sprite sheet animation playback.
     */
    class SpriteAnimator
    {
    public:
        SpriteAnimator();

        void AddAnimation(SpriteAnimation animation);
        void RemoveAnimation(const String& name);
        [[nodiscard]] bool HasAnimation(const String& name) const noexcept;

        void Play(const String& name, bool bReverse = false);
        void PlayOnceThen(const String& animName, const String& nextAnim);
        void Pause();
        void Stop();

        void Update(float deltaTime, std::optional<float> deltaTimeOverride = std::nullopt);

        [[nodiscard]] const SpriteFrame* GetCurrentFrame() const noexcept;
        
        [[nodiscard]] size_t GetCurrentFrameIndex() const noexcept { return currentFrameIndex_; }
        
        [[nodiscard]] bool IsPlaying() const noexcept { return bPlaying_; }
        
        [[nodiscard]] String GetCurrentAnimationName() const;

        void SetSpeed(float multiplier) noexcept;
        [[nodiscard]] float GetSpeed() const noexcept { return speedMultiplier_; }

        void SetAnimationSpeed(const String& name, float multiplier);

        void Seek(float normalizedTime);
        [[nodiscard]] float GetNormalizedTime() const noexcept;

        void SetPlaybackDirection(SpritePlaybackDirection direction) noexcept;
        [[nodiscard]] SpritePlaybackDirection GetPlaybackDirection() const noexcept { return playbackDirection_; }

        void SetPingPong(const String& name, bool enabled);

        void AddEvent(const String& animationName, SpriteEvent event);

        void ClearEvents(const String& animationName);

    private:
        [[nodiscard]] const SpriteAnimation* GetCurrentAnimation() const noexcept;
        void EvaluateNextFrame(float deltaTime);
        void DispatchFrameEvents(size_t animationIndex, const SpriteAnimation& animation, size_t fromFrame, size_t toFrame, bool inclusiveStart);

        std::vector<SpriteAnimation> animations_;
        std::unordered_map<String, size_t> animationLookup_;
        std::vector<std::vector<bool>> eventTriggerState_;

        size_t currentAnimIndex_;
        float playbackTime_;
        size_t currentFrameIndex_;
        bool bPlaying_;
        float speedMultiplier_;
        SpritePlaybackDirection playbackDirection_;
        bool bPendingPingPongBack_;

        String queuedAnimation_;
        bool bPlayQueuedAfterCurrent_;
        bool bForceSinglePlayback_;
    };
}
