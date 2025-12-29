#pragma once
#include <cstdint>
#include <limits>


union SDL_Event;

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

        
        void SetMouseEventRateLimit(int hz) noexcept;

    private:
        bool ShouldDropMouseMotionEvent_(const SDL_Event& event) noexcept;
        void UpdateMouseEventWindow_(std::uint64_t nowNs) noexcept;
        void NotifyMouseEventDropped_() noexcept;

        GuiModule* guiModule_{nullptr};
        SubsystemManager* subsystems_{nullptr};

        int mouseEventRateLimitHz_{200};
        int maxMouseEventsPerSecond_{500};
        
        std::uint64_t minMouseEventIntervalNs_{5'000'000};
        std::uint64_t lastMouseMotionTimestampNs_{0};
        std::uint64_t mouseEventsWindowStartNs_{0};
        
        int mouseEventsWindowCount_{0};
        int lastMouseX_{std::numeric_limits<int>::min()};
        int lastMouseY_{std::numeric_limits<int>::min()};
    };
}
