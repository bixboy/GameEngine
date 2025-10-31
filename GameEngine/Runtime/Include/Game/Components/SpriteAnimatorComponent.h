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

    struct SpriteAnimationClipConfig
    {
        String Name{"Animation"};
        String TexturePath{};
        int Columns = 1;
        int Rows = 1;
        int StartFrame = 0;
        int FrameCount = 0;
        int Padding = 0;
        int Margin = 0;
        float FrameRate = 12.0f;
        bool bLoop = true;
    };

    BCLASS()
    class SpriteAnimatorComponent : public Component
    {
        GENERATED_BODY()

    public:
        explicit SpriteAnimatorComponent(Actor* owner);

        void BeginPlay() override;
        void Update(float deltaTime) override;

        [[nodiscard]] String GetTypeName() const override { return "SpriteAnimatorComponent"; }

        void AddAnimation(const Render::SpriteAnimation& animation);

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

        void SetClips(std::vector<SpriteAnimationClipConfig> clips);
        void SetInitialClip(String clipName) noexcept;
        void SetAutoPlay(bool enabled) noexcept;
        [[nodiscard]] bool IsAutoPlayEnabled() const noexcept { return bAutoPlayOnLoad_; }
        [[nodiscard]] const std::vector<SpriteAnimationClipConfig>& GetClips() const noexcept { return clipConfigs_; }
        void ReloadAnimations();

        void DrawInspectorUI() override;

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
        void TryAutoPlay();

        bool BuildAnimationFromClip(const SpriteAnimationClipConfig& clipConfig, SDL_Renderer* sdlRenderer);

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

        std::vector<SpriteAnimationClipConfig> clipConfigs_;
        bool clipConfigsDirty_ = false;
        String initialClipName_{"Animation"};
        bool bAutoPlayOnLoad_ = true;
    
        size_t lastFrameIndex_ = std::numeric_limits<size_t>::max();
        bool wasPlaying_ = false;
    };
}
