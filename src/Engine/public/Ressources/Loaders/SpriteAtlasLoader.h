#pragma once

#include <memory>
#include "Containers/String.h"

namespace BixEngine::resources
{
    class SpriteAtlas;

    std::shared_ptr<SpriteAtlas> LoadSpriteAtlas(const String& path);
}
