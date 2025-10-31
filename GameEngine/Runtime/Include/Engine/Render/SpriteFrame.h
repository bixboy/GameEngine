#pragma once
#include <memory>
#include <functional>
#include "Core/Math/Rect.h"
#include "Engine/Render/Texture.h"


namespace BixEngine::Ressources
{

    struct SpriteFrameData
    {
        Texture* TexturePtr = nullptr;
        Math::Rect UVRect{};

        [[nodiscard]] bool operator==(const SpriteFrameData& other) const noexcept
        {
            return TexturePtr == other.TexturePtr &&
                   UVRect.X == other.UVRect.X &&
                   UVRect.Y == other.UVRect.Y &&
                   UVRect.Width == other.UVRect.Width &&
                   UVRect.Height == other.UVRect.Height;
        }
    };

    using SpriteFrameHandle = std::shared_ptr<const SpriteFrameData>;


    struct SpriteFrame
    {
        SpriteFrame() = default;
        explicit SpriteFrame(SpriteFrameHandle frameHandle) : handle(std::move(frameHandle)) {}

        [[nodiscard]] Texture* GetTexture() const noexcept { return handle ? handle->TexturePtr : nullptr; }
        [[nodiscard]] const Math::Rect& GetUVRect() const noexcept
        {
            static Math::Rect kEmpty{};
            return handle ? handle->UVRect : kEmpty;
        }

        SpriteFrameHandle handle;
    };
}
