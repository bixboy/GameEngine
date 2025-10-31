#include "Engine/Ressources/SpriteAtlas.h"

#include <filesystem>

#include "Core/Logger.h"
#include "Engine/Ressources/ResourceManager.h"
#include "Engine/Ressources/Texture.h"
#include "Engine/Render/SpriteAtlasUtils.h"

namespace BixEngine::Ressources
{
    namespace
    {
        [[nodiscard]] String ResolveTexturePath(const String& atlasPath, const std::string& texturePath)
        {
            std::filesystem::path atlasFile(atlasPath.c_str());
            std::filesystem::path resolved = atlasFile.parent_path() / texturePath;
            resolved = resolved.lexically_normal();
            return resolved.generic_string().c_str();
        }
    }

    bool SpriteAtlas::LoadFromFile(const String& path)
    {
        const std::string contents = SpriteAtlasUtils::LoadFileContents(path.Std());
        if (contents.empty())
        {
            LOG_ERROR("SpriteAtlas::LoadFromFile: unable to read asset: " + path);
            return false;
        }

        const SpriteAtlasDefinition definition = SpriteAtlasUtils::ParseDefinition(contents);
        if (definition.TexturePath.empty())
        {
            LOG_ERROR("SpriteAtlas::LoadFromFile: missing texture path in asset: " + path);
            return false;
        }

        texturePath_ = ResolveTexturePath(path, definition.TexturePath);

        auto& resourceManager = Core::ResourceManager::Get();
        texture_ = resourceManager.Get<Texture>(texturePath_);
        if (!texture_)
        {
            LOG_ERROR("SpriteAtlas::LoadFromFile: failed to load texture: " + texturePath_);
            return false;
        }

        frames_ = SpriteAtlasUtils::LoadFramesFromAtlas(*texture_, definition.Columns, definition.Rows, definition.Padding, definition.Margin);
        if (frames_.empty())
        {
            LOG_WARNING("SpriteAtlas::LoadFromFile: atlas has no frames: " + path);
        }

        animations_ = SpriteAtlasUtils::BuildAnimationsFromContent(contents, frames_);

        if (animations_.empty() && !frames_.empty())
        {
            SpriteAnimation fallback{};
            fallback.Name = "Default";
            fallback.FrameRate = 12.0f;
            fallback.Frames = frames_;
            animations_.push_back(std::move(fallback));
        }

        BuildAnimationLookup();

        return texture_ != nullptr;
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
            if (!animation.Name.IsEmpty())
            {
                animationLookup_[animation.Name] = index;
            }
        }
    }
}

