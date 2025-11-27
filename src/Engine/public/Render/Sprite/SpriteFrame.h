#pragma once
#include "Math/Rect.h"


namespace BixEngine::resources
{
    class Texture;

    /**
     * @brief Represents a single sprite frame referencing a texture region.
     */
    struct SpriteFrame
    {
        Texture* texture{nullptr};
        Math::Rect uvRect{};

        [[nodiscard]] Texture* GetTexture() const noexcept { return texture; }
        [[nodiscard]] const Math::Rect& GetUVRect() const noexcept { return uvRect; }
        [[nodiscard]] bool IsValid() const noexcept { return texture != nullptr; }
    };
}
