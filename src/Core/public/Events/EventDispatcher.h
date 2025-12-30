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
        EventDispatcher(GuiModule* guiModule, SubsystemManager* subsystems) noexcept;

        void SetDependencies(GuiModule* guiModule, SubsystemManager* subsystems) noexcept;
        
        void PumpEvents(bool& running);

        void SetMouseUpdateRate(int hz) noexcept;

    private:
        [[nodiscard]] bool ShouldDropMouseMotionEvent_(const SDL_Event& event) noexcept;

        GuiModule* guiModule_{nullptr};
        SubsystemManager* subsystems_{nullptr};

        // --- Rate Limiting ---
        std::uint64_t minMouseEventIntervalNs_{5'000'000}; // Par défaut 5ms (200Hz)
        std::uint64_t lastMouseMotionTimestampNs_{0};
        
        float lastMouseX_{-FLT_MAX}; 
        float lastMouseY_{-FLT_MAX};
    };
}