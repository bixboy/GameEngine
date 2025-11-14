#pragma once

namespace BixEngine::Gui::Widgets::Internal
{
    /**
     * \brief Classe utilitaire commune pour les scopes RAII ImGui.
     *
     * Gère l'état d'activation (push réussi) et interdit les copies, afin que les
     * classes dérivées puissent se concentrer sur la logique d'appel Pop.
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
        /** Marque le scope comme actif (Push effectué avec succès). */
        void Activate() noexcept { engaged_ = true; }

        /** Indique si le scope doit appeler Pop dans son destructeur. */
        [[nodiscard]] bool IsActive() const noexcept { return engaged_; }

        /** Réinitialise l'état (utilisé si Pop manuel). */
        void Deactivate() noexcept { engaged_ = false; }

    private:
        bool engaged_{false};
    };
}
