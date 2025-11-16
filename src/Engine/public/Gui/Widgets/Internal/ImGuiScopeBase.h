#pragma once

namespace BixEngine::Gui::Widgets::Internal
{
    /**
     * \brief Classe utilitaire commune pour les scopes RAII ImGui.
     */
    class ImGuiScopeBase
    {
    public:
        ImGuiScopeBase() = default;
        ImGuiScopeBase(const ImGuiScopeBase&) = delete;
        
        ImGuiScopeBase& operator=(const ImGuiScopeBase&) = delete;
        ImGuiScopeBase(ImGuiScopeBase&&) = delete;
        
        ImGuiScopeBase& operator=(ImGuiScopeBase&&) = delete;
        ~ImGuiScopeBase() = default;

    protected:
        /** Marque le scope comme actif. */
        void Activate() noexcept { engaged_ = true; }

        /** Indique si le scope doit appeler Pop dans son destructeur. */
        [[nodiscard]] bool IsActive() const noexcept { return engaged_; }

        /** Réinitialise l'état. */
        void Deactivate() noexcept { engaged_ = false; }

    private:
        bool engaged_{false};
    };
}
