#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "Core/Containers/String.h"
#include "Engine/Ressources/IResource.h"
#include "Engine/Render/SpriteAnimator.h"

namespace BixEngine::Ressources
{
    class Texture;

    /**
     * @brief Represents a sprite atlas resource containing frames and animations.
     */
    class SpriteAtlas : public Core::IResource
    {
    public:
        SpriteAtlas() = default;
        ~SpriteAtlas() override = default;

        bool LoadFromFile(const String& path) override;

        [[nodiscard]] const std::vector<SpriteFrame>& GetFrames() const noexcept { return frames_; }
        [[nodiscard]] const std::vector<SpriteAnimation>& GetAnimations() const noexcept { return animations_; }
        [[nodiscard]] const SpriteAnimation* GetAnimation(const String& name) const noexcept;

        [[nodiscard]] std::shared_ptr<Texture> GetTexture() const noexcept { return texture_; }
        [[nodiscard]] const String& GetTexturePath() const noexcept { return texturePath_; }

    private:
        void BuildAnimationLookup();

        std::shared_ptr<Texture> texture_{};
        std::vector<SpriteFrame> frames_{};
        std::vector<SpriteAnimation> animations_{};
        std::unordered_map<String, size_t> animationLookup_{};
        String texturePath_{};
    };
}

