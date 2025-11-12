#pragma once

namespace BixEngine::Core
{
    class SdlSystem
    {
    public:
        // Initialise SDL3 avec les infos de l'application.
        bool Initialize(const char* appName, const char* appId, const char* appVersion);

        // Ferme proprement SDL et libère tous les sous-systèmes.
        void Shutdown() noexcept;

        // Retourne true si SDL est déjà initialisée.
        bool IsInitialized() const noexcept { return initialized_; }

    private:
        bool initialized_{false};
    };
}
