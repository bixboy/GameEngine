#pragma once

#include <mutex>
#include <unordered_set>

#include "Engine/Render/SpriteFrame.h"

namespace BixEngine::Render
{
    /**
     * @brief Thread-safe pool that deduplicates sprite frame data across animators.
     */
    class SpriteFramePool
    {
    public:
        /**
         * @brief Returns the global frame pool instance.
         */
        static SpriteFramePool& Get();

        /**
         * @brief Retrieves or creates a shared frame handle for the given texture/UV pair.
         */
        [[nodiscard]] SpriteFrameHandle Acquire(Texture* texture, const Math::Rect& uvRect);

    private:
        SpriteFramePool() = default;

        struct FrameHasher
        {
            size_t operator()(const SpriteFrameHandle& handle) const noexcept;
        };

        struct FrameEqual
        {
            bool operator()(const SpriteFrameHandle& lhs, const SpriteFrameHandle& rhs) const noexcept;
        };

        std::mutex mutex_;
        std::unordered_set<SpriteFrameHandle, FrameHasher, FrameEqual> frames_;
    };
}
