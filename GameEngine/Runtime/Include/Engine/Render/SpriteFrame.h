#pragma once

#include <memory>
#include <functional>

#include "Core/Math/Rect.h"
#include "Core/Containers/String.h"
#include "Engine/Render/Texture.h"

namespace BixEngine::Render
{
    /**
     * @brief Shared data backing a sprite frame. Instances are pooled and reused across animations.
     */
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

    /**
     * @brief Lightweight handle that exposes a sprite frame backed by pooled data.
     */
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
