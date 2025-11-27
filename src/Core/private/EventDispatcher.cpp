#include "Events/EventDispatcher.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_timer.h>
#include <limits>
#include "Gui/Internal/GuiModule.h"
#include "Systems/Core/SubsystemManager.h"


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
        lastMouseMotionTimestampNs_ = 0;
        mouseEventsWindowStartNs_ = 0;
        mouseEventsWindowCount_ = 0;
        lastMouseX_ = std::numeric_limits<int>::min();
        lastMouseY_ = std::numeric_limits<int>::min();
    }

    void EventDispatcher::SetMouseEventRateLimit(int hz) noexcept
    {
        if (hz <= 0)
        {
            mouseEventRateLimitHz_ = 0;
            minMouseEventIntervalNs_ = 0;
            return;
        }

        mouseEventRateLimitHz_ = hz;
        const std::uint64_t interval = 1'000'000'000ull / static_cast<std::uint64_t>(mouseEventRateLimitHz_);
        minMouseEventIntervalNs_ = interval > 0 ? interval : 1;
    }

    void EventDispatcher::PumpEvents(bool& running)
    {
        SDL_Event event{};
        while (SDL_PollEvent(&event))
        {
            if (ShouldDropMouseMotionEvent_(event))
                continue;

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

    bool EventDispatcher::ShouldDropMouseMotionEvent_(const SDL_Event& event) noexcept
    {
        if (event.type != SDL_EVENT_MOUSE_MOTION)
            return false;

        const SDL_MouseMotionEvent& motion = event.motion;
        const std::uint64_t nowNs = SDL_GetTicksNS();

        UpdateMouseEventWindow_(nowNs);

        if (maxMouseEventsPerSecond_ > 0 && mouseEventsWindowCount_ >= maxMouseEventsPerSecond_)
        {
            NotifyMouseEventDropped_();
            return true;
        }

        if (minMouseEventIntervalNs_ > 0 && lastMouseMotionTimestampNs_ != 0 &&
            (nowNs - lastMouseMotionTimestampNs_) < minMouseEventIntervalNs_)
        {
            NotifyMouseEventDropped_();
            return true;
        }

        if (motion.x == lastMouseX_ && motion.y == lastMouseY_ && motion.xrel == 0 && motion.yrel == 0)
        {
            NotifyMouseEventDropped_();
            return true;
        }

        lastMouseMotionTimestampNs_ = nowNs;
        lastMouseX_ = motion.x;
        lastMouseY_ = motion.y;

        ++mouseEventsWindowCount_;
        return false;
    }

    void EventDispatcher::UpdateMouseEventWindow_(std::uint64_t nowNs) noexcept
    {
        if (mouseEventsWindowStartNs_ == 0)
        {
            mouseEventsWindowStartNs_ = nowNs;
            mouseEventsWindowCount_ = 0;
            return;
        }

        const std::uint64_t elapsed = nowNs - mouseEventsWindowStartNs_;
        if (elapsed >= 1'000'000'000ull)
        {
            mouseEventsWindowStartNs_ = nowNs;
            mouseEventsWindowCount_ = 0;
        }
    }

    void EventDispatcher::NotifyMouseEventDropped_() noexcept
    {
        if (subsystems_)
            subsystems_->NotifyMouseEventDropped();
    }
}
