#pragma once
#include <memory>
#include <unordered_map>
#include <vector>
#include "Containers/String.h"
#include "Render/Sprite/SpriteFrame.h"
#include "Ressources/Core/IResource.h"


namespace BixEngine::resources
{
    class Texture;

     
    struct SpriteAnimation
    {
        String name{};
        float frameRate{0.0f};
        bool loop{true};
        std::vector<size_t> frameIndices{};
    };

     
    class SpriteAtlas : public IResource
    {
    public:
        SpriteAtlas() = default;
        ~SpriteAtlas() override = default;

        bool LoadFromFile(const String& path) override;

        [[nodiscard]] const std::vector<SpriteFrame>& GetFrames() const noexcept { return frames_; }
        [[nodiscard]] const SpriteFrame* GetFrame(size_t index) const noexcept;
        [[nodiscard]] size_t GetFrameCount() const noexcept { return frames_.size(); }

        [[nodiscard]] const std::vector<SpriteAnimation>& GetAnimations() const noexcept { return animations_; }
        [[nodiscard]] const SpriteAnimation* GetAnimation(const String& name) const noexcept;

        [[nodiscard]] std::shared_ptr<Texture> GetTexture() const noexcept { return texture_; }
        [[nodiscard]] const String& GetTexturePath() const noexcept { return texturePath_; }

        [[nodiscard]] String GetPath() const { return path_; }

    private:
        void BuildAnimationLookup();

        std::shared_ptr<Texture> texture_{};
        std::vector<SpriteFrame> frames_{};
        std::vector<SpriteAnimation> animations_{};
        std::unordered_map<String, size_t> animationLookup_{};
        String texturePath_{};
        String path_{};
    };
}
