#pragma once

#include <string_view>

namespace BixEngine::Core
{
    class SdlSystem
    {
    public:
        bool Initialize(const char* appName, const char* appId, const char* appVersion);
        void Shutdown() noexcept;
        bool IsInitialized() const noexcept { return initialized_; }

    private:
        bool initialized_{false};
    };
}
