#pragma once

#include <functional>

#include "Core/Containers/String.h"

namespace BixEngine::Render
{
    /**
     * @brief Runtime callback triggered when a sprite animation reaches a specific frame.
     */
    struct SpriteEvent
    {
        /** Name used for debugging or editor tooling. */
        String Name;
        /** Index of the frame that should trigger this event. */
        size_t FrameIndex = 0;
        /** Optional normalized trigger time override (0..1). */
        float NormalizedTime = -1.0f;
        /** Callback invoked when the event fires. */
        std::function<void()> Callback;
        /** When true the event will only trigger once per play session. */
        bool bTriggerOnce = true;

        /**
         * @brief Returns whether the event is valid.
         */
        [[nodiscard]] bool IsValid() const noexcept { return Callback != nullptr || !Name.IsEmpty(); }
    };
}
