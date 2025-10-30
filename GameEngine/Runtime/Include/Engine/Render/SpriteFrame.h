#pragma once

#include "Core/Math/Rect.h"
#include "Engine/Render/Texture.h"

namespace BixEngine::Render
{
    struct SpriteFrame
    {
        Texture* TexturePtr = nullptr;
        Math::Rect UVRect;
    };
}
