#pragma once

namespace BixEngine::Core
{
    class SdlSystem
    {
    public:
        SdlSystem() = default;
        ~SdlSystem() { Shutdown(); }

        SdlSystem(const SdlSystem&) = delete;
        SdlSystem& operator=(const SdlSystem&) = delete;

        bool Initialize(const char* appName, const char* appId, const char* appVersion);
        void Shutdown() noexcept;
        
        [[nodiscard]] bool IsInitialized() const noexcept { return initialized_; }

    private:
        bool initialized_{false};
    };
}