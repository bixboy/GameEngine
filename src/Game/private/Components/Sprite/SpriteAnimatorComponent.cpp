#include "Components/Sprite/SpriteAnimatorComponent.h"
#include "Debug/Logger.h"
#include "Ressources/Core/ResourceManager.h"
#include "Ressources/RessourcesClass/SpriteAtlas.h"
#include "Framework/Actor.h"
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>
#include <imgui.h>


namespace BixEngine::Game
{
    namespace
    {
        std::string ToLower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });
            
            return value;
        }

        std::filesystem::path DetermineAtlasRoot()
        {
            std::error_code ec;
            const std::filesystem::path base = std::filesystem::current_path(ec);
            if (ec)
                return {};

            const std::filesystem::path content = base / "Content";
            if (std::filesystem::exists(content))
                return content;

            const std::filesystem::path resources = base / "Resources";
            if (std::filesystem::exists(resources))
                return resources;

            return {};
        }

        void CollectAtlasFiles(std::vector<std::filesystem::path>& outAtlases)
        {
            outAtlases.clear();
            const std::filesystem::path root = DetermineAtlasRoot();
            
            if (root.empty())
                return;

            std::error_code ec;
            for (std::filesystem::recursive_directory_iterator it(root, ec), end; it != end; it.increment(ec))
            {
                if (ec)
                    break;
                
                if (!it->is_regular_file())
                    continue;

                if (ToLower(it->path().extension().string()) == ".atlas")
                {
                    outAtlases.push_back(std::filesystem::relative(it->path(), root, ec)); 
                }
            }

            std::sort(outAtlases.begin(), outAtlases.end());
        }
    }

    SpriteAnimatorComponent::SpriteAnimatorComponent(Actor* owner) : SpriteComponent(owner)
    {
    }

    void SpriteAnimatorComponent::BeginPlay()
    {
        SpriteComponent::BeginPlay();

        if (atlas_)
        {
            animator_.SetSpriteAtlas(atlas_);
            bool hasAnim = false;
            
            if (!defaultAnimation_.IsEmpty() && atlas_->GetAnimation(defaultAnimation_))
            {
                currentAnimation_ = defaultAnimation_;
                hasAnim = true;
            }
            else if (!atlas_->GetAnimations().empty())
            {
                // Auto-pick first if default invalid or empty
                currentAnimation_ = atlas_->GetAnimations().front().name;
                hasAnim = true;
            }
            
            if (hasAnim)
            {
                animator_.Play(currentAnimation_);
            }
        }

        ApplyCurrentFrame(true);
    }

    void SpriteAnimatorComponent::Update(float deltaTime)
    {
        Super::Update(deltaTime);

        if (!atlas_)
            return;

        animator_.Update(std::max(0.0f, deltaTime));
        ApplyCurrentFrame(false);
    }

    bool SpriteAnimatorComponent::LoadSpriteAtlas(const String& atlasPath, const String& defaultAnimation)
    {
        auto& resourceManager = Resources::ResourceManager::Get();
        
        auto atlas = resourceManager.Get<Resources::SpriteAtlas>(atlasPath);
        if (!atlas)
        {
            LOG_ERROR("Failed to load sprite atlas: " + atlasPath);
            
            atlas_.reset();
            animator_.SetSpriteAtlas(nullptr);
            SetTexture(nullptr);
            
            return false;
        }

        atlas_ = std::move(atlas);
        animator_.SetSpriteAtlas(atlas_);

        if (!defaultAnimation.IsEmpty() && atlas_->GetAnimation(defaultAnimation))
        {
            currentAnimation_ = defaultAnimation;
            defaultAnimation_ = defaultAnimation;
        }
        else if (!defaultAnimation_.IsEmpty() && atlas_->GetAnimation(defaultAnimation_))
        {
            currentAnimation_ = defaultAnimation_;
        }
        else if (!atlas_->GetAnimations().empty())
        {
            currentAnimation_ = atlas_->GetAnimations().front().name;
            defaultAnimation_ = currentAnimation_;
        }
        else
        {
            currentAnimation_.clear();
            defaultAnimation_.clear();
        }

        if (!currentAnimation_.IsEmpty())
        {
            animator_.Play(currentAnimation_);
        }
        else
        {
            animator_.Stop();
        }

        ApplyCurrentFrame(true);
        return true;
    }

    void SpriteAnimatorComponent::Play()
    {
        if (currentAnimation_.IsEmpty() || !atlas_)
            return;

        if (animator_.Play(currentAnimation_))
        {
            ApplyCurrentFrame(false);
        }
    }

    void SpriteAnimatorComponent::Play(const String& animationName)
    {
        if (animationName.IsEmpty() || !atlas_)
            return;

        if (!atlas_->GetAnimation(animationName))
        {
            LOG_WARNING("Animation '" + animationName + "' not found in atlas.");
            return;
        }

        currentAnimation_ = animationName;
        Play();
    }

    void SpriteAnimatorComponent::Stop()
    {
        animator_.Stop();
        ApplyCurrentFrame(true);
    }

    void SpriteAnimatorComponent::ApplyCurrentFrame(bool allowFallbackToDefault)
    {
        const Resources::SpriteFrame* frame = animator_.GetCurrentFrame();

        if (!frame && allowFallbackToDefault && atlas_ && !currentAnimation_.IsEmpty())
        {
            if (const auto* anim = atlas_->GetAnimation(currentAnimation_))
            {
                if (!anim->frameIndices.empty())
                {
                    frame = atlas_->GetFrame(anim->frameIndices.front());
                }
            }
        }

        if (frame && frame->IsValid())
        {
            SetTexture(frame->GetTexture());
            SetUVRect(frame->GetUVRect());
        }
        else if (atlas_)
        {
            SetTexture(nullptr);
        }
    }

    void SpriteAnimatorComponent::DrawInspectorUI()
    {
        SpriteComponent::DrawInspectorUI();
    }
}