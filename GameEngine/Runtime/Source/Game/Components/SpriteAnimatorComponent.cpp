#include "Game/Components/SpriteAnimatorComponent.h"
#include "Core/Logger.h"
#include "Engine/Render/SpriteAtlasUtils.h"
#include "Engine/Render/SpriteFramePool.h"
#include "Graphics/Renderer.h"
#include "Game/Actor.h"
#include <algorithm>
#include <string>

namespace BixEngine::Game
{
    SpriteAnimatorComponent::SpriteAnimatorComponent(Actor* owner): SpriteComponent(owner)
    {
    }

    namespace
    {
        [[nodiscard]] String BuildAtlasId(const String& texturePath, int columns, int rows)
        {
            String atlasId = texturePath;
            atlasId += "_";
            atlasId += std::to_string(columns);
            atlasId += "x";
            atlasId += std::to_string(rows);
            return atlasId;
        }
    }

    void SpriteAnimatorComponent::BeginPlay()
    {
        SpriteComponent::BeginPlay();

        if (!frames_.empty())
        {
            const auto& frame = frames_[currentFrame_];
            SetTexture(frame.GetTexture());
            SetUVRect(frame.GetUVRect());
        }
    }

    void SpriteAnimatorComponent::Update(float deltaTime)
    {
        Super::Update(deltaTime);
        
        if (!bPlaying_ || frames_.empty())
            return;

        if (frameRate_ <= 0.0f)
        {
            LOG_WARNING("⚠️ Invalid frame rate supplied to SpriteAnimatorComponent. Animation halted.");
            bPlaying_ = false;
            return;
        }

        timer_ += std::max(0.0f, deltaTime);
        const float frameDuration = 1.0f / frameRate_;

        while (timer_ >= frameDuration && bPlaying_)
        {
            timer_ -= frameDuration;
            currentFrame_++;

            if (currentFrame_ >= totalFrames_)
            {
                if (bLoop_)
                {
                    currentFrame_ = 0;
                }
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
        if (columns <= 0 || rows <= 0)
        {
            LOG_ERROR("❌ Invalid spritesheet layout. Columns and rows must be greater than zero.");
            frames_.clear();
            totalFrames_ = 0;
            bPlaying_ = false;
            loadedTexture_.reset();
            return;
        }

        auto* renderer = Graphics::Renderer::Get();
        if (!renderer)
        {
            LOG_ERROR("❌ Renderer is not initialized — cannot load spritesheet: " + texturePath);
            frames_.clear();
            totalFrames_ = 0;
            bPlaying_ = false;
            loadedTexture_.reset();
            return;
        }

        loadedTexture_ = Ressources::TextureManager::Get().LoadTexture(texturePath, renderer->GetSDLRenderer());
        if (!loadedTexture_)
        {
            LOG_ERROR("❌ Failed to load spritesheet: " + texturePath);
            frames_.clear();
            totalFrames_ = 0;
            bPlaying_ = false;
            loadedTexture_.reset();
            return;
        }

        const String atlasId = BuildAtlasId(texturePath, columns, rows);
        std::vector<Ressources::SpriteFrame> cached = Ressources::TextureManager::Get().GetCachedAtlas(atlasId);

        if (!cached.empty())
        {
            frames_.clear();
            frames_.reserve(cached.size());
            Ressources::SpriteFramePool& pool = Ressources::SpriteFramePool::Get();
            for (const auto& frame : cached)
            {
                frames_.emplace_back(pool.Acquire(loadedTexture_.get(), frame.GetUVRect()));
            }
            if (!frames_.empty())
            {
                Ressources::TextureManager::Get().CacheAtlas(atlasId, frames_);
            }
        }
        else
        {
            frames_ = Ressources::SpriteAtlasUtils::LoadFramesFromAtlas(*loadedTexture_, columns, rows);
            if (!frames_.empty())
            {
                Ressources::TextureManager::Get().CacheAtlas(atlasId, frames_);
            }
        }

        totalFrames_ = static_cast<int>(frames_.size());
        if (frameRate <= 0.0f)
        {
            LOG_WARNING("⚠️ Requested frame rate is non-positive. Defaulting to 1 fps.");
            frameRate_ = 1.0f;
        }
        else
        {
            frameRate_ = frameRate;
        }
        bLoop_ = loop;
        currentFrame_ = 0;
        timer_ = 0.0f;

        if (totalFrames_ > 0)
        {
            const auto& frame = frames_[currentFrame_];
            SetTexture(frame.GetTexture());
            SetUVRect(frame.GetUVRect());
        }
        else
        {
            LOG_WARNING("⚠️ No frames generated from spritesheet: " + texturePath);
        }

        LOG_INFO("✅ Loaded " + String(std::to_string(totalFrames_)) + " frames from " + texturePath);
    }

    void SpriteAnimatorComponent::Play()
    {
        if (frames_.empty())
        {
            LOG_WARNING("⚠️ No frames loaded — cannot play animation.");
            return;
        }

        bPlaying_ = true;
        currentFrame_ = std::clamp(currentFrame_, 0, std::max(0, totalFrames_ - 1));
        timer_ = 0.0f;

        const auto& frame = frames_[currentFrame_];
        SetTexture(frame.GetTexture());
        SetUVRect(frame.GetUVRect());
    }

    void SpriteAnimatorComponent::Stop()
    {
        bPlaying_ = false;
        currentFrame_ = 0;
        timer_ = 0.0f;

        if (!frames_.empty())
        {
            const auto& frame = frames_[currentFrame_];
            SetTexture(frame.GetTexture());
            SetUVRect(frame.GetUVRect());
        }
    }
}
