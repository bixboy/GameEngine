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

        SDL_SetAppMetadata(appName, appVersion, appId);

        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            LOG_ERROR(String{"Couldn't initialize SDL: "} + SDL_GetError());
            return false;
        }

        initialized_ = true;
        return true;
    }

    void SdlSystem::Shutdown() noexcept
    {
        if (initialized_)
        {
            SDL_Quit();
            initialized_ = false;
        }
    }
}
