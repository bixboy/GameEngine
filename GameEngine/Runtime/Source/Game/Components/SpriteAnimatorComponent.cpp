#include "Game/Components/SpriteAnimatorComponent.h"
#include "Graphics/Renderer.h"
#include "Core/Logger.h"
#include "Engine/Render/SpriteAtlasUtils.h"
#include "Game/Actor.h"

namespace BixEngine::Game
{
    SpriteAnimatorComponent::SpriteAnimatorComponent(Actor* owner): SpriteComponent(owner)
    {
    }

    void SpriteAnimatorComponent::BeginPlay()
    {
        SpriteComponent::BeginPlay();
    }

    void SpriteAnimatorComponent::Update(float deltaTime)
    {
        if (!bPlaying_ || frames_.empty())
            return;

        timer_ += deltaTime;
        const float frameDuration = 1.f / frameRate_;

        if (timer_ >= frameDuration)
        {
            timer_ -= frameDuration;
            currentFrame_++;

            if (currentFrame_ >= totalFrames_)
            {
                if (bLoop_)
                    currentFrame_ = 0;
                else
                {
                    currentFrame_ = totalFrames_ - 1;
                    bPlaying_ = false;
                }
            }

            const auto& frame = frames_[currentFrame_];
            SetTexture(frame.GetTexture());
            SetUVRect(frame.GetUVRect());
        }
    }

    void SpriteAnimatorComponent::LoadSpriteSheet(const String& texturePath, int columns, int rows, float frameRate, bool loop)
    {
        texture_ = Render::TextureManager::Get().LoadTexture(
            texturePath, Graphics::Renderer::Get()->GetSDLRenderer());

        if (!texture_)
        {
            LOG_ERROR("❌ Failed to load spritesheet: " + texturePath);
            return;
        }

        frames_ = Render::SpriteAtlasUtils::LoadFramesFromAtlas(*texture_, columns, rows, 0, 0);
        totalFrames_ = static_cast<int>(frames_.size());
        frameRate_ = frameRate;
        bLoop_ = loop;

        if (totalFrames_ > 0)
        {
            SetTexture(frames_[0].GetTexture());
            SetUVRect(frames_[0].GetUVRect());
        }

        LOG_INFO("Loaded " + String(std::to_string(totalFrames_)) + " frames from " + texturePath);
    }

    void SpriteAnimatorComponent::Play()
    {
        if (frames_.empty())
        {
            LOG_WARNING("⚠️ No frames loaded — cannot play animation.");
            return;
        }

        bPlaying_ = true;
        currentFrame_ = 0;
        timer_ = 0.f;

        SetTexture(frames_[0].GetTexture());
        SetUVRect(frames_[0].GetUVRect());
    }

    void SpriteAnimatorComponent::Stop()
    {
        bPlaying_ = false;
        currentFrame_ = 0;
    }
}
