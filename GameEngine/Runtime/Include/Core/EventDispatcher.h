#pragma once

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

    private:
        GuiModule* guiModule_{ nullptr };
        SubsystemManager* subsystems_{ nullptr };
    };
}
