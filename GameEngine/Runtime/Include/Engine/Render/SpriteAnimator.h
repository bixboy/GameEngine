#pragma once

#include <vector>
#include "Core/Containers/String.h"
#include "Engine/Render/SpriteFrame.h"

namespace BixEngine::Render
{
    struct SpriteAnimation
    {
        String Name;
        std::vector<SpriteFrame> Frames;
        bool bLoop = true;
        float FrameRate = 12.0f;
    };

    class SpriteAnimator
    {
    public:
        SpriteAnimator() = default;

        // Add Animation to the list
        void AddAnimation(const SpriteAnimation& animation);

        // Play Sepcifique Animation
        void Play(const String& name);

        // Pause Current Animation
        void Pause();

        // Stop Current Animation
        void Stop();


        // Animation Updater
        void Update(float deltaTime);

        [[nodiscard]] const SpriteFrame* GetCurrentFrame() const noexcept;
        [[nodiscard]] bool IsPlaying() const noexcept { return bPlaying_; }

        void SetSpeed(float multiplier) noexcept { speedMultiplier_ = multiplier; }

    private:
        std::vector<SpriteAnimation> animations_;
        
        const SpriteAnimation* currentAnim_ = nullptr;
        
        float timeAccumulator_ = 0.0f;
        
        size_t currentFrameIndex_ = 0;
        
        bool bPlaying_ = false;
        
        float speedMultiplier_ = 1.0f;
    };
}
