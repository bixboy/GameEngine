#pragma once
#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
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

        template <typename PanelT, typename... Args>
        PanelT& AddPanelOfType(String name, String title, Args&&... args);

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

namespace BixEngine::Gui
{
    template <typename PanelT, typename... Args>
    PanelT& GuiPanelRegistry::AddPanelOfType(String name, String title, Args&&... args)
    {
        static_assert(std::is_base_of_v<GuiPanel, PanelT>, "PanelT must derive from GuiPanel");

        if (auto it = panels_.find(name); it != panels_.end())
        {
            auto& entry = it->second;
            if (!entry.panel)
            {
                entry.panel = std::make_unique<PanelT>(String{name}, String{title}, std::forward<Args>(args)...);
                PanelT& panelRef = static_cast<PanelT&>(*entry.panel);
                RegisterPanelIndex_(panelRef, it->first);
                if (OnPanelCreated)
                    OnPanelCreated(panelRef);
                return panelRef;
            }

            auto* existing = dynamic_cast<PanelT*>(entry.panel.get());
            if (!existing)
                throw std::runtime_error("GuiPanelRegistry::AddPanelOfType — panel already exists with a different type.");

            existing->SetTitle(std::move(title));
            return *existing;
        }

        PanelEntry entry{};
        entry.panel = std::make_unique<PanelT>(String{name}, String{title}, std::forward<Args>(args)...);
        PanelT& panelRef = static_cast<PanelT&>(*entry.panel);
        RegisterPanelIndex_(panelRef, name);

        const String key = name;
        panels_.emplace(key, std::move(entry));

        if (OnPanelCreated)
            OnPanelCreated(panelRef);

        return panelRef;
    }
}
