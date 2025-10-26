#include "Core/EventDispatcher.h"

#include <SDL3/SDL.h>

#include "Engine/Gui/GuiModule.h"
#include "Engine/Systems/SubsystemManager.h"

namespace BixEngine::Core
{
    void EventDispatcher::Configure(GuiModule* guiModule, SubsystemManager* subsystems) noexcept
    {
        guiModule_ = guiModule;
        subsystems_ = subsystems;
    }

    void EventDispatcher::Reset() noexcept
    {
        guiModule_ = nullptr;
        subsystems_ = nullptr;
    }

    void EventDispatcher::PumpEvents(bool& running)
    {
        SDL_Event event{};
        while (SDL_PollEvent(&event))
        {
            if (guiModule_)
                guiModule_->ProcessEvent(event);

            if (subsystems_)
                subsystems_->ProcessEvent(event);

            if (event.type == SDL_EVENT_QUIT)
                running = false;
        }

        if (subsystems_ && subsystems_->ShouldQuit())
            running = false;
    }
}
