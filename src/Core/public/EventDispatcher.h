#pragma once
#include <cstdint>
#include <limits>


union SDL_Event;

namespace BixEngine::Core
{
    class GuiModule;
    class SubsystemManager;

    // Centralise la distribution des événements SDL
    // vers le GUI et les sous-systèmes du moteur.
    class EventDispatcher
    {
    public:
        // Lie le dispatcher au module GUI et au SubsystemManager.
        void Configure(GuiModule* guiModule, SubsystemManager* subsystems) noexcept;

        // Réinitialise les pointeurs internes.
        void Reset() noexcept;

        // Récupère et distribue tous les événements SDL.
        void PumpEvents(bool& running);

        // Définit la fréquence maximale de traitement des mouvements de souris.
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
