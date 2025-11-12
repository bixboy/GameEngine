#pragma once

#include <functional>

#include "Core/Containers/String.h"

namespace BixEngine::resources
{

    struct SpriteEvent
    {
        String Name;
        
        size_t FrameIndex = 0;
        
        float NormalizedTime = -1.0f;
        
        std::function<void()> Callback;
        
        bool bTriggerOnce = true;
        
        [[nodiscard]] bool IsValid() const noexcept { return Callback != nullptr || !Name.IsEmpty(); }
    };
}
