#pragma once

#include <limits>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include "Core/Containers/String.h"
#include "Engine/Render/SpriteEvent.h"
#include "Engine/Render/SpriteFrame.h"

namespace BixEngine::Render
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

        /** Adds or replaces an animation. */
        void AddAnimation(SpriteAnimation animation);
        /** Removes a registered animation. */
        void RemoveAnimation(const String& name);
        /** Returns true if the animator owns an animation with the provided name. */
        [[nodiscard]] bool HasAnimation(const String& name) const noexcept;

        /** Plays the target animation with optional reverse flag. */
        void Play(const String& name, bool bReverse = false);
        /** Plays an animation once before chaining to another animation. */
        void PlayOnceThen(const String& animName, const String& nextAnim);
        /** Pauses the currently playing animation. */
        void Pause();
        /** Stops the current animation and resets playback state. */
        void Stop();

        /** Updates internal state using the provided delta time. */
        void Update(float deltaTime, std::optional<float> deltaTimeOverride = std::nullopt);

        /** Returns the current frame handle. */
        [[nodiscard]] const SpriteFrame* GetCurrentFrame() const noexcept;
        /** Returns the index of the current frame. */
        [[nodiscard]] size_t GetCurrentFrameIndex() const noexcept { return currentFrameIndex_; }
        /** Returns true when an animation is actively playing. */
        [[nodiscard]] bool IsPlaying() const noexcept { return bPlaying_; }
        /** Returns the name of the current animation or an empty string. */
        [[nodiscard]] String GetCurrentAnimationName() const;

        /** Overrides the global speed multiplier applied to every animation. */
        void SetSpeed(float multiplier) noexcept;
        [[nodiscard]] float GetSpeed() const noexcept { return speedMultiplier_; }

        /** Sets a per-animation speed override. */
        void SetAnimationSpeed(const String& name, float multiplier);

        /** Seeks to the provided normalized time (0..1). */
        void Seek(float normalizedTime);
        /** Returns the normalized time of the current animation (0..1). */
        [[nodiscard]] float GetNormalizedTime() const noexcept;

        /** Allows external systems to manually change playback direction. */
        void SetPlaybackDirection(SpritePlaybackDirection direction) noexcept;
        [[nodiscard]] SpritePlaybackDirection GetPlaybackDirection() const noexcept { return playbackDirection_; }

        /** Enables or disables ping-pong playback for the given animation. */
        void SetPingPong(const String& name, bool enabled);

        /** Registers a runtime event for the target animation. */
        void AddEvent(const String& animationName, SpriteEvent event);

        /** Clears runtime events registered on the target animation. */
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
