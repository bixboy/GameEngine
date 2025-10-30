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

        void AddAnimation(Render::SpriteAnimation animation);

        void Play(const String& name);
        void PlayOnceThen(const String& name, const String& next);
        void PlayForDuration(const String& name, float seconds, const String& fallbackState);

        void Pause();
        void Stop();

        void SetPlaybackSpeed(float speed);
        void SetDeltaTimeOverride(std::optional<float> overrideDelta) noexcept { deltaTimeOverride_ = overrideDelta; }

        [[nodiscard]] bool IsPlaying() const noexcept;

        void AddSpriteLayer(SpriteComponent* sprite);
        void RemoveSpriteLayer(SpriteComponent* sprite);
        void ClearSpriteLayers();

        void AddState(SpriteAnimationState state);
        void SetDefaultState(String stateName);

        void EnableDebug(bool enabled) noexcept { bDebugMode_ = enabled; }
        [[nodiscard]] bool IsDebugEnabled() const noexcept { return bDebugMode_; }

        std::function<void(const String&)> OnAnimationStart;
        std::function<void(const String&)> OnAnimationEnd;
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
