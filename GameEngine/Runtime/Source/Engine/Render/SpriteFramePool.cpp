#include "Engine/Render/SpriteFramePool.h"

#include <cmath>

namespace BixEngine::resources
{
    namespace
    {
        constexpr float kUvEpsilon = 1e-5f;

        [[nodiscard]] bool NearlyEqual(float lhs, float rhs) noexcept
        {
            return std::fabs(lhs - rhs) < kUvEpsilon;
        }
    }

    SpriteFramePool& SpriteFramePool::Get()
    {
        static SpriteFramePool gInstance;
        return gInstance;
    }

    SpriteFrameHandle SpriteFramePool::Acquire(Texture* texture, const Math::Rect& uvRect)
    {
        auto handle = std::make_shared<SpriteFrameData>();
        handle->TexturePtr = texture;
        handle->UVRect = uvRect;

        std::scoped_lock guard(mutex_);
        auto [it, inserted] = frames_.insert(handle);
        if (!inserted)
        {
            return *it;
        }

        return handle;
    }

    size_t SpriteFramePool::FrameHasher::operator()(const SpriteFrameHandle& handle) const noexcept
    {
        if (!handle)
            return 0;

        const auto* data = handle.get();
        size_t seed = reinterpret_cast<size_t>(data->TexturePtr);
        seed ^= std::hash<float>{}(data->UVRect.X) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<float>{}(data->UVRect.Y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<float>{}(data->UVRect.Width) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<float>{}(data->UVRect.Height) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }

    bool SpriteFramePool::FrameEqual::operator()(const SpriteFrameHandle& lhs, const SpriteFrameHandle& rhs) const noexcept
    {
        if (lhs == rhs)
            return true;
        if (!lhs || !rhs)
            return false;

        return lhs->TexturePtr == rhs->TexturePtr &&
               NearlyEqual(lhs->UVRect.X, rhs->UVRect.X) &&
               NearlyEqual(lhs->UVRect.Y, rhs->UVRect.Y) &&
               NearlyEqual(lhs->UVRect.Width, rhs->UVRect.Width) &&
               NearlyEqual(lhs->UVRect.Height, rhs->UVRect.Height);
    }
}
