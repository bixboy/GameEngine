#pragma once

#include <functional>
#include <optional>
#include <vector>
#include <limits>

#include "Game/Components/Component.h"
#include "Engine/Render/SpriteAnimator.h"
#include "Game/Components/SpriteComponent.h"
#include "Core/Math/Rect.h"
#include "SDL3/SDL.h"
#include "Reflection/ReflectionMacros.h"
#include "SpriteAnimatorComponent.generated.h"


namespace BixEngine::Game
{
    class SpriteAnimatorComponent;

    /** State definition used by the animator component state machine. */
    struct SpriteAnimationState
    {
        String StateName;
        String AnimationName;
        float BlendDuration = 0.15f;
        bool bLoop = true;
        std::function<bool(const SpriteAnimatorComponent&)> Condition;
    };

    BCLASS()
    class SpriteAnimatorComponent : public Component
    {
        GENERATED_BODY()

    public:
        explicit SpriteAnimatorComponent(Actor* owner);

        void BeginPlay() override;
        void Update(float deltaTime) override;

        /** Registers a new animation within the internal animator. */
        void AddAnimation(Render::SpriteAnimation animation);

        /** Directly plays an animation by name. */
        void Play(const String& name);
        /** Queues an animation to play once before returning to the default state. */
        void PlayOnceThen(const String& name, const String& next);
        /** Plays an animation for a fixed duration before restoring the previous state. */
        void PlayForDuration(const String& name, float seconds, const String& fallbackState);

        /** Pauses the currently active animation. */
        void Pause();
        /** Stops playback entirely. */
        void Stop();

        /** Sets the playback speed multiplier. */
        void SetPlaybackSpeed(float speed);
        /** Overrides the delta time used for animation playback. */
        void SetDeltaTimeOverride(std::optional<float> overrideDelta) noexcept { deltaTimeOverride_ = overrideDelta; }

        /** Returns true when an animation is currently playing. */
        [[nodiscard]] bool IsPlaying() const noexcept;

        /** Adds an additional sprite layer that will be updated each frame. */
        void AddSpriteLayer(SpriteComponent* sprite);
        /** Removes an existing sprite layer binding. */
        void RemoveSpriteLayer(SpriteComponent* sprite);
        /** Clears all registered sprite layers. */
        void ClearSpriteLayers();

        /** Registers a new animation state for automatic transitions. */
        void AddState(SpriteAnimationState state);
        /** Sets the default state used when no transitions match. */
        void SetDefaultState(String stateName);

        /** Enables or disables the debug overlay. */
        void EnableDebug(bool enabled) noexcept { bDebugMode_ = enabled; }
        [[nodiscard]] bool IsDebugEnabled() const noexcept { return bDebugMode_; }

        /** Event triggered when a new animation starts playing. */
        std::function<void(const String&)> OnAnimationStart;
        /** Event triggered when the current animation completes. */
        std::function<void(const String&)> OnAnimationEnd;
        /** Event triggered when the frame index changes. */
        std::function<void(const String&, size_t)> OnFrameChanged;

        void SetTargetSprite(SpriteComponent* sprite) noexcept;

    protected:
        struct SpriteBinding
        {
            SpriteComponent* Component = nullptr;
            Render::Texture* CurrentTexture = nullptr;
            Math::Rect CurrentUV{};
            SDL_Color BaseTint{255, 255, 255, 255};
        };

        void ApplyFrame(SpriteBinding& binding, const Render::SpriteFrame* frame, float alphaWeight);
        void EvaluateStateMachine();
        void UpdateBlend(float deltaTime);
        void UpdateDebugWindow();

        Render::SpriteAnimator primaryAnimator_;
        Render::SpriteAnimator blendAnimator_;
        std::vector<SpriteBinding> spriteBindings_;

        String defaultState_;
        String activeState_;
        String queuedState_;
        std::vector<SpriteAnimationState> states_;

        bool bBlending_ = false;
        float blendTimer_ = 0.0f;
        float blendDuration_ = 0.0f;

        std::optional<float> deltaTimeOverride_;
        float temporaryTimer_ = 0.0f;
        bool bUsingTemporary_ = false;
        String fallbackState_;

        bool bDebugMode_ = false;

        size_t lastFrameIndex_ = std::numeric_limits<size_t>::max();
        bool wasPlaying_ = false;
    };
}
