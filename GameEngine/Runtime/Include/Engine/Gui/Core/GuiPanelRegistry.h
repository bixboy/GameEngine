#pragma once
#include <unordered_map>
#include <memory>
#include <functional>
#include <vector>
#include "Core/Containers/String.h"
#include "Engine/Gui/Utils/GuiPanelController.h"

namespace BixEngine::Gui
{
    class GuiPanel;

    /**
     * @brief Gestion interne des panneaux GUI et de leurs contrôleurs.
     */
    class GuiPanelRegistry
    {
    public:
        
        /** 
         * @brief Données associées à un panneau (UI + logique).
         */
        struct PanelEntry
        {
            std::unique_ptr<GuiPanel> panel;
            std::unique_ptr<GuiPanelController> controller;
        };

        using MapType = std::unordered_map<String, PanelEntry>;
        using Callback = std::function<void(GuiPanel&)>;

        GuiPanelRegistry() = default;
        ~GuiPanelRegistry() = default;

        // ────────────────────────────────────────────────
        // 🧩 Gestion des panneaux
        // ────────────────────────────────────────────────

        /** Crée un nouveau panneau ou renvoie celui existant. */
        GuiPanel& AddPanel(String name, String title);

        /** Supprime le panneau et son contrôleur associé. */
        void RemovePanel(const String& name);

        /** Recherche un panneau par nom (modifiable). */
        [[nodiscard]] GuiPanel* FindPanel(const String& name) noexcept;

        /** Recherche un panneau par nom (lecture seule). */
        [[nodiscard]] const GuiPanel* FindPanel(const String& name) const noexcept;

        /** Retourne l'entrée complète d'un panneau via son nom. */
        [[nodiscard]] PanelEntry* FindPanelEntry(const String& name) noexcept;

        /** Retourne l'entrée complète d'un panneau via sa référence. */
        [[nodiscard]] PanelEntry* FindPanelEntry(GuiPanel& panel) noexcept;

        /** Version const de FindPanelEntry(name). */
        [[nodiscard]] const PanelEntry* FindPanelEntry(const String& name) const noexcept;

        /** Récupère tous les panneaux (modifiable). */
        [[nodiscard]] std::vector<GuiPanel*> GetAllPanels();

        /** Récupère tous les panneaux (lecture seule). */
        [[nodiscard]] std::vector<const GuiPanel*> GetAllPanels() const;

        /** Supprime tous les panneaux et vide les index. */
        void Clear();

        // ────────────────────────────────────────────────
        // 🔔 Callbacks
        // ────────────────────────────────────────────────

        Callback OnPanelCreated = nullptr; ///< Appelé après la création d’un panneau
        Callback OnPanelRemoved = nullptr; ///< Appelé juste avant la suppression d’un panneau

    private:
        MapType panels_; ///< Conteneur principal (nom → entrée)
        std::unordered_map<GuiPanel*, String> panelToName_;

        /** Ajoute un panneau à l’index inverse. */
        void RegisterPanelIndex_(GuiPanel& panel, const String& name);

        /** Retire un panneau de l’index inverse. */
        void UnregisterPanelIndex_(GuiPanel& panel);
    };
}
