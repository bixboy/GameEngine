#pragma once

#include <SDL3/SDL_events.h>

namespace BixEngine::Core
{
    class GuiModule;
    class SubsystemManager;

    class EventDispatcher
    {
    public:
        void Configure(GuiModule* guiModule, SubsystemManager* subsystems) noexcept;
        void Reset() noexcept;
        void PumpEvents(bool& running);

    private:
        GuiModule* guiModule_{nullptr};
        SubsystemManager* subsystems_{nullptr};
    };
}
