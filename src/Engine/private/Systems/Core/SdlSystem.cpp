#include "Systems/Core/SdlSystem.h"
#include <SDL3/SDL.h>
#include "Debug/Logger.h"
#include "Containers/String.h"

namespace BixEngine::Core
{
    bool SdlSystem::Initialize(const char* appName, const char* appId, const char* appVersion)
    {
        if (initialized_)
            return true;

        if (!SDL_SetAppMetadata(appName, appVersion, appId))
        {
            LOG_WARNING("Failed to set app metadata.");
        }
        
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        {
            LOG_ERROR(String{"Couldn't initialize SDL: "} + SDL_GetError());
            return false;
        }

        initialized_ = true;
        LOG_INFO("SDL System initialized.");
        return true;
    }

    void SdlSystem::Shutdown() noexcept
    {
        if (initialized_)
        {
            SDL_Quit();
            initialized_ = false;
            LOG_INFO("SDL System shutdown.");
        }
    }
}