#pragma once
#include "Math/Rect.h"

namespace BixEngine::Resources
{
    class Texture;

    struct SpriteFrame
    {
        Texture* texture{nullptr};
        Math::Rect uvRect{};

        [[nodiscard]] Texture* GetTexture() const noexcept { return texture; }
        [[nodiscard]] const Math::Rect& GetUVRect() const noexcept { return uvRect; }
        [[nodiscard]] bool IsValid() const noexcept { return texture != nullptr; }
    };
}