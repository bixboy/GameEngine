#pragma once
#include <memory>
#include "Containers/String.h"

namespace BixEngine::resources
{
    class SpriteAtlas;
    struct SpriteAnimation;
    struct SpriteFrame;

     
    class SpriteAnimator
    {
    public:
        SpriteAnimator() = default;

        void SetSpriteAtlas(std::shared_ptr<SpriteAtlas> atlas) noexcept;

        bool Play(const String& animationName);
        void Stop() noexcept;
        void Update(float deltaTime) noexcept;

        [[nodiscard]] bool IsPlaying() const noexcept { return isPlaying_; }
        [[nodiscard]] const SpriteFrame* GetCurrentFrame() const noexcept;
        [[nodiscard]] String GetCurrentAnimation() const noexcept { return currentAnimationName_; }

    private:
        [[nodiscard]] const SpriteAnimation* ResolveAnimation() const noexcept;

        std::weak_ptr<SpriteAtlas> atlas_{};
        String currentAnimationName_{};
        const SpriteAnimation* currentAnimation_{nullptr};
        float accumulatedTime_{0.0f};
        size_t currentFrameIndex_{0};
        bool isPlaying_{false};
    };
}
