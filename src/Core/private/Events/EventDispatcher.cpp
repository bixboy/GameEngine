#include "Events/EventDispatcher.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_timer.h>
#include "Gui/Internal/GuiModule.h"
#include "Systems/Core/SubsystemManager.h"


namespace BixEngine::Core
{
    EventDispatcher::EventDispatcher(GuiModule* guiModule, SubsystemManager* subsystems) noexcept : guiModule_(guiModule), subsystems_(subsystems)
    {
    }

    void EventDispatcher::SetDependencies(GuiModule* guiModule, SubsystemManager* subsystems) noexcept
    {
        guiModule_ = guiModule;
        subsystems_ = subsystems;
    }

    void EventDispatcher::SetMouseUpdateRate(int hz) noexcept
    {
        if (hz <= 0)
        {
            minMouseEventIntervalNs_ = 0;
        }
        else
        {
            minMouseEventIntervalNs_ = 1'000'000'000ull / static_cast<std::uint64_t>(hz);
        }
    }

    void EventDispatcher::PumpEvents(bool& running)
    {
        SDL_Event event{};
    
        while (SDL_PollEvent(&event))
        {
            // --- 1. Événements Critiques Système ---
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
                return;
            }

            if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
            {
                if (subsystems_) 
                    subsystems_->OnWindowResize(event.window.data1, event.window.data2);
                
                continue;
            }

            if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST || event.type == SDL_EVENT_WINDOW_MINIMIZED)
            {
                if (subsystems_) 
                {
                    subsystems_->ResetInput();
                }
            }

            // Drag & Drop de fichiers (Pour l'éditeur)
            if (event.type == SDL_EVENT_DROP_FILE)
            {
                if (guiModule_)
                {
                    guiModule_->ProcessEvent(event);   
                }
                else if (subsystems_)
                {
                    subsystems_->OnFileDrop(event.drop.data);   
                }
                
                continue;
            }

            // --- 2. Filtrage Souris (Rate Limiting) ---
            if (ShouldDropMouseMotionEvent_(event))
                continue;

            // --- 3. Dispatch UI ---
            bool consumedByGui = false;
            if (guiModule_)
            {
                consumedByGui = guiModule_->ProcessEvent(event);
            }

            // --- 4. Dispatch Gameplay ---
            if (!consumedByGui && subsystems_)
            {
                subsystems_->ProcessEvent(event);
            }
        }

        if (subsystems_ && subsystems_->ShouldQuit())
        {
            running = false;
        }
    }

    bool EventDispatcher::ShouldDropMouseMotionEvent_(const SDL_Event& event) noexcept
    {
        if (event.type != SDL_EVENT_MOUSE_MOTION)
            return false;

        if (minMouseEventIntervalNs_ == 0)
            return false;

        const std::uint64_t nowNs = SDL_GetTicksNS();
        if (lastMouseMotionTimestampNs_ != 0 && (nowNs - lastMouseMotionTimestampNs_) < minMouseEventIntervalNs_)
        {
            return true;
        }
        
        constexpr float epsilon = 1e-5f;
        if (std::abs(event.motion.x - lastMouseX_) < epsilon && std::abs(event.motion.y - lastMouseY_) < epsilon)
        {
            return true;
        }

        lastMouseMotionTimestampNs_ = nowNs;
        lastMouseX_ = event.motion.x;
        lastMouseY_ = event.motion.y;

        return false;
    }
}