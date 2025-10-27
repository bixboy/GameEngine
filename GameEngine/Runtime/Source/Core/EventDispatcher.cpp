#include "Core/EventDispatcher.h"
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
            bool consumed = false;

            // GUI reçoit l'événement
            if (guiModule_)
            {
                consumed = guiModule_->ProcessEvent(event);
            }

            // Si non consommé
            if (!consumed && subsystems_)
            {
                subsystems_->ProcessEvent(event);
            }
            else if (subsystems_)
            {
                subsystems_->ResetInput();
            }

            // Gestion de la fermeture SDL
            if (event.type == SDL_EVENT_QUIT)
                running = false;
        }

        if (subsystems_ && subsystems_->ShouldQuit())
            running = false;
    }
}
