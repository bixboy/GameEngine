#include "Ressources/RessourcesClass/SpriteAtlas.h"
#include <filesystem>
#include "Debug/Logger.h"
#include "Ressources/Core/ResourceManager.h"
#include "Ressources/RessourcesClass/Texture.h"

// Tu dois avoir ce fichier quelque part pour parser ton format d'atlas
#include "Ressources/Atlas/SpriteAtlasUtils.h" 

namespace BixEngine::Resources
{
    namespace
    {
        [[nodiscard]] String ResolveTexturePath(const String& atlasPath, const String& relativeTexture)
        {
            std::filesystem::path atlasFile(atlasPath.c_str());
            std::filesystem::path resolved = atlasFile.parent_path() / relativeTexture.c_str();
            return String(resolved.lexically_normal().generic_string().c_str());
        }
    }

    bool SpriteAtlas::LoadFromFile(const String& path)
    {
        SpriteAtlasDefinition definition;
        std::vector<SpriteAnimationDefinition> animationDefinitions;

        if (!SpriteAtlasUtils::ParseAtlasFile(path, definition, animationDefinitions))
        {
            LOG_ERROR("SpriteAtlas::LoadFromFile: failed to parse atlas " + path);
            return false;
        }

        path_ = path;
        texturePath_ = ResolveTexturePath(path, definition.texturePath);

        auto& resourceManager = ResourceManager::Get();
        texture_ = resourceManager.Get<Texture>(texturePath_);
        
        if (!texture_)
        {
            LOG_ERROR("SpriteAtlas::LoadFromFile: unable to load texture " + texturePath_);
            return false;
        }

        frames_ = SpriteAtlasUtils::GenerateFrames(*texture_, definition.columns, definition.rows, definition.padding, definition.margin);
        
        if (frames_.empty())
        {
            LOG_WARNING("SpriteAtlas::LoadFromFile: atlas has no frames: " + path);
        }

        animations_.clear();
        animations_.reserve(animationDefinitions.size());

        const size_t frameCount = frames_.size();
        
        for (const auto& animDef : animationDefinitions)
        {
            if (animDef.name.empty())
                continue;

            SpriteAnimation animation;
            animation.name = animDef.name;
            animation.frameRate = animDef.frameRate;
            animation.loop = animDef.loop;

            for (size_t frameIndex : animDef.frames)
            {
                if (frameIndex < frameCount)
                {
                    animation.frameIndices.push_back(frameIndex);
                }
                else
                    LOG_WARNING("SpriteAtlas::LoadFromFile: frame index out of range in animation '" + animDef.name + "'.");
            }

            if (!animation.frameIndices.empty())
                animations_.push_back(std::move(animation));
        }

        if (animations_.empty() && frameCount > 0)
        {
            SpriteAnimation defaultAnimation;
            defaultAnimation.name = "Default";
            defaultAnimation.frameRate = 12.0f;
            defaultAnimation.loop = true;
            defaultAnimation.frameIndices.resize(frameCount);

            for (size_t i = 0; i < frameCount; ++i)
                defaultAnimation.frameIndices[i] = i;

            animations_.push_back(std::move(defaultAnimation));
        }

        BuildAnimationLookup();
        return texture_ != nullptr;
    }

    const SpriteFrame* SpriteAtlas::GetFrame(size_t index) const noexcept
    {
        if (index >= frames_.size())
            return nullptr;
        
        return &frames_[index];
    }

    const SpriteAnimation* SpriteAtlas::GetAnimation(const String& name) const noexcept
    {
        const auto found = animationLookup_.find(name);
        if (found == animationLookup_.end())
            return nullptr;

        const size_t index = found->second;
        if (index >= animations_.size())
            return nullptr;

        return &animations_[index];
    }

    void SpriteAtlas::BuildAnimationLookup()
    {
        animationLookup_.clear();
        for (size_t index = 0; index < animations_.size(); ++index)
        {
            const auto& animation = animations_[index];
            if (!animation.name.empty())
            {
                animationLookup_[animation.name] = index;
            }
        }
    }
}