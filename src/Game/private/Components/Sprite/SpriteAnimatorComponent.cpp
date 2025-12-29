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

                std::string extension = ToLower(it->path().extension().generic_string());
                if (extension == ".atlas")
                    outAtlases.push_back(it->path());
            }

            std::sort(outAtlases.begin(), outAtlases.end(),
              [](const std::filesystem::path& a, const std::filesystem::path& b)
              {
                  return a.generic_string() < b.generic_string();
              });
        }

        std::string MakeDisplayName(const std::filesystem::path& path, const std::filesystem::path& root)
        {
            if (root.empty())
                return path.generic_string();

            std::error_code ec;
            const std::filesystem::path relative = std::filesystem::relative(path, root, ec);
            if (!ec && !relative.empty())
                return relative.generic_string();

            return path.generic_string();
        }
    }


    
    SpriteAnimatorComponent::SpriteAnimatorComponent(Actor* owner) : SpriteComponent(owner)
    {
    }

    void SpriteAnimatorComponent::BeginPlay()
    {
        SpriteComponent::BeginPlay();

        LOG_INFO("ANIMATOR_DEBUG: BeginPlay. AtlasPtr Valid: " + String(atlas_ ? "YES" : "NO") + ", AtlasPath: '" + atlasPath_ + "'");

        if (!atlas_ && !atlasPath_.IsEmpty())
            LoadSpriteAtlas(atlasPath_, defaultAnimation_);

        if (atlas_)
        {
            animator_.SetSpriteAtlas(atlas_);
            if (!defaultAnimation_.IsEmpty() && atlas_->GetAnimation(defaultAnimation_))
            {
                currentAnimation_ = defaultAnimation_;
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
        auto& resourceManager = resources::ResourceManager::Get();
        auto atlas = resourceManager.Get<resources::SpriteAtlas>(atlasPath);
        if (!atlas)
        {
            LOG_ERROR("❌ Failed to load sprite atlas: " + atlasPath);

            atlas_.reset();
            animator_.SetSpriteAtlas(nullptr);
            atlasPath_.Clear();
            defaultAnimation_.Clear();
            currentAnimation_.Clear();
            SetTexture(nullptr);

            return false;
        }

        atlas_ = std::move(atlas);
        animator_.SetSpriteAtlas(atlas_);
        atlasPath_ = atlasPath;

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
            currentAnimation_.Clear();
            defaultAnimation_.Clear();
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
        if (currentAnimation_.IsEmpty())
        {
            LOG_WARNING("⚠️ No animation selected — cannot play.");
            return;
        }

        if (!atlas_)
        {
            LOG_WARNING("⚠️ Cannot play animation '" + currentAnimation_ + "': No SpriteAtlas assigned.");
            return;
        }

        if (!atlas_->GetAnimation(currentAnimation_))
        {
            LOG_WARNING("⚠️ Animation '" + currentAnimation_ + "' not found in atlas: " + atlas_->GetPath());
            return;
        }

        if (!animator_.Play(currentAnimation_))
        {
            LOG_WARNING("⚠️ Failed to start animation: " + currentAnimation_);
            return;
        }

        ApplyCurrentFrame(false);
    }

    void SpriteAnimatorComponent::Play(const String& animationName)
    {
        if (animationName.IsEmpty())
        {
            LOG_WARNING("⚠️ Cannot play animation with empty name.");
            return;
        }

        if (!atlas_)
        {
            LOG_WARNING("⚠️ Cannot play animation '" + animationName + "': No SpriteAtlas assigned.");
            return;
        }

        if (!atlas_->GetAnimation(animationName))
        {
            LOG_WARNING("⚠️ Animation '" + animationName + "' not found in atlas: " + atlas_->GetPath());
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
        const resources::SpriteFrame* frame = animator_.GetCurrentFrame();

        if (!frame && allowFallbackToDefault && atlas_ && !currentAnimation_.IsEmpty())
        {
            const resources::SpriteAnimation* animation = atlas_->GetAnimation(currentAnimation_);
            if (animation && !animation->frameIndices.empty())
            {
                frame = atlas_->GetFrame(animation->frameIndices.front());
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

        
        
        if (atlas_ && atlasPath_ != atlas_->GetPath())
        {
            String oldPath = atlasPath_;
            atlasPath_ = atlas_->GetPath();
            LOG_INFO("ANIMATOR_DEBUG: Inspector Synced AtlasPath. Old: '" + oldPath + "' New: '" + atlasPath_ + "'");
        }
    }
}
